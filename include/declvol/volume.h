#ifndef VOLUME_SETTER_INCLUDE_DECLVOL_VOLUME_H
#define VOLUME_SETTER_INCLUDE_DECLVOL_VOLUME_H

#include "declvol/profile.h"
#include "declvol/windows.h"

#include <audiopolicy.h>
#include <mmdeviceapi.h>

#include <concepts>
#include <functional>
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
 *
 * Note that calling this method twice on the same device returns a _different_
 * audio manager.
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
 * Return a view over the session controls for the sessions managed by a
 * session manager.
 */
auto get_audio_sessions(const winrt::com_ptr<IAudioSessionManager2> &sessionMgr) {
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
std::optional<float> set_named_session_volume(
    const VolumeProfile &profile,
    std::string_view procName,
    const winrt::com_ptr<IAudioSessionControl> &sessionCtrl);

/**
 * Callable to be invoked when an audio session is created.
 *
 * Handlers should return `S_OK`.
 */
template<class T>
concept session_notification_handler =
    std::invocable<T, const winrt::com_ptr<IAudioSessionControl2> &>
    && std::is_convertible_v<
        std::invoke_result_t<T, const winrt::com_ptr<IAudioSessionControl2> &>,
        winrt::hresult>;

/**
 * Register a handler to be called when a new audio session is created.
 *
 * The handler should be deregistered by a call to
 * `unregister_session_notification` when it is no longer required.
 */
template<session_notification_handler F>
winrt::com_ptr<IAudioSessionNotification> register_session_notification(
    const winrt::com_ptr<IAudioSessionManager2> &mgr, F &&callback) {
  struct callback_t : winrt::implements<callback_t, IAudioSessionNotification> {
    std::move_only_function<winrt::hresult(const winrt::com_ptr<IAudioSessionControl2> &)> f;

    explicit callback_t(F &&f) : f{std::move(f)} {}

    HRESULT STDMETHODCALLTYPE OnSessionCreated(IAudioSessionControl *session) noexcept override try {
      winrt::com_ptr<IAudioSessionControl> ownedSession;
      ownedSession.copy_from(session);
      return std::invoke(f, ownedSession.as<IAudioSessionControl2>());
    } catch (...) {
      return winrt::to_hresult();
    }
  };

  const auto c{winrt::make<callback_t>(std::forward<F>(callback))};
  winrt::check_hresult(mgr->RegisterSessionNotification(c.get()));
  return c;
}

/**
 * Unregister a previously registered session notification handler.
 */
void unregister_session_notification(
    const winrt::com_ptr<IAudioSessionManager2> &mgr,
    const winrt::com_ptr<IAudioSessionNotification> &handle);

}// namespace em

#endif// VOLUME_SETTER_INCLUDE_DECLVOL_VOLUME_H
