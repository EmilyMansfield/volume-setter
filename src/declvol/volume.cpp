#include "declvol/volume.h"

#include <endpointvolume.h>

namespace em {

winrt::com_ptr<IMMDevice> get_default_audio_device() {
  const auto deviceEnumerator{winrt::create_instance<IMMDeviceEnumerator>(
      winrt::guid_of<MMDeviceEnumerator>(), CLSCTX_ALL, nullptr)};

  winrt::com_ptr<IMMDevice> device;
  winrt::check_hresult(deviceEnumerator->GetDefaultAudioEndpoint(
      EDataFlow::eRender, ERole::eMultimedia, device.put()));

  return device;
}

winrt::com_ptr<IAudioSessionManager2>
get_audio_session_manager(const winrt::com_ptr<IMMDevice> &device) {
  winrt::com_ptr<IAudioSessionManager2> sessionMgr;
  winrt::check_hresult(device->Activate(
      winrt::guid_of<IAudioSessionManager2>(), CLSCTX_ALL, nullptr, sessionMgr.put_void()));
  return sessionMgr;
}

DWORD get_process_id(const winrt::com_ptr<IAudioSessionControl2> &sessionCtrl2) {
  DWORD pid;
  winrt::check_hresult(sessionCtrl2->GetProcessId(&pid));
  return pid;
}

std::optional<float> set_device_volume(
    const VolumeProfile &profile,
    const winrt::com_ptr<IMMDevice> &device) {
  winrt::com_ptr<IAudioEndpointVolume> deviceVolume;
  winrt::check_hresult(device->Activate(
      winrt::guid_of<IAudioEndpointVolume>(), CLSCTX_ALL, nullptr, deviceVolume.put_void()));

  std::optional<float> setVolume{};
  for (const auto &control : profile.controls) {
    if (control.suffix() != ":device") continue;
    const float targetVol{control.relative_volume()};

    winrt::check_hresult(deviceVolume->SetMasterVolumeLevelScalar(targetVol, nullptr));
    setVolume = targetVol;
    // To be consistent with later controls overriding earlier ones when they
    // both match the same executable, do not early exit.
  }
  return setVolume;
}

std::optional<float> set_system_sound_volume(
    const VolumeProfile &profile,
    const winrt::com_ptr<IAudioSessionControl> &sessionCtrl) {
  std::optional<float> setVolume{};
  for (const auto &control : profile.controls) {
    if (control.suffix() != ":system") continue;
    const float targetVol{control.relative_volume()};

    // HACK: See em::set_session_volume()
    const auto volume{sessionCtrl.as<ISimpleAudioVolume>()};
    winrt::check_hresult(volume->SetMasterVolume(targetVol, nullptr));
    setVolume = targetVol;
  }
  return setVolume;
}

std::optional<float> set_session_volume(
    const VolumeProfile &profile,
    std::string_view procName,
    const winrt::com_ptr<IAudioSessionControl> &sessionCtrl) {
  std::optional<float> setVolume{};
  for (const auto &control : profile.controls) {
    if (!procName.ends_with(control.suffix())) continue;
    const float targetVol{control.relative_volume()};

    // HACK: This is undocumented behaviour! At least, as far as I know.
    //       With mild apologies to the Windows developers, I was not able to
    //       do this another way, and it seems quite absurd that a user cannot
    //       programmatically change the volume of their own applications.
    //       Best I've found is this answer by a Microsoft employee stating that
    //       you can often do it: https://stackoverflow.com/a/6084029
    const auto volume{sessionCtrl.as<ISimpleAudioVolume>()};
    winrt::check_hresult(volume->SetMasterVolume(targetVol, nullptr));
    setVolume = targetVol;
  }
  return setVolume;
}

void unregister_session_notification(
    const winrt::com_ptr<IAudioSessionManager2> &mgr,
    const winrt::com_ptr<IAudioSessionNotification> &handle) {
  winrt::check_hresult(mgr->UnregisterSessionNotification(handle.get()));
}

}// namespace em
