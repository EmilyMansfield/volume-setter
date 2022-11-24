#ifndef VOLUME_SETTER_INCLUDE_DECLVOL_MI_H
#define VOLUME_SETTER_INCLUDE_DECLVOL_MI_H

#include "declvol/windows.h"

#include <functional>
#include <mi.h>

namespace em::mi {

/**
 * Result of an MI API call.
 *
 * Follows the semantics of `winrt::hresult`, except that it has an additional
 * conversion to `bool`.
 */
struct miresult {
  MI_Result value{};

  constexpr miresult() noexcept = default;

  // NOLINTNEXTLINE "google-explicit-constructor"
  constexpr explicit(false) miresult(MI_Result v) noexcept : value{v} {}
  // NOLINTNEXTLINE "google-explicit-constructor"
  constexpr explicit(false) operator MI_Result() const noexcept { return value; }

  /**
   * Return whether the result represents success.
   */
  constexpr explicit operator bool() const noexcept { return value == MI_RESULT_OK; }
};

/**
 * Exception containing an `miresult` error.
 *
 * This was going to follow `winrt::hresult_error`, but the way in which
 * extended error information is obtained is different. It is not possible to
 * take an `MI_Result` and convert it to an exception with additional
 * information like with `winrt::hresult_error and `winrt::check_hresult`
 * because the additional information is conveyed through out-parameters.
 *
 * Additionally, the particular out-parameters that are used are not consistent;
 * some functions have no such parameters, others return an `MI_Instance`,
 * others return an `MI_Instance` and a string, and the positions in the
 * parameter list can differ. Any of them may be null. With user cooperation
 * or lots of wrapper code this can be worked around, but the `MI_Instance`s
 * are not even straightforward to work with.
 *
 * The documentation is very sparse regarding the class of the error objects,
 * but it suggests that they are usually a (subclass of) `CIM_Error`. This
 * class is defined by the CIM standard and its description available in a
 * MOF file, as well as in the docs, but does not seem to be available as
 * source code. Microsoft's documentation for creating an MI provider states
 * that the `convert-moftoprovider.exe` tool can convert MOF files to C source,
 * but it doesn't appear to be included in the Windows 10 SDK available from
 * Visual Studio Build Tools. There's also `mofcomp.exe`, but I'm very hesitant
 * to try this tool with an existing MOF file as it appears to modify the
 * registry and is intended for user-created MOF files. The `CIM_Error` class
 * can be obtained at runtime, but this is quite complicated, especially when
 * doing it in an exception class.
 */
class miresult_error : public std::exception {
public:
  /**
   * Construct an exception representing an MI error, using the default error
   * string for that error code.
   */
  explicit miresult_error(miresult result) : mResult{result}, mErrString{} {}
  /**
   * Construct an exception representing an MI error with a custom error string.
   */
  explicit miresult_error(miresult result, const MI_Char *errorString);

  ~miresult_error() override = default;

  [[nodiscard]] const char *what() const noexcept override;
  [[nodiscard]] miresult code() const noexcept { return mResult; }

private:
  miresult mResult;
  // Need to store an error string for the extended error information because
  // the std::exception API requires us to return a const char * and we are
  // given a const wchar_t * whose contents need to be converted and whose
  // lifetime must outlast the call to what().
  std::shared_ptr<std::string> mErrString;
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
 * Users should pass as the `Deleter` a callable to call the appropriate cleanup
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
 * @tparam Deleter Callable that takes a pointer to the stored `T` and deletes
 *  it using the appropriate cleanup function.
 */
template<class T, T Init, auto Deleter>
class MIBase {
public:
  MIBase() = default;
  ~MIBase() { Deleter(get()); }

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
class SubscribeOperation;

/**
 * RAII wrapper around `MI_Application`.
 */
class Application : public detail::MIBase<
                        MI_Application,
                        MI_Application(MI_APPLICATION_NULL),
                        [](MI_Application *p) { ::MI_Application_Close(p); }> {
public:
  explicit Application(const MI_Char *applicationId);
  Application() = default;

  Session local_session(SessionProtocol protocol, MI_SessionCallbacks *callbacks);
  SubscriptionOptions make_subscription_options(MI_SubscriptionDeliveryType deliveryType);
};

using SubscriptionCallback = std::move_only_function<void(const MI_Instance *instance, miresult result, const MI_Char *errorString)>;

/**
 * RAII wrapper around `MI_Session`.
 *
 * This implementation does not use the completion callback in its destructor so
 * is not safe to be destructed from an async callback.
 */
class Session : public detail::MIBase<
                    MI_Session,
                    MI_Session(MI_APPLICATION_NULL),
                    [](MI_Session *p) { ::MI_Session_Close(p, nullptr, nullptr); }> {
public:
  /**
   * Subscribe to events in the given namespace matching a query string.
   */
  Operation subscribe(MI_OperationOptions *options,
                      const MI_Char *namespaceName,
                      QueryDialect dialect,
                      const MI_Char *query,
                      const MI_SubscriptionDeliveryOptions *deliveryOptions,
                      MI_OperationCallbacks *callbacks);

  SubscribeOperation subscribe(MI_OperationOptions *options,
                               const MI_Char *namespaceName,
                               QueryDialect dialect,
                               const MI_Char *query,
                               const SubscriptionOptions &subOptions,
                               SubscriptionCallback callback);
};

/**
 * RAII wrapper around `MI_SubscriptionDeliveryOptions`.
 */
class SubscriptionOptions : public detail::MIBase<
                                MI_SubscriptionDeliveryOptions,
                                MI_SubscriptionDeliveryOptions(MI_SUBSCRIPTIONDELIVERYOPTIONS_NULL),
                                [](MI_SubscriptionDeliveryOptions *p) { ::MI_SubscriptionDeliveryOptions_Delete(p); }> {
public:
};

/**
 * RAII wrapper around `MI_Operation`.
 */
class Operation : public detail::MIBase<
                      MI_Operation,
                      MI_Operation(MI_OPERATION_NULL),
                      [](MI_Operation *p) { ::MI_Operation_Close(p); }> {
public:
  void cancel(MI_CancellationReason reason);
};

/**
 * RAII wrapper around a subscribe operation.
 */
class SubscribeOperation {
public:
  friend class Session;

  SubscribeOperation(const SubscribeOperation &) = delete;
  SubscribeOperation(SubscribeOperation &&) = delete;

  void cancel(MI_CancellationReason reason) { return mOp.cancel(reason); };

private:
  explicit SubscribeOperation(Session &session,
                              MI_OperationOptions *options,
                              const MI_Char *namespaceName,
                              QueryDialect dialect,
                              const MI_Char *query,
                              const SubscriptionOptions &subOptions,
                              SubscriptionCallback callback);

  SubscriptionCallback mCallback;
  Operation mOp;
};

}// namespace em::mi

#endif// VOLUME_SETTER_INCLUDE_DECLVOL_MI_H
