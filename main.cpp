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
inline auto get_audio_sessions(IAudioSessionEnumerator *sessionEnum) {
  int numSessions;
  winrt::check_hresult(sessionEnum->GetCount(&numSessions));

  return std::views::iota(0, numSessions) | std::views::transform([sessionEnum](int i) {
           winrt::com_ptr<IAudioSessionControl> sessionCtrl;
           winrt::check_hresult(sessionEnum->GetSession(i, sessionCtrl.put()));
           return sessionCtrl;
         });
}

struct VolumeControl {
  std::string suffix;
  float relative_volume;
};

struct VolumeProfile {
  std::vector<VolumeControl> controls;
};
}// namespace em

int main() {
  std::map<std::string, em::VolumeProfile> profiles;

  auto data{toml::parse("example-profiles.toml")};
  for (const auto &section : data.as_table()) {
    em::VolumeProfile profile{};

    const auto controls{toml::find<std::vector<toml::table>>(section.second, "controls")};
    for (const auto &entry : controls) {
      em::VolumeControl control{};
      control.suffix = entry.at("suffix").as_string();
      control.relative_volume = static_cast<float>(entry.at("volume").as_floating());
      profile.controls.emplace_back(std::move(control));
    }

    profiles.try_emplace(section.first, std::move(profile));
  }

  const std::string activeProfileName{"default"};
  const auto &activeProfile{profiles.at(activeProfileName)};

  winrt::init_apartment();

  const auto deviceEnumerator{winrt::create_instance<IMMDeviceEnumerator>(
      winrt::guid_of<MMDeviceEnumerator>(), CLSCTX_ALL, nullptr)};

  winrt::com_ptr<IMMDevice> device;
  winrt::check_hresult(deviceEnumerator->GetDefaultAudioEndpoint(
      EDataFlow::eRender, ERole::eMultimedia, device.put()));

  winrt::com_ptr<IAudioEndpointVolume> deviceVolume;
  winrt::check_hresult(device->Activate(
      winrt::guid_of<IAudioEndpointVolume>(), CLSCTX_ALL, nullptr, deviceVolume.put_void()));

  for (const auto &control : activeProfile.controls) {
    if (control.suffix != "*") continue;
    winrt::check_hresult(deviceVolume->SetMasterVolumeLevelScalar(control.relative_volume, nullptr));
    // To be consistent with later controls overriding earlier ones when they
    // both match the same executable, do not early exit.
  }

  winrt::com_ptr<IAudioSessionManager2> sessionMgr;
  winrt::check_hresult(device->Activate(
      winrt::guid_of<IAudioSessionManager2>(), CLSCTX_ALL, nullptr, sessionMgr.put_void()));

  winrt::com_ptr<IAudioSessionEnumerator> sessionEnum;
  winrt::check_hresult(sessionMgr->GetSessionEnumerator(sessionEnum.put()));

  for (const auto &sessionCtrl : em::get_audio_sessions(sessionEnum.get())) {
    const auto sessionCtrl2{sessionCtrl.as<IAudioSessionControl2>()};

    // To get reliable name information about the session we need the PID of
    // the process managing it. There is `IAudioSessionControl::GetDisplayName`,
    // but it's up to the application to set that and many do not. `sndvol` has
    // to create a fallback in that case, we opt to instead match the executable
    // path.
    //
    // TODO: Support setting the system sounds volume.

    DWORD pid;
    winrt::check_hresult(sessionCtrl2->GetProcessId(&pid));
    if (pid == 0) {
      // System idle process
      continue;
    }

    const winrt::handle procHnd{::OpenProcess(
        PROCESS_QUERY_LIMITED_INFORMATION, /*bInheritHandle=*/false, pid)};
    if (!procHnd) {
      std::cerr << "Failed to open process with PID " << pid << '\n';
      continue;
    }

    // TODO: Support paths longer than MAX_PATH, which has been superseded on
    //       modern systems.
    std::string procName(MAX_PATH, '\0');
    auto procNameSize{static_cast<DWORD>(procName.size())};
    winrt::check_bool(::QueryFullProcessImageNameA(procHnd.get(), 0, procName.data(), &procNameSize));
    procName.resize(procNameSize);

    for (const auto &control : activeProfile.controls) {
      if (!procName.ends_with(control.suffix)) continue;

      const auto volume{sessionCtrl.as<ISimpleAudioVolume>()};
      volume->SetMasterVolume(control.relative_volume, nullptr);
      std::cout << "Set volume of " << procName << " to " << control.relative_volume << '\n';
    }
  }

  return 0;
}
