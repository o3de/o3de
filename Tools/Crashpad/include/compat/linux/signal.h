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

#ifndef CRASHPAD_COMPAT_LINUX_SIGNAL_H_
#define CRASHPAD_COMPAT_LINUX_SIGNAL_H_

#include_next <signal.h>

// Missing from glibc and bionic-x86_64
#if defined(__x86_64__) || defined(__i386__)
#if !defined(X86_FXSR_MAGIC)
#define X86_FXSR_MAGIC 0x0000
#endif
#endif  // __x86_64__ || __i386__

#endif  // CRASHPAD_COMPAT_LINUX_SIGNAL_H_
