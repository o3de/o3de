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

#ifndef CRASHPAD_COMPAT_LINUX_SYS_PTRACE_H_
#define CRASHPAD_COMPAT_LINUX_SYS_PTRACE_H_

#include_next <sys/ptrace.h>

#include <sys/cdefs.h>

// https://sourceware.org/bugzilla/show_bug.cgi?id=22433
#if !defined(PTRACE_GET_THREAD_AREA) && \
    defined(__GLIBC__) && (defined(__i386__) || defined(__x86_64__))
static constexpr __ptrace_request PTRACE_GET_THREAD_AREA =
    static_cast<__ptrace_request>(25);
#define PTRACE_GET_THREAD_AREA PTRACE_GET_THREAD_AREA
#endif  // !PTRACE_GET_THREAD_AREA && __GLIBC__ && (__i386__ || __x86_64__)

#endif  // CRASHPAD_COMPAT_LINUX_SYS_PTRACE_H_
