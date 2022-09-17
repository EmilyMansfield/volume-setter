#include <audiopolicy.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>

#include <functional>
#include <iostream>
#include <map>
#include <string>

struct Finally {
  std::function<void()> f;

  ~Finally() { f(); }
};

template<class F>
auto finally(F &&f) { return Finally{std::forward<F>(f)}; }

int main() try {
  float globalVolume = 0.27f;
  // The value displayed by sndvol is the product of this value and the
  // volume of the device (globalVolume), e.g. overall volume 26 with master
  // volume for steam set to 0.23 displays steam as 6.
  // These strings are matched against the end of the process image name.
  std::vector<std::pair<std::wstring, float>> volumes{
      {L"\\chrome.exe", 0.9f},
      {L"\\steam.exe", 0.3f},
  };

  ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  const auto coUninit = finally([] { ::CoUninitialize(); });

  HRESULT result;
  IMMDeviceEnumerator *deviceEnumerator;
  result = ::CoCreateInstance(
      __uuidof(MMDeviceEnumerator),
      /*pUnkOuter=*/nullptr,
      CLSCTX_ALL,
      IID_PPV_ARGS(&deviceEnumerator));
  if (result != S_OK) {
    throw std::runtime_error("CoCreateInstance(IMMDeviceEnumerator) failed");
  }
  const auto deallocDeviceEnumerator = finally([&] { deviceEnumerator->Release(); });

  IMMDevice *device;
  result = deviceEnumerator->GetDefaultAudioEndpoint(EDataFlow::eRender, ERole::eMultimedia, &device);
  if (result != S_OK) {
    throw std::runtime_error("GetDefaultAudioEndpoint() failed");
  }
  const auto deallocDevice = finally([&] { device->Release(); });

  IAudioEndpointVolume *deviceVolume;
  result = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, reinterpret_cast<void **>(&deviceVolume));
  if (result != S_OK) {
    throw std::runtime_error("Activate(IAudioEndpointVolume) failed");
  }
  const auto deallocDeviceVolume = finally([&] { deviceVolume->Release(); });

  result = deviceVolume->SetMasterVolumeLevelScalar(globalVolume, nullptr);
  if (result != S_OK) {
    throw std::runtime_error("SetMasterVolumeLevel() failed");
  }

  IAudioSessionManager2 *sessionMgr;
  result = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, reinterpret_cast<void **>(&sessionMgr));
  if (result != S_OK) {
    throw std::runtime_error("Activate(IAudioSessionManager) failed");
  }
  const auto deallocSessionMgr = finally([&] { sessionMgr->Release(); });

  IAudioSessionEnumerator *sessionEnum;
  result = sessionMgr->GetSessionEnumerator(&sessionEnum);
  if (result != S_OK) {
    throw std::runtime_error("GetSessionEnumerator() failed");
  }
  int numSessions;
  result = sessionEnum->GetCount(&numSessions);
  if (result != S_OK) {
    throw std::runtime_error("sessionEnum->GetCount() failed");
  }
  std::wcout << "Found " << numSessions << " audio sessions\n";

  for (int i = 0; i < numSessions; ++i) {
    IAudioSessionControl *sessionCtrl;
    result = sessionEnum->GetSession(i, &sessionCtrl);
    if (result != S_OK) {
      std::wcerr << "Failed to get session " << i << '\n';
      continue;
    }
    const auto deallocSessionCtrl = finally([&] { sessionCtrl->Release(); });

    IAudioSessionControl2 *sessionCtrl2;
    result = sessionCtrl->QueryInterface(IID_PPV_ARGS(&sessionCtrl2));
    if (result != S_OK) {
      std::wcerr << "Failed to get SessionControl2 " << i << '\n';
      continue;
    }

    // This will fail for the overall system process, which is fine.
    // I personally have system sounds always set to zero.
    DWORD pid;
    result = sessionCtrl2->GetProcessId(&pid);
    if (result != S_OK) {
      std::wcerr << "Failed to get the PID the audio session " << i << '\n';
      continue;
    }

    const auto procHnd = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid);
    if (procHnd == nullptr) {
      std::wcerr << "Failed to open process with PID " << pid << " for audio session " << i << '\n';
      continue;
    }
    const auto deallocProcHnd = finally([&] { ::CloseHandle(procHnd); });

    // Don't bother with display name since applications don't always set it.
    std::wstring procName(256, '\0');
    auto procNameSize{static_cast<DWORD>(procName.size())};
    if (0 == QueryFullProcessImageNameW(procHnd, 0, procName.data(), &procNameSize)) {
      std::wcerr << "Failed to get process name for PID " << pid << " for audio session " << i << '\n';
      continue;
    }
    procName.resize(procNameSize);

    std::wcout << "Session " << i << ": " << procName << '\n';

    for (const auto &entry: volumes) {
      if (!procName.ends_with(entry.first))
        continue;

      ISimpleAudioVolume *volume;
      result = sessionCtrl->QueryInterface(IID_PPV_ARGS(&volume));
      if (result != S_OK) {
        std::wcerr << "Failed to get AudioEndpointControl " << i << '\n';
        continue;
      }
      const auto deallocVolume = finally([&] { volume->Release(); });

      volume->SetMasterVolume(entry.second, nullptr);
      std::wcout << "Set volume of " << procName << " to " << entry.second << '\n';
    }
  }

  return 0;
} catch (const std::runtime_error &err) {
  std::wcerr << "Unhandled exception " << err.what() << '\n';
}
