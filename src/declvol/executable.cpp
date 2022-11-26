#include "declvol/config.h"
#include "declvol/process.h"
#include "declvol/profile.h"
#include "declvol/volume.h"
#include "declvol/windows.h"

#include <argparse/argparse.hpp>

#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace em {
namespace {

constexpr inline std::string_view ExecutableName = EM_EXECUTABLE_NAME;
constexpr inline std::string_view ExecutableVersion = EM_EXECUTABLE_VERSION;

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
 * Set the volume of each running application matching a `VolumeControl` in the
 * `VolumeProfile`.
 */
void set_all_volumes(const VolumeProfile &profile) {
  const auto device{em::get_default_audio_device()};
  const auto sessionMgr{em::get_audio_session_manager(device)};
  if (const auto v{em::set_device_volume(profile, device)}) {
    std::cout << "Set volume of device to " << *v << '\n';
  }

  for (const auto &sessionCtrl : em::get_audio_sessions(sessionMgr)) {
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
      continue;
    }

    const auto pid{em::get_process_id(sessionCtrl2)};
    // PID should be nonzero since we've already handled the system sounds.
    const auto procHnd{em::open_process(pid)};
    const auto procName{em::get_process_image_name(procHnd)};
    if (const auto v{em::set_session_volume(profile, procName, sessionCtrl)}) {
      std::cout << "Set volume of " << procName << " to " << *v << '\n';
    }
  }
}

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
  em::set_all_volumes(activeProfileIt->second);

  return 0;
} catch (const em::ProfileError &e) {
  std::cerr << e.what() << '\n';
} catch (const std::exception &e) {
  std::cerr << "Unhandled exception: " << e.what() << '\n';
} catch (const winrt::hresult_error &e) {
  std::cerr << "Unhandled exception: " << winrt::to_string(e.message()) << '\n';
}
