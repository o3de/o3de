// Copyright 2017 The Crashpad Authors. All rights reserved.
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

#ifndef CRASHPAD_HANDLER_FUCHSIA_EXCEPTION_HANDLER_SERVER_H_
#define CRASHPAD_HANDLER_FUCHSIA_EXCEPTION_HANDLER_SERVER_H_

#include "base/macros.h"

namespace crashpad {

class CrashReportExceptionHandler;

//! \brief Runs the main exception-handling server in Crashpad's handler
//!     process. This class is not yet implemented.
class ExceptionHandlerServer {
 public:
  //! \brief Constructs an ExceptionHandlerServer object.
  ExceptionHandlerServer();
  ~ExceptionHandlerServer();

  //! \brief Runs the exception-handling server.
  //!
  //! \param[in] handler The handler to which the exceptions are delegated when
  //!     they are caught in Run(). Ownership is not transferred.
  void Run(CrashReportExceptionHandler* handler);

 private:
  DISALLOW_COPY_AND_ASSIGN(ExceptionHandlerServer);
};

}  // namespace crashpad

#endif  // CRASHPAD_HANDLER_FUCHSIA_EXCEPTION_HANDLER_SERVER_H_
