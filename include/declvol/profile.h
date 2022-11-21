#ifndef VOLUME_SETTER_INCLUDE_DECLVOL_PROFILE_H
#define VOLUME_SETTER_INCLUDE_DECLVOL_PROFILE_H

#include "declvol/exception.h"

#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace em {

/**
 * Type of errors that occur when reading volume profiles.
 */
class ProfileError : public VolumeException {
public:
  explicit ProfileError(const std::string &msg) : VolumeException(msg) {}

  explicit ProfileError(const std::filesystem::path &profilePath,
                        std::string_view context);
};

class VolumeControl {
public:
  explicit VolumeControl(std::string suffix, float relativeVolume);

  VolumeControl(const VolumeControl &) = default;
  VolumeControl &operator=(const VolumeControl &) = default;
  VolumeControl(VolumeControl &&) noexcept = default;
  VolumeControl &operator=(VolumeControl &&) noexcept = default;

  [[nodiscard]] const std::string &suffix() const noexcept {
    return mSuffix;
  }

  [[nodiscard]] float relative_volume() const noexcept {
    return mRelativeVolume;
  }

private:
  std::string mSuffix;
  float mRelativeVolume;
};

struct VolumeProfile {
  std::vector<VolumeControl> controls;
};

/**
 * Return the volume profiles defined by a TOML configuration file.
 *
 * \throws ProfileError if the profile cannot be read.
 */
std::map<std::string, em::VolumeProfile>
parse_profiles_toml(const std::filesystem::path &profilePath);

}// namespace em

#endif// VOLUME_SETTER_INCLUDE_DECLVOL_PROFILE_H
