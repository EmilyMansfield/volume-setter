#include "declvol/exception.h"
#include "declvol/mi.h"
#include "declvol/profile.h"
#include "declvol/windows.h"

#include <argparse/argparse.hpp>

#include <atomic>
#include <format>
#include <fstream>
#include <syncstream>
#include <variant>

namespace em {
namespace {

constexpr inline std::string_view ServiceName = EM_SERVICE_NAME;
constexpr inline std::string_view ServiceVersion = EM_SERVICE_VERSION;

// Restrictions on the ServiceName imposed by CreateServiceA.
static_assert(ServiceName.size() <= 256);
static_assert(!ServiceName.contains('/'));
static_assert(!ServiceName.contains('\\'));

using namespace std::chrono_literals;

/**
 * Main service task to be launched by the Service Control Manager.
 */
void WINAPI run_service(DWORD argc, LPSTR argv[]);

/**
 * `winrt::handle` support for `SC_HANDLE`.
 **/
struct sc_handle_traits {
  using type = SC_HANDLE;
  static void close(type value) noexcept { ::CloseServiceHandle(value); }
  static type invalid() noexcept { return nullptr; }
};

/**
 * A service handle object.
 */
using sc_handle = winrt::handle_type<sc_handle_traits>;

/**
 * `winrt::handle` support for `SERVICE_STATUS_HANDLE`.
 */
struct service_status_handle_traits {
  using type = SERVICE_STATUS_HANDLE;
  static void close(type) noexcept {}
  static type invalid() noexcept { return nullptr; }
};

/**
 * A service status handle object.
 */
using service_status_handle = winrt::handle_type<service_status_handle_traits>;

/**
 * Return the full executable name of the current process.
 */
std::string get_current_executable_name() {
  // MAX_PATH includes the null-terminator, and it's allowed to write to the
  // null-terminator of std::string so long as the written value is a null.
  std::string path(MAX_PATH - 1, '\0');
  const auto size{::GetModuleFileNameA(nullptr, path.data(), MAX_PATH)};
  if (size == 0 || size == MAX_PATH) winrt::throw_last_error();
  path.resize(size);
  return path;
}

/**
 * Install the service to run as the LocalSystem user on-demand.
 */
int install_service() {
  const sc_handle scm{::OpenSCManagerA(nullptr, nullptr,
                                       SC_MANAGER_CREATE_SERVICE | SC_MANAGER_CONNECT)};
  if (!scm) winrt::throw_last_error();

  // Paths cannot contain " so there's nothing to escape.
  const auto quotedPath{std::format("\"{}\"", em::get_current_executable_name())};
  const sc_handle sc{::CreateServiceA(scm.get(),
                                      em::ServiceName.data(),
                                      em::ServiceName.data(),
                                      SERVICE_ALL_ACCESS,
                                      SERVICE_WIN32_OWN_PROCESS,
                                      SERVICE_DEMAND_START,
                                      SERVICE_ERROR_NORMAL,
                                      quotedPath.c_str(),
                                      /*lpLoadOrderGroup=*/nullptr,
                                      /*lpdwTagId=*/nullptr,
                                      /*lpDependencies=*/nullptr,
                                      /*lpServiceStartName=*/nullptr,
                                      /*lpPassword=*/nullptr)};
  if (!sc) winrt::throw_last_error();

  std::cout << "Installed service\n";

  return 0;
}

/**
 * Start the service dispatcher and block until the service process has been
 * stopped.
 *
 * The dispatcher listens for requests from the Service Control Manager to
 * start or stop a service implemented by the service process. In this case,
 * that is just the volume control service implemented by em::run_service.
 */
int start_service_dispatcher() {
  // Table requires a pointer to non-const char. Incidentally this also forces
  // the required null-termination which a string_view alone doesn't guarantee,
  // though currently it's fine due to how it's initialized, of course.
  std::string windowsServiceName{em::ServiceName};
  const std::array dispatchTable{
      SERVICE_TABLE_ENTRY{windowsServiceName.data(), em::run_service},
      SERVICE_TABLE_ENTRY{nullptr, nullptr},
  };

  // This blocks until the service is stopped.
  winrt::check_bool(::StartServiceCtrlDispatcherA(dispatchTable.data()));
  return 0;
}

/**
 * Strong typedef for service states used to notify the Service Control Manager
 * of the current state of the service.
 *
 * Enumerator values must match their corresponding define so that they can be
 * passed directly to Windows API functions with `std::to_underlying`.
 */
enum class ServiceStateId : std::uint32_t {
  ContinuePending = SERVICE_CONTINUE_PENDING,
  PausePending = SERVICE_PAUSE_PENDING,
  Paused = SERVICE_PAUSED,
  Started = SERVICE_RUNNING,
  StartPending = SERVICE_START_PENDING,
  StopPending = SERVICE_STOP_PENDING,
  Stopped = SERVICE_STOPPED
};

/**
 * Base class for service states.
 */
template<ServiceStateId S>
struct ServiceStateBase {
  /**
   * The state ID used to report the state to the Service Control Manager.
   */
  constexpr static inline ServiceStateId StateId = S;
};

/**
 * Convenience typedef for handling wait durations that are reported to the
 * Service Control Manager.
 */
using dmilliseconds = std::chrono::duration<DWORD, std::ratio<1, 1000>>;

/**
 * State in the state machine used to control the startup and shutdown behaviour
 * of the service.
 */
template<class T>
concept service_state =
    requires {
      /**
       * State ID that will be reported to the Service Control Manager when in
       * this state.
       */
      { T::StateId } -> std::convertible_to<ServiceStateId>;
      /**
       * Controls issued by the Service Control Manager that will be accepted
       * by the service when in this state.
       */
      { T::ControlsAccepted } -> std::convertible_to<DWORD>;
    };

/**
 * A service state in which an operation is pending and may make progress before
 * transitioning to a non-pending service state.
 */
template<class T>
concept service_pending_state = service_state<T>
    && requires(T v) {
         /**
          * Integer checkpoint that can be incremented to indicate to the
          * Service Control Manager that progress is being made.
          */
         { v.checkpoint } -> std::convertible_to<DWORD>;
         /**
          * Estimated time that the service will remain in this pending state.
          */
         { v.timeRemaining } -> std::convertible_to<dmilliseconds>;
       };

/**
 * A terminal service state, namely one that can only transition to itself.
 */
template<class T>
concept service_terminal_state = service_state<T>
    && requires(T v) {
         /**
          * Exit code to report to the Service Control Manager.
          */
         { v.exitCode } -> std::convertible_to<DWORD>;
       };

/**
 * Service state in which the service is starting.
 */
struct ServiceStateStartPending : ServiceStateBase<ServiceStateId::StartPending> {
  constexpr static inline DWORD ControlsAccepted = 0;

