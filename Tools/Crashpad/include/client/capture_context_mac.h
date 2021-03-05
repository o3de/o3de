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

#ifndef CRASHPAD_CLIENT_CAPTURE_CONTEXT_MAC_H_
#define CRASHPAD_CLIENT_CAPTURE_CONTEXT_MAC_H_

#include <mach/mach.h>

#include "build/build_config.h"

namespace crashpad {

#if defined(ARCH_CPU_X86_FAMILY)
using NativeCPUContext = x86_thread_state;
#endif

//! \brief Saves the CPU context.
//!
//! The CPU context will be captured as accurately and completely as possible,
//! containing an atomic snapshot at the point of this function’s return. This
//! function does not modify any registers.
//!
//! \param[out] cpu_context The structure to store the context in.
//!
//! \note On x86_64, the value for `%%rdi` will be populated with the address of
//!     this function’s argument, as mandated by the ABI. If the value of
//!     `%%rdi` prior to calling this function is needed, it must be obtained
//!     separately prior to calling this function. For example:
//!     \code
//!       uint64_t rdi;
//!       asm("movq %%rdi, %0" : "=m"(rdi));
//!     \endcode
void CaptureContext(NativeCPUContext* cpu_context);

}  // namespace crashpad

#endif  // CRASHPAD_CLIENT_CAPTURE_CONTEXT_MAC_H_
