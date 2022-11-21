#include "declvol/config.h"
#include "declvol/windows.h"

#include <ShlObj_core.h>

namespace em {

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

std::filesystem::path get_default_config_path() {
  return em::local_app_data() / "volume-setter" / "config.toml";
}

}// namespace em