  DWORD checkpoint{};
  dmilliseconds timeRemaining{};
};

/**
 * Service state in which the service has started and is fully operational.
 */
struct ServiceStateStarted : ServiceStateBase<ServiceStateId::Started> {
  constexpr static inline DWORD ControlsAccepted = SERVICE_ACCEPT_STOP;
};

/**
 * Service state in which the service is stopping.
 */
struct ServiceStateStopPending : ServiceStateBase<ServiceStateId::StopPending> {
  constexpr static inline DWORD ControlsAccepted = 0;

  DWORD checkpoint{};
  dmilliseconds timeRemaining{};
};

/**
 * Terminal service state in which the service has stopped, possibly with an
 * error.
 */
struct ServiceStateStopped : ServiceStateBase<ServiceStateId::Stopped> {
  constexpr static inline DWORD ControlsAccepted = 0;

  DWORD exitCode{NO_ERROR};
};

/**
 * Holder for an arbitrary service state.
 */
struct ServiceState : std::variant<ServiceStateStopped,
                                   ServiceStateStartPending,
                                   ServiceStateStarted,
                                   ServiceStateStopPending> {
  using variant::variant;

  [[nodiscard]] constexpr ServiceStateId state_id() const {
    return std::visit([]<class T>(T) { return T::StateId; }, *this);
  }

  [[nodiscard]] constexpr DWORD controls_accepted() const {
    return std::visit([]<class S>(S) { return S::ControlsAccepted; }, *this);
  }

  [[nodiscard]] constexpr bool is_stopped() const {
    return std::holds_alternative<ServiceStateStopped>(*this);
  }
};

/**
 * Context type storing the current service state and allowing the service
 * control handler to communicate with the actual service thread.
 */
class ServiceContext {
public:
  using State = ServiceState;

  ServiceContext() : mCurrentState{std::in_place_type<ServiceStateStopped>} {
    mOs = std::make_unique<std::ofstream>(std::filesystem::temp_directory_path() / "volume_setter.log");
  }

  [[nodiscard]] constexpr State current_state() const {
    return mCurrentState;
  }

