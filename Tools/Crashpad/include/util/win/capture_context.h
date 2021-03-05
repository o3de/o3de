// Copyright 2015 The Crashpad Authors. All rights reserved.
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

#ifndef CRASHPAD_CLIENT_CAPTURE_CONTEXT_WIN_H_
#define CRASHPAD_CLIENT_CAPTURE_CONTEXT_WIN_H_

#include <windows.h>

namespace crashpad {

//! \brief Saves the CPU context.
//!
//! The CPU context will be captured as accurately and completely as possible,
//! containing an atomic snapshot at the point of this function’s return. This
//! function does not modify any registers.
//!
//! This function captures all integer registers as well as the floating-point
//! and vector (SSE) state. It does not capture debug registers, which are
//! inaccessible by user code.
//!
//! This function is a replacement for `RtlCaptureContext()`, which contains
//! bugs and limitations. On 32-bit x86, `RtlCaptureContext()` requires that
//! `ebp` be used as a frame pointer, and returns `ebp`, `esp`, and `eip` out of
//! sync with the other registers. Both the 32-bit x86 and 64-bit x86_64
//! versions of `RtlCaptureContext()` capture only the state of the integer
//! registers, ignoring floating-point and vector state.
//!
//! \param[out] context The structure to store the context in.
//!
//! \note On x86_64, the value for `rcx` will be populated with the address of
//!     this function’s argument, as mandated by the ABI.
void CaptureContext(CONTEXT* context);

}  // namespace crashpad

#endif  // CRASHPAD_CLIENT_CAPTURE_CONTEXT_WIN_H_
