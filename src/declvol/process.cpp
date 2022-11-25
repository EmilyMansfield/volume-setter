#include "declvol/process.h"

namespace em {

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

winrt::handle open_process(DWORD pid) {
  winrt::handle hnd{::OpenProcess(
      PROCESS_QUERY_LIMITED_INFORMATION, /*bInheritHandle=*/false, pid)};
  if (!hnd) winrt::throw_last_error();
  return hnd;
}

}// namespace em
