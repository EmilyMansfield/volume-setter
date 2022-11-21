#ifndef VOLUME_SETTER_INCLUDE_DECLVOL_EXCEPTION_H
#define VOLUME_SETTER_INCLUDE_DECLVOL_EXCEPTION_H

#include <exception>
#include <string>

namespace em {

/**
 * Base class of all exceptions thrown by this application.
 *
 * \remark Doesn't worry about being nothrow copy-constructible because the
 *         libraries that we depend on don't. It's only a requirement for
 *         standard library exceptions anyway. In practice, throwing in the
 *         copy constructor does not necessarily call `std::terminate`, and
 *         frankly this application isn't important enough to fret about it.
 */
class VolumeException : public std::exception {
public:
  explicit VolumeException(std::string msg) : mMsg{std::move(msg)} {}

  [[nodiscard]] const char *what() const noexcept override {
    return mMsg.c_str();
  }

private:
  std::string mMsg;
};

}// namespace em

#endif// VOLUME_SETTER_INCLUDE_DECLVOL_EXCEPTION_H
