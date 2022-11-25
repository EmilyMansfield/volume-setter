#ifndef VOLUME_SETTER_INCLUDE_DECLVOL_VOLUME_H
#define VOLUME_SETTER_INCLUDE_DECLVOL_VOLUME_H

#include "declvol/profile.h"
#include "declvol/windows.h"

#include <audiopolicy.h>
#include <mmdeviceapi.h>

#include <optional>
#include <ranges>
#include <string_view>

namespace em {

/**
 * Return the default output multimedia audio device.
 */
winrt::com_ptr<IMMDevice> get_default_audio_device();

/**
 * Return an audio session manager for an audio device.
 */
winrt::com_ptr<IAudioSessionManager2>
get_audio_session_manager(const winrt::com_ptr<IMMDevice> &device);

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
  const auto sessionMgr{em::get_audio_session_manager(device)};

  winrt::com_ptr<IAudioSessionEnumerator> sessionEnum;
  winrt::check_hresult(sessionMgr->GetSessionEnumerator(sessionEnum.put()));

  return em::get_audio_sessions(sessionEnum);
}

/**
 * Return the PID of the process managing the audio session.
 */
DWORD get_process_id(const winrt::com_ptr<IAudioSessionControl2> &sessionCtrl2);

/**
 * Set the volume of the device to that specified in the profile.
 *
 * The device volume is given by controls with suffix `:device`. Like actual
 * suffixes, if there are multiple controls with this suffix then whichever
 * comes last takes precedence.
 */
std::optional<float> set_device_volume(
    const VolumeProfile &profile,
    const winrt::com_ptr<IMMDevice> &device);

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
    const winrt::com_ptr<IAudioSessionControl> &sessionCtrl);

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
    const winrt::com_ptr<IAudioSessionControl> &sessionCtrl);

}// namespace em

#endif// VOLUME_SETTER_INCLUDE_DECLVOL_VOLUME_H
