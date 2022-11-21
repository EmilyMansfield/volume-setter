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

}// namespace em
