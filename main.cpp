#include <argparse/argparse.hpp>
#include <toml.hpp>

#include <audiopolicy.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <shlobj_core.h>

#include <winrt/base.h>

#include <iostream>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

namespace em {
namespace {

struct VolumeControl {
  std::string suffix;
  float relative_volume;
};

struct VolumeProfile {
  std::vector<VolumeControl> controls;
};

/**
 * Return the volume profiles defined by a TOML configuration file.
 */
std::map<std::string, em::VolumeProfile>
parse_profiles_toml(const std::filesystem::path &profilePath) {
  const auto data{toml::parse(profilePath)};

  std::map<std::string, em::VolumeProfile> profiles;
  for (const auto &section : data.as_table()) {
    em::VolumeProfile profile{};

    const auto controls{toml::find<std::vector<toml::table>>(section.second, "controls")};
    for (const auto &entry : controls) {
      profile.controls.emplace_back(em::VolumeControl{
          .suffix = entry.at("suffix").as_string(),
          .relative_volume = static_cast<float>(entry.at("volume").as_floating()),
      });
    }

    profiles.try_emplace(section.first, std::move(profile));
  }

  return profiles;
}

/**
 * Return the path of the current user's local app data folder.
 *
 * On Windows this is the user's `LocalAppData` folder, which usually
 * corresponds to the value of the environment variable `LOCALAPPDATA`.
 */
std::filesystem::path local_app_data();

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

