// Copyright 2014 The Crashpad Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CRASHPAD_CLIENT_CRASHPAD_CLIENT_H_
#define CRASHPAD_CLIENT_CRASHPAD_CLIENT_H_

#include <map>
#include <string>
#include <vector>

#include <stdint.h>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "build/build_config.h"

#if defined(OS_MACOSX)
#include "base/mac/scoped_mach_port.h"
#elif defined(OS_WIN)
#include <windows.h>
#include "util/win/scoped_handle.h"
#endif

namespace crashpad {

//! \brief The primary interface for an application to have Crashpad monitor
//!     it for crashes.
class CrashpadClient {
 public:
  CrashpadClient();
  ~CrashpadClient();

  //! \brief Starts a Crashpad handler process, performing any necessary
  //!     handshake to configure it.
  //!
  //! This method directs crashes to the Crashpad handler. On macOS, this is
  //! applicable to this process and all subsequent child processes. On Windows,
  //! child processes must also register by using SetHandlerIPCPipe().
  //!
  //! On macOS, this method starts a Crashpad handler and obtains a Mach send
  //! right corresponding to a receive right held by the handler process. The
  //! handler process runs an exception server on this port. This method sets
  //! the task’s exception port for `EXC_CRASH`, `EXC_RESOURCE`, and `EXC_GUARD`
  //! exceptions to the Mach send right obtained. The handler will be installed
  //! with behavior `EXCEPTION_STATE_IDENTITY | MACH_EXCEPTION_CODES` and thread
  //! state flavor `MACHINE_THREAD_STATE`. Exception ports are inherited, so a
  //! Crashpad handler started here will remain the handler for any child
  //! processes created after StartHandler() is called. These child processes do
  //! not need to call StartHandler() or be aware of Crashpad in any way. The
  //! Crashpad handler will receive crashes from child processes that have
  //! inherited it as their exception handler even after the process that called
  //! StartHandler() exits.
  //!
  //! On Windows, if \a asynchronous_start is `true`, this function will not
  //! directly call `CreateProcess()`, making it suitable for use in a
  //! `DllMain()`. In that case, the handler is started from a background
  //! thread, deferring the handler's startup. Nevertheless, regardless of the
  //! value of \a asynchronous_start, after calling this method, the global
  //! unhandled exception filter is set up, and all crashes will be handled by
  //! Crashpad. Optionally, use WaitForHandlerStart() to join with the
  //! background thread and retrieve the status of handler startup.
  //!
  //! \param[in] handler The path to a Crashpad handler executable.
  //! \param[in] database The path to a Crashpad database. The handler will be
  //!     started with this path as its `--database` argument.
  //! \param[in] metrics_dir The path to an already existing directory where
  //!     metrics files can be stored. The handler will be started with this
  //!     path as its `--metrics-dir` argument.
  //! \param[in] url The URL of an upload server. The handler will be started
  //!     with this URL as its `--url` argument.
  //! \param[in] annotations Process annotations to set in each crash report.
  //!     The handler will be started with an `--annotation` argument for each
  //!     element in this map.
  //! \param[in] arguments Additional arguments to pass to the Crashpad handler.
  //!     Arguments passed in other parameters and arguments required to perform
  //!     the handshake are the responsibility of this method, and must not be
  //!     specified in this parameter.
  //! \param[in] restartable If `true`, the handler will be restarted if it
  //!     dies, if this behavior is supported. This option is not available on
  //!     all platforms, and does not function on all OS versions. If it is
  //!     not supported, it will be ignored.
  //! \param[out] asynchronous_start If `true`, the handler will be started from
  //!     a background thread. Optionally, WaitForHandlerStart() can be used at
  //!     a suitable time to retreive the result of background startup. This
  //!     option is only used on Windows.
  //!
  //! \return `true` on success, `false` on failure with a message logged.
  bool StartHandler(const base::FilePath& handler,
                    const base::FilePath& database,
                    const base::FilePath& metrics_dir,
                    const std::string& url,
                    const std::map<std::string, std::string>& annotations,
                    const std::vector<std::string>& arguments,
                    bool restartable,
                    bool asynchronous_start);

#if defined(OS_MACOSX) || DOXYGEN
  //! \brief Sets the process’ crash handler to a Mach service registered with
  //!     the bootstrap server.
  //!
  //! This method is only defined on macOS.
  //!
  //! See StartHandler() for more detail on how the port and handler are
  //! configured.
  //!
  //! \param[in] service_name The service name of a Crashpad exception handler
  //!     service previously registered with the bootstrap server.
  //!
  //! \return `true` on success, `false` on failure with a message logged.
  bool SetHandlerMachService(const std::string& service_name);

