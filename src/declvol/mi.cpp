#include "declvol/mi.h"

namespace em::mi {

miresult_error::miresult_error(miresult result, const MI_Char *errorString)
    : miresult_error(result) {
  if (!errorString) return;
  mErrString = std::make_shared<std::string>(winrt::to_string(winrt::hstring{errorString}));
}

[[nodiscard]] const char *miresult_error::what() const noexcept {
  if (mErrString) return mErrString->c_str();

  // Descriptions are from the documentation
  // https://learn.microsoft.com/en-us/windows/win32/api/mi/ne-mi-mi_result
  switch (mResult) {
  case MI_RESULT_OK:
    return "The operation was successful";
  case MI_RESULT_FAILED:
    return "A general error occurred, not covered by a more specific error code";
  case MI_RESULT_ACCESS_DENIED:
    return "Access to a CIM resource is not available to the client";
  case MI_RESULT_INVALID_NAMESPACE:
    return "The target namespace does not exist";
  case MI_RESULT_INVALID_PARAMETER:
    return "One or more parameter values passed to the method are not valid";
  case MI_RESULT_INVALID_CLASS:
    return "The specified class does not exist";
  case MI_RESULT_NOT_FOUND:
    return "The requested object cannot be found";
  case MI_RESULT_NOT_SUPPORTED:
    return "The requested operation is not supported";
  case MI_RESULT_CLASS_HAS_CHILDREN:
    return "The operation cannot be invoked because the class has no subclass";
  case MI_RESULT_CLASS_HAS_INSTANCES:
    return "The operation cannot be invoked because an object already exists";
  case MI_RESULT_INVALID_SUPERCLASS:
    return "The operation cannot be invoked because the superclass does not exist";
  case MI_RESULT_ALREADY_EXISTS:
    return "The operation cannot be invoked because an object already exists";
  case MI_RESULT_NO_SUCH_PROPERTY:
    return "The specified property does not exist";
  case MI_RESULT_TYPE_MISMATCH:
    return "The value supplied is not compatible with the type";
  case MI_RESULT_QUERY_LANGUAGE_NOT_SUPPORTED:
    return "The query language is not recognized or supported";
  case MI_RESULT_INVALID_QUERY:
    return "The query is not valid for the specified query language";
  case MI_RESULT_METHOD_NOT_AVAILABLE:
    return "The extrinsic method cannot be invoked";
  case MI_RESULT_METHOD_NOT_FOUND:
    return "The specified extrinsic method does not exist";
  case MI_RESULT_NAMESPACE_NOT_EMPTY:
    return "The specified namespace is not empty";
  case MI_RESULT_INVALID_ENUMERATION_CONTEXT:
    return "The enumeration identified by the specified context is not valid";
  case MI_RESULT_INVALID_OPERATION_TIMEOUT:
    return "The specified operation timeout is not supported by the CIM server";
  case MI_RESULT_PULL_HAS_BEEN_ABANDONED:
    return "The pull operation has been abandoned";
  case MI_RESULT_PULL_CANNOT_BE_ABANDONED:
    return "The attempt to abandon a concurrent pull request failed";
  case MI_RESULT_FILTERED_ENUMERATION_NOT_SUPPORTED:
    return "Using a filter in the enumeration is not supported by the CIM server";
  case MI_RESULT_CONTINUATION_ON_ERROR_NOT_SUPPORTED:
    return "The CIM server does not support continuation on error";
  case MI_RESULT_SERVER_LIMITS_EXCEEDED:
    return "The operation failed because server limits were exceeded";
  case MI_RESULT_SERVER_IS_SHUTTING_DOWN:
    return "The CIM server is shutting down and cannot process the operation";
  }
  return "An unknown MI_Result occurred";
}

Application::Application(const MI_Char *applicationId) {
  mi::check_miresult(MI_Application_Initialize(0, applicationId, nullptr, get()));
}

Session Application::local_session(SessionProtocol protocol, MI_SessionCallbacks *callbacks) {
  const MI_Char *miProtocol = [protocol] {
    switch (protocol) {
    case SessionProtocol::DCOM: return L"WMIDCOM";
    case SessionProtocol::WINRM: return L"WINRM";
    }
    throw std::invalid_argument("Specified SessionProtocol is not supported");
  }();

  Session sess;
  mi::check_miresult(MI_Application_NewSession(
      get(), miProtocol, nullptr, nullptr, callbacks, nullptr, sess.get()));
  return sess;
}

SubscribeOperation Session::subscribe(
    MI_OperationOptions *options,
    const MI_Char *namespaceName,
    QueryDialect dialect,
    const MI_Char *query,
    const SubscriptionOptions &subOptions,
    SubscriptionCallback callback) {
  return SubscribeOperation(*this, options, namespaceName, dialect, query, subOptions, std::move(callback));
}

SubscriptionOptions Application::make_subscription_options(MI_SubscriptionDeliveryType deliveryType) {
  SubscriptionOptions opts;
  mi::check_miresult(::MI_Application_NewSubscriptionDeliveryOptions(get(), deliveryType, opts.get()));
  return opts;
}

Operation Session::subscribe(MI_OperationOptions *options,
                             const MI_Char *namespaceName,
                             QueryDialect dialect,
                             const MI_Char *query,
                             const MI_SubscriptionDeliveryOptions *deliveryOptions,
                             MI_OperationCallbacks *callbacks) {
  const MI_Char *miDialect = [dialect] {
    switch (dialect) {
    case QueryDialect::WQL: return L"WQL";
    case QueryDialect::CQL: return L"CQL";
    }
    throw std::invalid_argument("Specified QueryDialect is not supported");
  }();

  Operation op;
  ::MI_Session_Subscribe(get(), 0, options, namespaceName, miDialect, query,
                         deliveryOptions, callbacks, op.get());
  return op;
}

void Operation::cancel(MI_CancellationReason reason) {
  mi::check_miresult(::MI_Operation_Cancel(get(), reason));
}

SubscribeOperation::SubscribeOperation(Session &session,
                                       MI_OperationOptions *options,
                                       const MI_Char *namespaceName,
                                       QueryDialect dialect,
                                       const MI_Char *query,
                                       const SubscriptionOptions &subOptions,
                                       SubscriptionCallback callback) : mCallback{std::move(callback)} {
  MI_OperationCallbacks callbackTable{
      .callbackContext = &mCallback,
      .indicationResult = [](MI_Operation *op,
                             void *rawCtx,
                             const MI_Instance *instance,
                             const MI_Char * /*bookmark*/,
                             const MI_Char * /*machineId*/,
                             MI_Boolean /*moreResults*/,
                             MI_Result resultCode,
                             const MI_Char *errorString,
                             const MI_Instance * /*errorDetails*/,
                             MI_Result(MI_CALL * ack)(MI_Operation * op)) {
        // TODO: Use gsl::finally or similar
        // This is in case the callback throws an exception and doesn't catch
        // it. If somehow that works, we still need to call ack because
        // otherwise the operation will not be able to stop and the service
        // will deadlock if it hasn't already crashed. I'm not sure what the
        // right thing to do here is.
        struct final_ack_ {
          MI_Result(MI_CALL *ack)(MI_Operation *);
          MI_Operation *op;
          ~final_ack_() { (ack && op) ? ack(op) : MI_Result{}; }
        } final_ack{ack, op};
        static_cast<SubscriptionCallback *>(rawCtx)->operator()(instance, resultCode, errorString);
      },
  };
  mOp = session.subscribe(options, namespaceName, dialect, query, subOptions.get(), &callbackTable);
}

}// namespace em::mi