  return em::local_app_data() / "volume-setter" / "config.toml";
}

/**
 * Return the PID of the process managing the audio session.
 */
DWORD get_process_id(const winrt::com_ptr<IAudioSessionControl2> &sessionCtrl2) {
  DWORD pid;
  winrt::check_hresult(sessionCtrl2->GetProcessId(&pid));
  return pid;
}

/**
 * Return the full executable name of the given process.
 */
std::string get_process_image_name(const winrt::handle &processHandle) {
  constexpr DWORD PROCESS_NAME_WIN32{0};

  // MAX_PATH includes the null-terminator
  std::string procName(MAX_PATH - 1, '\0');
  auto procNameSize{static_cast<DWORD>(procName.size())};
  winrt::check_bool(::QueryFullProcessImageNameA(
      processHandle.get(), PROCESS_NAME_WIN32, procName.data(), &procNameSize));
  procName.resize(procNameSize);

  return procName;
}

/**
 * Return the path of the current user's local app data folder.
 */
std::filesystem::path local_app_data() {
  wchar_t *path{};
  const winrt::hresult result{::SHGetKnownFolderPath(
      FOLDERID_LocalAppData, /*dwFlags=*/0, /*hToken=*/nullptr, &path)};

  // Documentation states that `CoTaskMemFree` must be called even if the call
  // to `SHGetKnownFolderPath` fails, use RAII to ensure this happens even if
  // the path construction throws.
  struct DeallocString {
    wchar_t *data;
    ~DeallocString() { ::CoTaskMemFree(data); }
  } deallocAppdataPathRaw{path};

  winrt::check_hresult(result);

  return std::filesystem::path{path};
}

/**
 * Return a view over the session controls enumerated by a session enumerator.
 */
auto get_audio_sessions(const winrt::com_ptr<IAudioSessionEnumerator> &sessionEnum) {
  int numSessions;
  winrt::check_hresult(sessionEnum->GetCount(&numSessions));

  return std::views::iota(0, numSessions) | std::views::transform([sessionEnum](int i) {
           winrt::com_ptr<IAudioSessionControl> sessionCtrl;
           winrt::check_hresult(sessionEnum->GetSession(i, sessionCtrl.put()));
           return sessionCtrl;
         });
}

/**
 * Return a view over the session controls for the sessions on an audio device.
 */
auto get_audio_sessions(const winrt::com_ptr<IMMDevice> &device) {
  winrt::com_ptr<IAudioSessionManager2> sessionMgr;
  winrt::check_hresult(device->Activate(
      winrt::guid_of<IAudioSessionManager2>(), CLSCTX_ALL, nullptr, sessionMgr.put_void()));

  winrt::com_ptr<IAudioSessionEnumerator> sessionEnum;
  winrt::check_hresult(sessionMgr->GetSessionEnumerator(sessionEnum.put()));

  return em::get_audio_sessions(sessionEnum);
}

/**
 * Return the default output multimedia audio device.
 */
auto get_default_audio_device() {
  const auto deviceEnumerator{winrt::create_instance<IMMDeviceEnumerator>(
      winrt::guid_of<MMDeviceEnumerator>(), CLSCTX_ALL, nullptr)};

  winrt::com_ptr<IMMDevice> device;
  winrt::check_hresult(deviceEnumerator->GetDefaultAudioEndpoint(
      EDataFlow::eRender, ERole::eMultimedia, device.put()));

  return device;
}

/**
 * Set the volume of the device to that specified in the profile.
 *
 * The device volume is given by controls with suffix `:device`. Like actual
 * suffixes, if there are multiple controls with this suffix then whichever
 * comes last takes precedence.
 */
std::optional<float> set_device_volume(
    const VolumeProfile &profile,
    const winrt::com_ptr<IMMDevice> &device) {
  winrt::com_ptr<IAudioEndpointVolume> deviceVolume;
  winrt::check_hresult(device->Activate(
      winrt::guid_of<IAudioEndpointVolume>(), CLSCTX_ALL, nullptr, deviceVolume.put_void()));

  std::optional<float> setVolume{};
  for (const auto &control : profile.controls) {
    if (control.suffix != ":device") continue;
    winrt::check_hresult(deviceVolume->SetMasterVolumeLevelScalar(control.relative_volume, nullptr));
    setVolume = control.relative_volume;
    // To be consistent with later controls overriding earlier ones when they
    // both match the same executable, do not early exit.
  }
  return setVolume;
}

/**
 * Set the system sound volume to that specified in the profile.
 *
 * The system sound volume is given by controls with suffix `:system`. Like
 * actual suffixes, if there are multiple controls with this suffix then
 * whichever comes last takes precedence. `sessionCtrl` must be the system
 * sounds sessions.
 */
std::optional<float> set_system_sound_volume(
    const VolumeProfile &profile,
    const winrt::com_ptr<IAudioSessionControl> &sessionCtrl) {
  std::optional<float> setVolume{};
  for (const auto &control : profile.controls) {
    if (control.suffix != ":system") continue;

    // HACK: See em::set_session_volume()
    const auto volume{sessionCtrl.as<ISimpleAudioVolume>()};
    volume->SetMasterVolume(control.relative_volume, nullptr);
    setVolume = control.relative_volume;
  }
  return setVolume;
}

/**
 * Set the volume of a session with the given process image path.
 *
 * The last volume control whose suffix matches the `procName` is used to set
 * the volume of the given session, which must be managed by a process with the
 * given name.
 */
std::optional<float> set_session_volume(
    const VolumeProfile &profile,
    std::string_view procName,
    const winrt::com_ptr<IAudioSessionControl> &sessionCtrl) {
  std::optional<float> setVolume{};
  for (const auto &control : profile.controls) {
    if (!procName.ends_with(control.suffix)) continue;

    // HACK: This is undocumented behaviour! At least, as far as I know.
    //       With mild apologies to the Windows developers, I was not able to
    //       do this another way, and it seems quite absurd that a user cannot
    //       programmatically change the volume of their own applications.
    //       Best I've found is this answer by a Microsoft employee stating that
    //       you can often do it: https://stackoverflow.com/a/6084029
    const auto volume{sessionCtrl.as<ISimpleAudioVolume>()};
    volume->SetMasterVolume(control.relative_volume, nullptr);
    setVolume = control.relative_volume;
  }
  return setVolume;
}

/**
 * Set the volume of each running application matching a `VolumeControl` in the
 * `VolumeProfile`.
 */
void set_all_volumes(const VolumeProfile &profile) {
  const auto device{em::get_default_audio_device()};
  if (const auto v{em::set_device_volume(profile, device)}) {
    std::cout << "Set volume of device to " << *v << '\n';
  }

  for (const auto &sessionCtrl : em::get_audio_sessions(device)) {
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
    const winrt::handle procHnd{::OpenProcess(
        PROCESS_QUERY_LIMITED_INFORMATION, /*bInheritHandle=*/false, pid)};
    if (!procHnd) {
      std::cerr << "Failed to open process with PID " << pid << '\n';
      continue;
    }
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

  argparse::ArgumentParser app("volume-setter", "0.1.0");
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
  const auto &activeProfile{profiles.at(app.get<std::string>("profile"))};
  em::set_all_volumes(activeProfile);

  return 0;
} catch (const std::exception &e) {
  std::cerr << "Unhandled exception: " << e.what() << '\n';
} catch (const winrt::hresult_error &e) {
  std::cerr << "Unhandled exception: " << winrt::to_string(e.message()) << '\n';
}
