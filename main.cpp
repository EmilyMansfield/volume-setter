#include <audiopolicy.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <winrt/base.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  float globalVolume = 0.27f;
  // The value displayed by sndvol is the product of this value and the
  // volume of the device (globalVolume), e.g. overall volume 26 with master
  // volume for steam set to 0.23 displays steam as 6.
  // These strings are matched against the end of the process image name.
  std::vector<std::pair<std::wstring, float>> volumes{
      {L"\\chrome.exe", 0.9f},
      {L"\\steam.exe", 0.3f},
  };

  winrt::init_apartment();

  const auto deviceEnumerator{winrt::create_instance<IMMDeviceEnumerator>(
      winrt::guid_of<MMDeviceEnumerator>(), CLSCTX_ALL, nullptr)};

  winrt::com_ptr<IMMDevice> device;
  winrt::check_hresult(deviceEnumerator->GetDefaultAudioEndpoint(
      EDataFlow::eRender, ERole::eMultimedia, device.put()));

  winrt::com_ptr<IAudioEndpointVolume> deviceVolume;
  winrt::check_hresult(device->Activate(
      winrt::guid_of<IAudioEndpointVolume>(), CLSCTX_ALL, nullptr, deviceVolume.put_void()));

  winrt::check_hresult(deviceVolume->SetMasterVolumeLevelScalar(globalVolume, nullptr));

  winrt::com_ptr<IAudioSessionManager2> sessionMgr;
  winrt::check_hresult(device->Activate(
      winrt::guid_of<IAudioSessionManager2>(), CLSCTX_ALL, nullptr, sessionMgr.put_void()));

  winrt::com_ptr<IAudioSessionEnumerator> sessionEnum;
  winrt::check_hresult(sessionMgr->GetSessionEnumerator(sessionEnum.put()));

  int numSessions;
  winrt::check_hresult(sessionEnum->GetCount(&numSessions));
  std::wcout << "Found " << numSessions << " audio sessions\n";

  for (int i = 0; i < numSessions; ++i) {
    winrt::com_ptr<IAudioSessionControl> sessionCtrl;
    winrt::check_hresult(sessionEnum->GetSession(i, sessionCtrl.put()));
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
      std::wcerr << "Failed to open process with PID " << pid << " for audio session " << i << '\n';
      continue;
    }

    // TODO: Support paths longer than MAX_PATH, which has been superseded on
    //       modern systems.
    std::wstring procName(MAX_PATH, '\0');
    auto procNameSize{static_cast<DWORD>(procName.size())};
    winrt::check_bool(::QueryFullProcessImageNameW(procHnd.get(), 0, procName.data(), &procNameSize));
    procName.resize(procNameSize);

    std::wcout << "Session " << i << ": " << procName << '\n';

    for (const auto &entry : volumes) {
      if (!procName.ends_with(entry.first)) continue;

      const auto volume{sessionCtrl.as<ISimpleAudioVolume>()};
      volume->SetMasterVolume(entry.second, nullptr);
      std::wcout << "Set volume of " << procName << " to " << entry.second << '\n';
    }
  }

  return 0;
}