  //! \brief Sets the process’ crash handler to a Mach port.
  //!
  //! This method is only defined on macOS.
  //!
  //! See StartHandler() for more detail on how the port and handler are
  //! configured.
  //!
  //! \param[in] exception_port An `exception_port_t` corresponding to a
  //!     Crashpad exception handler service.
  //!
  //! \return `true` on success, `false` on failure with a message logged.
  bool SetHandlerMachPort(base::mac::ScopedMachSendRight exception_port);

  //! \brief Retrieves a send right to the process’ crash handler Mach port.
  //!
  //! This method is only defined on macOS.
  //!
  //! This method can be used to obtain the crash handler Mach port when a
  //! Crashpad client process wishes to provide a send right to this port to
  //! another process. The IPC mechanism used to convey the right is under the
  //! application’s control. If the other process wishes to become a client of
  //! the same crash handler, it can provide the transferred right to
  //! SetHandlerMachPort().
  //!
  //! See StartHandler() for more detail on how the port and handler are
  //! configured.
  //!
  //! \return The Mach port set by SetHandlerMachPort(), possibly indirectly by
  //!     a call to another method such as StartHandler() or
  //!     SetHandlerMachService(). This method must only be called after a
  //!     successful call to one of those methods. `MACH_PORT_NULL` on failure
  //!     with a message logged.
  base::mac::ScopedMachSendRight GetHandlerMachPort() const;
#endif

#if defined(OS_WIN) || DOXYGEN
  //! \brief Sets the IPC pipe of a presumably-running Crashpad handler process
  //!     which was started with StartHandler() or by other compatible means
  //!     and does an IPC message exchange to register this process with the
  //!     handler. Crashes will be serviced once this method returns.
  //!
  //! This method is only defined on Windows.
  //!
  //! This method sets the unhandled exception handler to a local
  //! function that when reached will "signal and wait" for the crash handler
  //! process to create the dump.
  //!
  //! \param[in] ipc_pipe The full name of the crash handler IPC pipe. This is
  //!     a string of the form `&quot;\\.\pipe\NAME&quot;`.
  //!
  //! \return `true` on success and `false` on failure.
  bool SetHandlerIPCPipe(const std::wstring& ipc_pipe);

  //! \brief Retrieves the IPC pipe name used to register with the Crashpad
  //!     handler.
  //!
  //! This method is only defined on Windows.
  //!
  //! This method retrieves the IPC pipe name set by SetHandlerIPCPipe(), or a
  //! suitable IPC pipe name chosen by StartHandler(). It must only be called
  //! after a successful call to one of those methods. It is intended to be used
  //! to obtain the IPC pipe name so that it may be passed to other processes,
  //! so that they may register with an existing Crashpad handler by calling
  //! SetHandlerIPCPipe().
  //!
  //! \return The full name of the crash handler IPC pipe, a string of the form
  //!     `&quot;\\.\pipe\NAME&quot;`.
  std::wstring GetHandlerIPCPipe() const;

  //! \brief When `asynchronous_start` is used with StartHandler(), this method
  //!     can be used to block until the handler launch has been completed to
  //!     retrieve status information.
  //!
  //! This method should not be used unless `asynchronous_start` was `true`.
  //!
  //! \param[in] timeout_ms The number of milliseconds to wait for a result from
  //!     the background launch, or `0xffffffff` to block indefinitely.
  //!
  //! \return `true` if the hander startup succeeded, `false` otherwise, and an
  //!     error message will have been logged.
  bool WaitForHandlerStart(unsigned int timeout_ms);

