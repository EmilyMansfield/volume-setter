#include "declvol/config.h"
#include "declvol/process.h"
#include "declvol/profile.h"
#include "declvol/v1/declvol.pb.h"
#include "declvol/volume.h"
#include "declvol/windows.h"

#include <argparse/argparse.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include <filesystem>
#include <future>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ipc = boost::interprocess;

namespace em {
namespace {

constexpr inline std::string_view ExecutableName = EM_EXECUTABLE_NAME;
constexpr inline std::string_view ExecutableVersion = EM_EXECUTABLE_VERSION;

/**
 * Name of the interprocess queue used to notify the waiter process, if any,
 * that the active profile has changed.
 *
 * On Windows at least, Boost.Interprocess creates a shared memory mapped file
 * with this name, so it needs to be unique among all other programs that use
 * that directory. Hopefully this one is, but if anybody finds a collision then
 * it can be changed.
 *
 * For consistency reasons, it's important that at most one waiter process is
 * running at a time across all versions of the software. This includes forks!
 * This is guaranteed provided that all versions use the same `RpcQueueName`.
 *
 * If a breaking change to the queue or message format would cause buggy
 * behaviour when a waiter and setter from different versions across that
 * breaking change interact, then it may be better to increase the version
 * number on the queue. This allows those versions to exist independently,
 * preventing any bugs from mismatched queue formats, but violating consistency
 * unless users are careful to not run those two versions simultaneously.
 * Protobuf allows a number of changes without breaking compatibility, so this
 * is unlikely to be necessary.
 */
constexpr inline std::string_view RpcQueueName = "em_volume_setter_ipc_queue_v1";

/**
 * Maximum size of a serialized message in the interprocess queue.
 *
 * This sets a limit on the maximum size of the serialized Protobuf messages
 * used to communicate between waiters and setters. Increasing it does not
 * constitute a breaking change.
 */
constexpr inline std::size_t MaxMessageSize = 512ull;

/**
 * Return the path to the config file in which the profiles are defined.
 *
 * If `--config` is passed as a command line argument then its value is used as
 * the path to the config file. Otherwise, `./volume-setter/config.toml` will be
 * used relative to the directory returned by `local_app_data()`.
 */
std::filesystem::path get_config_path(const argparse::ArgumentParser &app) {
  if (auto configPath{app.present<std::string>("--config")}) {
    return *configPath;
  }

  return em::get_default_config_path();
}

/**
 * Set the volume of an audio session.
 *
 * Unlike `set_named_session_volume` this does not need a process name and will
 * also work with the system audio session.
 */
void set_session_volume(const VolumeProfile &profile,
                        const winrt::com_ptr<IAudioSessionControl> &sessionCtrl) {
  const auto sessionCtrl2{sessionCtrl.as<IAudioSessionControl2>()};

  // To get reliable name information about the session we need the PID of
  // the process managing it. There is `IAudioSessionControl::GetDisplayName`,
  // but it's up to the application to set that and many do not. `sndvol` has
  // to create a fallback in that case, we opt to instead match the executable
  // path.

  // For the system process we can't get executable path from the PID, but
  // we can still get the PID, which is zero. We actually can use
  // `GetDisplayName` in this case, because the system process sets it,
  // but rather than trying to match on that or relying on the PID we can
  // instead use `IAudioSessionControl2::IsSystemSoundsSession`, which sounds
  // much more reliable.
  if (sessionCtrl2->IsSystemSoundsSession() == S_OK) {
    if (const auto v{em::set_system_sound_volume(profile, sessionCtrl)}) {
      std::cout << "Set volume of system sounds to " << *v << '\n';
    }
    return;
  }

  const auto pid{em::get_process_id(sessionCtrl2)};
  // PID should be nonzero since we've already handled the system sounds.
  const auto procHnd{em::open_process(pid)};
  const auto procName{em::get_process_image_name(procHnd)};
  if (const auto v{em::set_named_session_volume(profile, procName, sessionCtrl)}) {
    std::cout << "Set volume of " << procName << " to " << *v << '\n';
  }
}

/**
 * Implementation of the `declvol` service managing an active volume profile.
 *
 * This service only manages which profile is currently active, it does not
 * change the volume of any sessions itself. The volume of any sessions opened
 * after the service has started should be set by the session handler, and the
 * volume of any sessions that are already open should be set by the client
 * notifying the service of the profile change.
 */
class DeclvolService {
public:
  explicit DeclvolService(ipc::message_queue &channel, VolumeProfile profile)
      : mChannel{channel}, mActiveProfile{std::move(profile)} {}

