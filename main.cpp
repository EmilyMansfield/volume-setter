#include <toml.hpp>

#include <audiopolicy.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>

#include <winrt/base.h>

#include <iostream>
#include <ranges>
#include <string>
#include <vector>

namespace em {

/**
 * Return a view over the session controls enumerated by a session enumerator.
 */
inline auto get_audio_sessions(const winrt::com_ptr<IAudioSessionEnumerator> &sessionEnum) {
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
inline auto get_audio_sessions(const winrt::com_ptr<IMMDevice> &device) {
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
inline auto get_default_audio_device() {
  const auto deviceEnumerator{winrt::create_instance<IMMDeviceEnumerator>(
      winrt::guid_of<MMDeviceEnumerator>(), CLSCTX_ALL, nullptr)};

  winrt::com_ptr<IMMDevice> device;
  winrt::check_hresult(deviceEnumerator->GetDefaultAudioEndpoint(
      EDataFlow::eRender, ERole::eMultimedia, device.put()));

  return device;
}

/**
 * Return the PID of the process managing the audio session.
 */
inline DWORD get_process_id(const winrt::com_ptr<IAudioSessionControl2> &sessionCtrl2) {
  DWORD pid;
  winrt::check_hresult(sessionCtrl2->GetProcessId(&pid));
  return pid;
}

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
inline std::map<std::string, em::VolumeProfile>
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
 * Set the volume of the device to that specified in the profile.
 *
 * The device volume is given by controls with suffix `:device`. Like actual
 * suffixes, if there are multiple controls with this suffix then whichever
 * comes last takes precedence.
 */
inline void set_device_volume(const VolumeProfile &profile, const winrt::com_ptr<IMMDevice> &device) {
  winrt::com_ptr<IAudioEndpointVolume> deviceVolume;
  winrt::check_hresult(device->Activate(
      winrt::guid_of<IAudioEndpointVolume>(), CLSCTX_ALL, nullptr, deviceVolume.put_void()));

  for (const auto &control : profile.controls) {
    if (control.suffix != ":device") continue;
    winrt::check_hresult(deviceVolume->SetMasterVolumeLevelScalar(control.relative_volume, nullptr));
    std::cout << "Set volume of device to " << control.relative_volume << '\n';
    // To be consistent with later controls overriding earlier ones when they
    // both match the same executable, do not early exit.
  }
}

/**
 * Set the system sound volume to that specified in the profile.
 *
 * The system sound volume is given by controls with suffix `:system`. Like
 * actual suffixes, if there are multiple controls with this suffix then
 * whichever comes last takes precedence. `sessionCtrl` must be the system
 * sounds sessions.
 */
inline void set_system_sound_volume(const VolumeProfile &profile,
                                    const winrt::com_ptr<IAudioSessionControl> &sessionCtrl) {
  for (const auto &control : profile.controls) {
    if (control.suffix != ":system") continue;

    const auto volume{sessionCtrl.as<ISimpleAudioVolume>()};
    volume->SetMasterVolume(control.relative_volume, nullptr);
    std::cout << "Set volume of system sounds to " << control.relative_volume << '\n';
  }
}

/**
 * Set the volume of a session with the given process image path.
 *
 * The last volume control whose suffix matches the `procName` is used to set
 * the volume of the given session, which must be managed by a process with the
 * given name.
 */
inline void set_session_volume(const VolumeProfile &profile,
                               std::string_view procName,
                               const winrt::com_ptr<IAudioSessionControl> &sessionCtrl) {
  for (const auto &control : profile.controls) {
    if (!procName.ends_with(control.suffix)) continue;

    const auto volume{sessionCtrl.as<ISimpleAudioVolume>()};
    volume->SetMasterVolume(control.relative_volume, nullptr);
    std::cout << "Set volume of " << procName << " to " << control.relative_volume << '\n';
  }
}
}// namespace em

int main() {
  const auto profiles{em::parse_profiles_toml("example-profiles.toml")};

  const std::string activeProfileName{"default"};
  const auto &activeProfile{profiles.at(activeProfileName)};

  winrt::init_apartment();

  const auto device{em::get_default_audio_device()};
  em::set_device_volume(activeProfile, device);

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
      em::set_system_sound_volume(activeProfile, sessionCtrl);
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
    em::set_session_volume(activeProfile, procName, sessionCtrl);
  }

  return 0;
}
