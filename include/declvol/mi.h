#ifndef VOLUME_SETTER_INCLUDE_DECLVOL_MI_H
#define VOLUME_SETTER_INCLUDE_DECLVOL_MI_H

#include "declvol/windows.h"

#include <mi.h>

namespace em::mi {

/**
 * Result of an MI API call.
 *
 * Follows the semantics of `winrt::hresult`.
 */
struct miresult {
  MI_Result value{};

  constexpr miresult() noexcept = default;
  // NOLINTNEXTLINE "google-explicit-constructor"
  constexpr explicit(false) miresult(MI_Result v) noexcept : value{v} {}
  // NOLINTNEXTLINE "google-explicit-constructor"
  constexpr explicit(false) operator MI_Result() const noexcept { return value; }
};

/**
 * Exception containing an `miresult` error.
 *
 * This was going to follow `winrt::hresult_error`, but the way in which
 * extended error information is obtained is different. For now, this class
 * does not support extended error information.
 */
class miresult_error : public std::exception {
public:
  explicit miresult_error(miresult result) : mResult{result} {}

  ~miresult_error() override = default;

  [[nodiscard]] const char *what() const noexcept override;
  [[nodiscard]] miresult code() const noexcept { return mResult; }

private:
  miresult mResult;
};

/**
 * Throw the given `miresult` as an exception.
 */
[[noreturn]] inline void throw_miresult(miresult result) {
  throw miresult_error(result);
}

/**
 * If the given `miresult` represents an error, throw it as an exception,
 * otherwise return it unchanged.
 */
miresult inline check_miresult(miresult result) {
  if (result != MI_RESULT_OK) {
    throw_miresult(result);
  }
  return result;
}

/**
 * Protocol to use for MI sessions.
 */
enum class SessionProtocol {
  DCOM,
  WINRM,
};

/**
 * Language to use for MI queries.
 */
enum class QueryDialect {
  WQL,
  CQL,
};

namespace detail {

/**
 * Base class for MI class wrappers. Simplifies default initialization and
 * provides move semantics for MI classes.
 *
 * Users should implement the destructor to call the appropriate cleanup
 * function for the wrapped `T`. The API does not say anything one way or
 * another about calling those functions on 'null' `T`, but since the
 * implementations are in the header we can see that they will return an error
 * but otherwise will be a no-op, as desired. Given that nothing else is said,
 * perhaps we can consider the implementation as part of the contract.
 *
 * @tparam T MI class type to wrap, e.g. MI_Application.
 * @tparam Init An object whose value is the 'null' value required to be
 *  assigned to  a `T` before it is initialized by an appropriate function.
 *  In practice this just zero-initializes, but since in the future it could
 *  change we'll be a good user this time and do as we're told.
 */
template<class T, T Init>
class MIBase {
public:
  MIBase() = default;

  MIBase(const MIBase &) = delete;
  MIBase &operator=(const MIBase &) = delete;

  MIBase(MIBase &&other) noexcept
      : mImpl(std::exchange(other.mImpl, T(Init))) {}

  MIBase &operator=(MIBase &&other) noexcept {
    mImpl = std::exchange(other.mImpl, T(Init));
    return *this;
  }

  [[nodiscard]] T *get() noexcept { return &mImpl; }
  [[nodiscard]] const T *get() const noexcept { return &mImpl; }

private:
  T mImpl = Init;
};

}// namespace detail

class Operation;
class Session;
class SubscriptionOptions;

/**
 * RAII wrapper around `MI_Application`.
 */
class Application : public detail::MIBase<MI_Application, MI_Application(MI_APPLICATION_NULL)> {
public:
  explicit Application(const MI_Char *applicationId);
  ~Application();

  Application() = default;
  Application(Application &&) noexcept = default;
  Application &operator=(Application &&) noexcept = default;

  Session local_session(SessionProtocol protocol, MI_SessionCallbacks *callbacks);
  SubscriptionOptions make_subscription_options(MI_SubscriptionDeliveryType deliveryType);
};

/**
 * RAII wrapper around `MI_Session`.
 *
 * This implementation does not use the completion callback in its destructor so
 * is not safe to be destructed from an async callback.
 */
class Session : public detail::MIBase<MI_Session, MI_Session(MI_APPLICATION_NULL)> {
public:
  ~Session();

  Session() = default;
  Session(Session &&) noexcept = default;
  Session &operator=(Session &&) noexcept = default;

  /**
   * Subscribe to events in the given namespace matching a query string.
   */
  Operation subscribe(MI_OperationOptions *options,
                      const MI_Char *namespaceName,
                      QueryDialect dialect,
                      const MI_Char *query,
                      const MI_SubscriptionDeliveryOptions *deliveryOptions,
                      MI_OperationCallbacks *callbacks);
};

/**
 * RAII wrapper around `MI_SubscriptionDeliveryOptions`.
 */
class SubscriptionOptions : public detail::MIBase<
                                MI_SubscriptionDeliveryOptions,
                                MI_SubscriptionDeliveryOptions(MI_SUBSCRIPTIONDELIVERYOPTIONS_NULL)> {
public:
  ~SubscriptionOptions();

  SubscriptionOptions() = default;
  SubscriptionOptions(SubscriptionOptions &&) noexcept = default;
  SubscriptionOptions &operator=(SubscriptionOptions &&) noexcept = default;
};

/**
 * RAII wrapper around `MI_Operation`.
 */
class Operation : public detail::MIBase<MI_Operation, MI_Operation(MI_OPERATION_NULL)> {
public:
  ~Operation();

  Operation() = default;
  Operation(Operation &&) noexcept = default;
  Operation &operator=(Operation &&) noexcept = default;

  void cancel(MI_CancellationReason reason);
};

}// namespace em::mi

#endif// VOLUME_SETTER_INCLUDE_DECLVOL_MI_H
