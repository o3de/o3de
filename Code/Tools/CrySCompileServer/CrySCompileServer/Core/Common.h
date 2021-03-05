/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYSCOMPILESERVER_CORE_COMMON_H
#define CRYINCLUDE_CRYSCOMPILESERVER_CORE_COMMON_H
#pragma once

#include <AzCore/base.h>
#include <AzCore/PlatformDef.h>
#include <AzCore/PlatformIncl.h>

#if defined(AZ_PLATFORM_WINDOWS)
#   if !defined(_WIN32_WINNT)
#       define _WIN32_WINNT 0x0501
#   endif

// Windows platform requires either a long or an unsigned long/uint64 for the
// Interlock instructions.
typedef long AtomicCountType;

#else

// Linux/Mac platforms don't support a long for the atomic types, only int32 or
// int64 (no unsigned support).
typedef int32_t AtomicCountType;

#endif

#endif // CRYINCLUDE_CRYSCOMPILESERVER_CORE_COMMON_H
