/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * O3DE-maintained replacement for mcpp's configure-generated config.h.
 * Apple targets use mcpp's macOS-compatible path behavior; other Unix-like
 * targets use its Linux-compatible configuration.
 */

#pragma once

#define COMPILER INDEPENDENT
#define MCPP_LIB 1

#if defined(_M_X64) || defined(__x86_64__)
#   define CPU "x86_64"
#elif defined(_M_IX86) || defined(__i386__)
#   define CPU "i386"
#elif defined(_M_ARM64) || defined(__aarch64__)
#   define CPU "aarch64"
#elif defined(_M_ARM) || defined(__arm__)
#   define CPU "arm"
#else
#   define CPU "unknown"
#endif

#if defined(_WIN32)
#   undef COMPILER
#   define COMPILER MSC
#   define HOST_COMPILER MSC
#   define HOST_CMP_NAME "Visual C"
#   define HOST_SYSTEM SYS_WIN
#   define SYSTEM SYS_WIN
#   define FNAME_FOLD 1
#   define LL_FORM "I64"
#   define OBJEXT "obj"
#else
#   define HOST_COMPILER GNUC
#   define HOST_CMP_NAME "GCC-compatible"
#   define GCC_MAJOR_VERSION "4"
#   define GCC_MINOR_VERSION "2"
#   if defined(__APPLE__)
#       define HOST_SYSTEM SYS_MAC
#       define SYSTEM SYS_MAC
#       define FNAME_FOLD 1
#   else
#       define HOST_SYSTEM SYS_LINUX
#       define SYSTEM SYS_LINUX
#   endif
#   define HAVE_DLFCN_H 1
#   define HAVE_STPCPY 1
#   define HAVE_UNISTD_H 1
#   define LL_FORM "j"
#   define OBJEXT "o"
#endif

#define HAVE_INTMAX_T 1
#define HAVE_INTTYPES_H 1
#define HAVE_LONG_LONG 1
#define HAVE_MEMORY_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define STDC_HEADERS 1

#define PACKAGE "mcpp"
#define PACKAGE_NAME "mcpp"
#define PACKAGE_STRING "mcpp 2.7.2"
#define PACKAGE_TARNAME "mcpp"
#define PACKAGE_VERSION "2.7.2"
#define VERSION "2.7.2"
