#ifndef VOLUME_SETTER_INCLUDE_DECLVOL_CONFIG_H
#define VOLUME_SETTER_INCLUDE_DECLVOL_CONFIG_H

#include <filesystem>

namespace em {

/**
 * Return the path of the current user's local app data folder.
 *
 * On Windows this is the user's `LocalAppData` folder, which usually
 * corresponds to the value of the environment variable `LOCALAPPDATA`.
 */
std::filesystem::path local_app_data();

/**
 * Return the path of the default configuration file.
 */
std::filesystem::path get_default_config_path();

}// namespace em

#endif// VOLUME_SETTER_INCLUDE_DECLVOL_CONFIG_H