  /**
   * Change the active profile used by the service to the one described by the
   * Protobuf message.
   */
  void switch_profile(const declvol::v1::SwitchProfileRequest *request) {
    load_profile(request->config_path(), request->profile());
    std::cout << "Switched profile to " << request->profile() << std::endl;
  }

  /**
   * Pull requests from the interprocess queue and run them until `shutdown`
   * has been called.
   *
   * Once `shutdown` is called there may be a delay of up to a second until
   * this function returns. This is because, unlike some concurrent queues,
   * Boost.Interprocess's message queue does not have any way of closing it and
   * waking receivers. Pulling until the queue is closed therefore involves
   * either sending an in-band shutdown message to the queue, or receiving with
   * a timeout and polling an out-of-band signal in between.
   *
   * Since an in-band approach requires checking the type of the serialized
   * message, and implementing a new Protobuf for essentially an implementation
   * detail, this function polls.
   */
  void wait() {
    constexpr auto pollInterval = std::chrono::seconds{1};
    std::array<std::byte, em::MaxMessageSize> buf{};

    do {
      std::size_t size{};
      unsigned int priority{};
      const auto deadline{std::chrono::steady_clock::now() + pollInterval};
      if (!mChannel.timed_receive(buf.data(), buf.size(), size, priority, deadline)) {
        continue;
      }

      ::declvol::v1::SwitchProfileRequest request;
      if (!request.ParseFromArray(buf.data(), static_cast<int>(size))) {
        std::cerr << "Received invalid SwitchProfileRequest\n";
        continue;
      }
      switch_profile(&request);
    } while (!mCloseFlag.test());
  }

  /**
   * Close the queue so that any call to `wait` on the queue returns.
   *
   * Once the queue has been closed it cannot be reopened.
   */
  void shutdown() {
    mCloseFlag.test_and_set();
    mCloseFlag.notify_all();
  }

  /**
   * Return a copy of the currently active profile.
   *
   * This function is thread-safe.
   *
   * This function returns a copy to avoid concurrency issues where the profile
   * could be modified while the profile is being changed. This is not the most
   * efficient way to do it, but it's simple and sufficient.
   */
  em::VolumeProfile get_active_profile() const {
    std::lock_guard lock{mMut};
    return mActiveProfile;
  }

  /**
   * Load a volume profile from a config file and set it to the currently
   * active profile.
   *
   * This function is thread-safe.
   *
   * Because the service only manages which profile is currently active, this
   * function does not change the volume of any sessions. Any session opened
   * after this call will have its volume set correctly by the session handler,
   * and any existing sessions should be set by the client.
   */
  void load_profile(const std::filesystem::path &configPath,
                    const std::string &profileName) {
    const auto profiles{em::parse_profiles_toml(configPath)};
    const auto activeProfileIt{profiles.find(profileName)};
    if (activeProfileIt == profiles.end()) {
      throw em::ProfileError(configPath,
                             std::format("Profile {} does not exist", profileName));
    }

    {
      std::lock_guard lock{mMut};
      mActiveProfile = activeProfileIt->second;
    }
  }

private:
  ipc::message_queue &mChannel;
  mutable std::mutex mMut;
  em::VolumeProfile mActiveProfile;
  std::atomic_flag mCloseFlag;
};

/**
 * Client class to interact with the `declvol` service.
 */
class DeclvolClient final {
public:
  explicit DeclvolClient(ipc::message_queue &channel) : mChannel{channel} {}

  /**
   * Notify the connected waiter process that the active profile has changed
   * so that it can correctly set the volume of future audio sessions.
   *
   * The waiter service does not set the volume of any existing sessions, so
   * the intention is that client's will change the volume of all existing
   * processes and also issue this notification to the service.
   */
  void switch_profile(const std::filesystem::path &configPath,
                      const std::string &profileName) {
    declvol::v1::SwitchProfileRequest req;
    req.set_profile(profileName);
    req.set_config_path(configPath.string());

    const auto buf{req.SerializeAsString()};
    if (!mChannel.try_send(buf.data(), buf.size(), 0)) {
      throw std::runtime_error("Cannot notify waiter that active profile is changed, too many requests in queue.");
    }
  }

private:
  ipc::message_queue &mChannel;
};

}// namespace
}// namespace em

