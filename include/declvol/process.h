#ifndef VOLUME_SETTER_INCLUDE_DECLVOL_PROCESS_H
#define VOLUME_SETTER_INCLUDE_DECLVOL_PROCESS_H

#include "declvol/windows.h"

#include <string>

namespace em {

/**
 * Return the full executable name of the given process.
 */
std::string get_process_image_name(const winrt::handle &processHandle);

/**
 * Return a handle to the given process with the
 * `PROCESS_QUERY_LIMITED_INFORMATION` access right.
 */
winrt::handle open_process(DWORD pid);

}// namespace em

#endif// VOLUME_SETTER_INCLUDE_DECLVOL_PROCESS_H