  constexpr void transition(ServiceState newState) {
    mCurrentState = newState;
  }

  std::osyncstream os() {
    return std::osyncstream(*mOs);
  }

  /**
   * Lippincott function to print the active exception to the context's log.
   *
   * This function must be called inside of a catch-handler.
   */
  void log_active_exception() {
    try {
      throw;
    } catch (const em::ProfileError &e) {
      os() << e.what() << '\n';
    } catch (const std::exception &e) {
      os() << "Unhandled exception: " << e.what() << '\n';
    } catch (const winrt::hresult_error &e) {
      os() << "Unhandled exception: " << winrt::to_string(e.message()) << '\n';
    }
  }

  std::atomic_flag stopFlag;

  mi::Application miApp;
  mi::Session miSession;

private:
  State mCurrentState;
  std::unique_ptr<std::ostream> mOs;
};

/**
 * Return an object used to report the current service status to the Service
 * Control Manager.
 */
[[nodiscard]] constexpr SERVICE_STATUS service_status(ServiceState state);

/**
 * Run the given state and return a new state to transition to.
 */
[[nodiscard]] constexpr ServiceState run_state(ServiceState state, ServiceContext &ctx);

// Note: These _impl functions have a suffix to prevent accidental stack
//       overflow. The API idea is that overloads can be used to define
//       behaviour for each state, and then there's a single function that takes
//       an arbitrary state (variant) which dispatches to the appropriate
//       overload. If every concrete state has an overload (including those
//       added by function templates) then all is well, but if one is missing
//       then the variant overload will be matched, causing unbounded recursion.
//       Perhaps these should be operators on class templates, that way they'd
//       also work with specializations.

[[nodiscard]] constexpr SERVICE_STATUS service_status_impl(service_state auto state) {
  SERVICE_STATUS status{};
  status.dwServiceType = SERVICE_USER_OWN_PROCESS;
  status.dwCurrentState = std::to_underlying(decltype(state)::StateId);
  status.dwControlsAccepted = decltype(state)::ControlsAccepted;
  return status;
}

[[nodiscard]] constexpr SERVICE_STATUS service_status_impl(service_pending_state auto state) {
  SERVICE_STATUS status{};
  status.dwServiceType = SERVICE_USER_OWN_PROCESS;
  status.dwCurrentState = std::to_underlying(decltype(state)::StateId);
  status.dwControlsAccepted = decltype(state)::ControlsAccepted;
  status.dwCheckPoint = state.checkpoint;
  status.dwWaitHint = dmilliseconds{state.timeRemaining}.count();
  return status;
}

[[nodiscard]] constexpr SERVICE_STATUS service_status_impl(service_terminal_state auto state) {
  SERVICE_STATUS status{};
  status.dwServiceType = SERVICE_USER_OWN_PROCESS;
  status.dwCurrentState = std::to_underlying(decltype(state)::StateId);
  status.dwControlsAccepted = decltype(state)::ControlsAccepted;
  status.dwWin32ExitCode = state.exitCode;
  return status;
}

ServiceState run_state_impl(ServiceStateStartPending /*state*/, ServiceContext &ctx) {
  ctx.os() << "Status -> StartPending" << std::endl;
  ctx.os() << "TODO: Load the config file" << std::endl;

  ctx.miApp = mi::Application(nullptr);
  ctx.os() << "Created Application" << std::endl;
  ctx.miSession = ctx.miApp.local_session(mi::SessionProtocol::WINRM, nullptr);
  ctx.os() << "Created Session" << std::endl;

  return ServiceStateStarted{};
}

ServiceState run_state_impl(ServiceStateStarted /*state*/, ServiceContext &ctx) {
  ctx.os() << "Status -> Started" << std::endl;

  const auto options{ctx.miApp.make_subscription_options(MI_SubscriptionDeliveryType_Pull)};
  auto operation{ctx.miSession.subscribe(
      nullptr, L"Root\\CIMV2", mi::QueryDialect::WQL, L"SELECT * FROM Win32_ProcessStartTrace", options,
      [&ctx](const MI_Instance *instance, mi::miresult result, const MI_Char *errorString) {
        // No function try block for lambdas, so we lose an indentation level :(
        try {
          if (!result) throw mi::miresult_error(result, errorString);

          MI_Value value{};
          MI_Type type{};
          mi::check_miresult(::MI_Instance_GetElement(instance, L"ProcessName", &value, &type, nullptr, nullptr));
          if (type != MI_STRING) {
            ctx.os() << "Expected string type, received " << int{type} << std::endl;
            return;
          }

          const auto processName{winrt::hstring{value.string}};
          ctx.os() << "Process " << winrt::to_string(processName) << " started" << std::endl;
        } catch (...) {
          ctx.log_active_exception();
        }
      })};

  ctx.stopFlag.wait(false);
  ctx.os() << "Woken with stop notification" << std::endl;
  operation.cancel(MI_REASON_SERVICESTOP);
  ctx.os() << "Operation cancelled" << std::endl;
  return ServiceStateStopPending{};
}

ServiceState run_state_impl(ServiceStateStopPending /*state*/, ServiceContext &ctx) {
  ctx.os() << "Status -> StopPending" << std::endl;
  return ServiceStateStopped{};
}

ServiceState run_state_impl(ServiceStateStopped state, ServiceContext &ctx) {
  ctx.os() << "Status -> Stopped" << std::endl;
  return state;
}

[[nodiscard]] constexpr SERVICE_STATUS service_status(ServiceState state) {
  return std::visit([](auto s) { return em::service_status_impl(s); }, state);
}

[[nodiscard]] constexpr ServiceState run_state(ServiceState state, ServiceContext &ctx) {
  return std::visit([&ctx](auto s) { return em::run_state_impl(s, ctx); }, state);
}

/**
 * Service control handler function invoked by the Service Control Manager to
 * change the state of the service.
 */
DWORD WINAPI service_control_handler(DWORD ctrl, DWORD eventType, void *eventData, void *rawCtx) {
  auto *const ctx{static_cast<ServiceContext *>(rawCtx)};
  ctx->os() << "Received event: " << ctrl << ", " << eventType << ", " << eventData << std::endl;
  // Return codes are specified by the HandlerEx documentation.
  switch (ctrl) {
  case SERVICE_CONTROL_INTERROGATE: return NO_ERROR;
  case SERVICE_CONTROL_STOP: {
    ctx->stopFlag.test_and_set();
    ctx->stopFlag.notify_one();
    return NO_ERROR;
  }
  default: return ERROR_CALL_NOT_IMPLEMENTED;
  }
}

void WINAPI run_service(DWORD /*argc*/, LPSTR /*argv*/[]) {
  ServiceContext ctx{};
  ctx.os() << "run_service()" << std::endl;

  try {
    const em::service_status_handle svcHandle{::RegisterServiceCtrlHandlerExA(
        em::ServiceName.data(), em::service_control_handler, &ctx)};
    if (!svcHandle) winrt::throw_last_error();
    ctx.os() << "Registered handler" << std::endl;

    ctx.transition(ServiceStateStartPending{.checkpoint = 0, .timeRemaining = 1000ms});
    auto initialStatus{em::service_status(ctx.current_state())};
    winrt::check_bool(::SetServiceStatus(svcHandle.get(), &initialStatus));
    ctx.os() << "Set initial status" << std::endl;

    do {
      const auto newState{em::run_state(ctx.current_state(), ctx)};
      auto status{em::service_status(newState)};
      winrt::check_bool(::SetServiceStatus(svcHandle.get(), &status));
      ctx.os() << "Updated status to " << status.dwCurrentState << std::endl;

      ctx.transition(newState);
    } while (!ctx.current_state().is_stopped());
    ctx.os() << "run_service() terminated" << std::endl;
  } catch (...) {
    ctx.log_active_exception();
  }
}

}// namespace
}// namespace em

int main(int argc, char *argv[]) try {
  winrt::init_apartment();

  argparse::ArgumentParser app(std::string{em::ServiceName},
                               std::string{em::ServiceVersion});
  app.add_description("Launch or control a service managing the volumes of running programs.");
  app.add_argument("--install")
      .default_value(false)
      .implicit_value(true)
      .help("install the service");

  try {
    app.parse_args(argc, argv);
  } catch (const std::runtime_error &e) {
    std::cerr << e.what() << '\n'
              << app;
    return 1;
  }

  if (app.get<bool>("--install")) {
    return em::install_service();
  }

  return em::start_service_dispatcher();
} catch (const em::ProfileError &e) {
  std::cerr << e.what() << '\n';
} catch (const std::exception &e) {
  std::cerr << "Unhandled exception: " << e.what() << '\n';
} catch (const winrt::hresult_error &e) {
  std::cerr << "Unhandled exception: " << winrt::to_string(e.message()) << '\n';
}