int main(int argc, char *argv[]) try {
  winrt::init_apartment();

  argparse::ArgumentParser app(std::string{em::ExecutableName},
                               std::string{em::ExecutableVersion});
  app.add_description("Set the volume of running programs to preset values.");
  app.add_argument("profile")
      .help("name of the profile to make active")
      .required();
  app.add_argument("--config")
      .help("path to the configuration file");
  app.add_argument("--wait")
      .implicit_value(true)
      .default_value(false)
      .help("keep running and modify the volume of programs when they start.");

  try {
    app.parse_args(argc, argv);
  } catch (const std::runtime_error &e) {
    std::cerr << e.what() << '\n'
              << app;
    return 1;
  }

  const auto configPath{em::get_config_path(app)};
  const auto profiles{em::parse_profiles_toml(configPath)};
  const auto activeProfileName{app.get<std::string>("profile")};
  const auto activeProfileIt{profiles.find(activeProfileName)};
  if (activeProfileIt == profiles.end()) {
    std::cerr << "[error] Profile " << activeProfileName << " in "
              << configPath.string() << " does not exist\n";
    return 1;
  }
  const auto &profile{activeProfileIt->second};

  const auto device{em::get_default_audio_device()};
  const auto sessionMgr{em::get_audio_session_manager(device)};

  // A waiter cannot be launched without an active profile, because it would not
  // be able to set volumes. If it didn't also set volumes of existing processes
  // on startup, then the volume state would not match the profile. Therefore,
  // waiters are also setters.
  //
  // While ordinarily there should be at most one waiter running, we cannot
  // guarantee that without trying to start the waiter service. Therefore, start
  // the waiter service for waiter processes before we set any volumes.
  // So that it is easier to maintain compatibility in the future, exit if a
  // waiter is already running.

  std::unique_ptr<ipc::message_queue> channel;
  std::unique_ptr<em::DeclvolService> service;
  std::future<void> serviceSignal;

  if (app.get<bool>("--wait")) {
    // Note: for consistency reasons one might want to delete the queue first,
    //       under the assumption that it will not be deleted if it's in use.
    //       This would help avoid any issues due to crashes where the queue is
    //       not deleted but waiters can't start because it already exists.
    //       However, this assumption is false, and the queue _can_ be deleted
    //       while processes still have access to it by renaming the backing
    //       file and keeping the handles open.

    try {
      channel = std::make_unique<ipc::message_queue>(
          ipc::create_only, em::RpcQueueName.data(), 1, em::MaxMessageSize);
    } catch (const ipc::interprocess_exception &) {
      std::cerr << "A waiter process is already running\n";
      return 1;
    }

    service = std::make_unique<em::DeclvolService>(*channel, profile);
    serviceSignal = std::async(std::launch::async, [svc = service.get()] {
      svc->wait();
    });
  } else {
    try {
      channel = std::make_unique<ipc::message_queue>(ipc::open_only, em::RpcQueueName.data());
    } catch (const ipc::interprocess_exception &) {
      // Could not open channel, presumably because it hasn't been created by a
      // waiter. channel will remain default-initialized in this case.
    }
  }

  // Now we are either a proper setter, or the sole waiter, and therefore it is
  // safe to set volumes. Only a proper setter must try to notify a waiter of
  // the active profile change.

  if (const auto v{em::set_device_volume(profile, device)}) {
    std::cout << "Set volume of device to " << *v << '\n';
  }

  for (const auto &sessionCtrl : em::get_audio_sessions(sessionMgr)) {
    em::set_session_volume(profile, sessionCtrl);
  }

  if (service) {
    const auto eventHandle{em::register_session_notification(
        sessionMgr,
        [svc = service.get()](const winrt::com_ptr<IAudioSessionControl2> &sessionCtrl) {
          em::set_session_volume(svc->get_active_profile(), sessionCtrl);
          std::flush(std::cout);
          return S_OK;
        })};

    // Wait on stdin.
    std::cout << em::ExecutableName << " will now set the volume of launched processes, press enter to stop." << std::endl;
    std::cin.get();

    service->shutdown();
    em::unregister_session_notification(sessionMgr, eventHandle);
    ipc::message_queue::remove(em::RpcQueueName.data());
    return 0;
  }

  if (channel) {
    em::DeclvolClient client(*channel);
    client.switch_profile(configPath, activeProfileName);
    return 0;
  }

  return 0;
} catch (const em::ProfileError &e) {
  std::cerr << e.what() << '\n';
} catch (const std::exception &e) {
  std::cerr << "Unhandled exception: " << e.what() << '\n';
} catch (const winrt::hresult_error &e) {
  std::cerr << "Unhandled exception: " << winrt::to_string(e.message()) << '\n';
}