  //! \brief Requests that the handler capture a dump even though there hasn't
  //!     been a crash.
  //!
  //! \param[in] context A `CONTEXT`, generally captured by CaptureContext() or
  //!     similar.
  static void DumpWithoutCrash(const CONTEXT& context);

  //! \brief Requests that the handler capture a dump using the given \a
  //!     exception_pointers to get the `EXCEPTION_RECORD` and `CONTEXT`.
  //!
  //! This function is not necessary in general usage as an unhandled exception
  //! filter is installed by StartHandler() or SetHandlerIPCPipe().
  //!
  //! \param[in] exception_pointers An `EXCEPTION_POINTERS`, as would generally
  //!     passed to an unhandled exception filter.
  static void DumpAndCrash(EXCEPTION_POINTERS* exception_pointers);

  //! \brief Requests that the handler capture a dump of a different process.
  //!
  //! The target process must be an already-registered Crashpad client. An
  //! exception will be triggered in the target process, and the regular dump
  //! mechanism used. This function will block until the exception in the target
  //! process has been handled by the Crashpad handler.
  //!
  //! This function is unavailable when running on Windows XP and will return
  //! `false`.
  //!
  //! \param[in] process A `HANDLE` identifying the process to be dumped.
  //! \param[in] blame_thread If non-null, a `HANDLE` valid in the caller's
  //!     process, referring to a thread in the target process. If this is
  //!     supplied, instead of the exception referring to the location where the
  //!     exception was injected, an exception record will be fabricated that
  //!     refers to the current location of the given thread.
  //! \param[in] exception_code If \a blame_thread is non-null, this will be
  //!     used as the exception code in the exception record.
  //!
  //! \return `true` if the exception was triggered successfully.
  bool DumpAndCrashTargetProcess(HANDLE process,
                                 HANDLE blame_thread,
                                 DWORD exception_code) const;

  enum : uint32_t {
    //! \brief The exception code (roughly "Client called") used when
    //!     DumpAndCrashTargetProcess() triggers an exception in a target
    //!     process.
    //!
    //! \note This value does not have any bits of the top nibble set, to avoid
    //!     confusion with real exception codes which tend to have those bits
    //!     set.
    kTriggeredExceptionCode = 0xcca11ed,
  };
#endif

#if defined(OS_MACOSX) || DOXYGEN
  //! \brief Configures the process to direct its crashes to the default handler
  //!     for the operating system.
  //!
  //! On macOS, this sets the task’s exception port as in SetHandlerMachPort(),
  //! but the exception handler used is obtained from
  //! SystemCrashReporterHandler(). If the system’s crash reporter handler
  //! cannot be determined or set, the task’s exception ports for crash-type
  //! exceptions are cleared.
  //!
  //! Use of this function is strongly discouraged.
  //!
  //! \warning After a call to this function, Crashpad will no longer monitor
  //!     the process for crashes until a subsequent call to
  //!     SetHandlerMachPort().
  //!
  //! \note This is provided as a static function to allow it to be used in
  //!     situations where a CrashpadClient object is not otherwise available.
  //!     This may be useful when a child process inherits its parent’s Crashpad
  //!     handler, but wants to sever this tie.
  static void UseSystemDefaultHandler();
#endif

 private:
#if defined(OS_MACOSX)
  base::mac::ScopedMachSendRight exception_port_;
#elif defined(OS_WIN)
  std::wstring ipc_pipe_;
  ScopedKernelHANDLE handler_start_thread_;
#endif  // OS_MACOSX

  DISALLOW_COPY_AND_ASSIGN(CrashpadClient);
};

}  // namespace crashpad

#endif  // CRASHPAD_CLIENT_CRASHPAD_CLIENT_H_
