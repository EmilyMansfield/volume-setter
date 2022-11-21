#include "declvol/profile.h"

#include <toml.hpp>

#include <format>
#include <stdexcept>

namespace em {

ProfileError::ProfileError(const std::filesystem::path &profilePath,
                           std::string_view context)
    : ProfileError(std::format("[error] Could not read profile file at {}\n{}",
                               profilePath.string(), context)) {}

VolumeControl::VolumeControl(std::string suffix, float relativeVolume)
    : mSuffix{std::move(suffix)}, mRelativeVolume{relativeVolume} {
  if (mRelativeVolume < 0.0f || mRelativeVolume > 1.0f) {
    throw std::invalid_argument(std::format(
        "Volume {} is out of range [0.0, 1.0]", mRelativeVolume));
  }
}

std::map<std::string, em::VolumeProfile>
parse_profiles_toml(const std::filesystem::path &profilePath) try {
  const auto data{toml::parse(profilePath)};

  std::map<std::string, em::VolumeProfile> profiles;
  for (const auto &section : data.as_table()) {
    em::VolumeProfile profile{};

    const auto controls{toml::find(section.second, "controls").as_array()};
    for (const auto &entry : controls) {
      auto suffix{toml::find<std::string>(entry, "suffix")};
      const auto &volumeObj{toml::find(entry, "volume")};
      const auto volume{toml::get<float>(volumeObj)};

      try {
        profile.controls.emplace_back(em::VolumeControl{std::move(suffix), volume});
      } catch (const std::invalid_argument &e) {
        throw ProfileError(std::format(
            "[error] Could not read profile at {}\n{}",
            profilePath.string(),
            toml::format_error(e.what(), volumeObj, "volume must be in range")));
      }
    }

    profiles.try_emplace(section.first, std::move(profile));
  }

  return profiles;
} catch (const toml::exception &e) {
  throw ProfileError(profilePath, e.what());
} catch (const std::out_of_range &e) {
  throw ProfileError(profilePath, e.what());
} catch (const std::runtime_error &e) {
  throw ProfileError(std::format(
      "[error] Could not read profile at {}\n[error] {}",
      profilePath.string(), e.what()));
}

}// namespace em
