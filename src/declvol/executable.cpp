#include "declvol/config.h"
#include "declvol/process.h"
#include "declvol/profile.h"
#include "declvol/v1/declvol.grpc.pb.h"
#include "declvol/v1/declvol.pb.h"
#include "declvol/volume.h"
#include "declvol/windows.h"

#include <argparse/argparse.hpp>
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include <filesystem>
#include <future>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace em {
namespace {

constexpr inline std::string_view ExecutableName = EM_EXECUTABLE_NAME;
constexpr inline std::string_view ExecutableVersion = EM_EXECUTABLE_VERSION;
constexpr inline std::uint16_t DefaultPort = 50057;

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
class DeclvolService final : public declvol::v1::Declvol::Service {
public:
  explicit DeclvolService(VolumeProfile profile) : mActiveProfile{std::move(profile)} {}

  grpc::Status SwitchProfile(grpc::ServerContext * /*context*/,
                             const declvol::v1::SwitchProfileRequest *request,
                             ::declvol::v1::SwitchProfileResponse * /*response*/) override try {
    load_profile(request->config_path(), request->profile());
    std::cout << "Switched profile to " << request->profile() << std::endl;
    return grpc::Status::OK;
  } catch (const em::ProfileError &e) {
    return {grpc::StatusCode::FAILED_PRECONDITION, e.what()};
  } catch (const std::system_error &e) {
    return {grpc::StatusCode::UNAVAILABLE, e.what()};
  } catch (...) {
    return {grpc::StatusCode::INTERNAL, "Unhandled exception"};
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
  mutable std::mutex mMut;
  em::VolumeProfile mActiveProfile;
};

/**
 * Client class to interact with the `declvol` service.
 */
class DeclvolClient final {
public:
  explicit DeclvolClient(const std::shared_ptr<grpc::ChannelInterface> &channel)
      : mStub(declvol::v1::Declvol::NewStub(channel)) {}

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
    grpc::ClientContext ctx;

    declvol::v1::SwitchProfileRequest req;
    req.set_profile(profileName);
    req.set_config_path(configPath.string());

    declvol::v1::SwitchProfileResponse resp;

    mStub->SwitchProfile(&ctx, req, &resp);
    (void) resp;
  }

private:
  std::unique_ptr<declvol::v1::Declvol::Stub> mStub;
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

  const auto serviceUri{std::format("localhost:{}", em::DefaultPort)};

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
  std::unique_ptr<em::DeclvolService> service;
  std::unique_ptr<grpc::Server> server;
  if (app.get<bool>("--wait")) {
    service = std::make_unique<em::DeclvolService>(profile);
    grpc::ServerBuilder serverBuilder;
    serverBuilder.AddListeningPort(serviceUri, grpc::InsecureServerCredentials());
    serverBuilder.RegisterService(service.get());
    server = std::move(serverBuilder.BuildAndStart());
    if (!server) {
      // Almost certainly preceded by a big scary gRPC message, so add a newline
      // to try and make this stand out.
      std::cerr << "\nA waiter process is already running.\n";
      return 1;
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

  if (server) {
    const auto eventHandle{em::register_session_notification(
        sessionMgr,
        [svc = service.get()](const winrt::com_ptr<IAudioSessionControl2> &sessionCtrl) {
          em::set_session_volume(svc->get_active_profile(), sessionCtrl);
          return S_OK;
        })};

    // Wait on stdin.
    std::cout << em::ExecutableName << " will now set the volume of launched processes, press enter to stop." << std::endl;
    std::cin.get();

    server->Shutdown();
    em::unregister_session_notification(sessionMgr, eventHandle);
    return 0;
  }

  auto channel{grpc::CreateChannel(serviceUri, grpc::InsecureChannelCredentials())};
  // TODO: Use a more reliable way to check if there is a service running.
  if (!channel->WaitForConnected(std::chrono::system_clock::now() + std::chrono::milliseconds{100})) {
    // No service (probably), this not an error.
    return 0;
  }

  em::DeclvolClient client(channel);
  client.switch_profile(configPath, activeProfileName);

  return 0;
} catch (const em::ProfileError &e) {
  std::cerr << e.what() << '\n';
} catch (const std::exception &e) {
  std::cerr << "Unhandled exception: " << e.what() << '\n';
} catch (const winrt::hresult_error &e) {
  std::cerr << "Unhandled exception: " << winrt::to_string(e.message()) << '\n';
}
