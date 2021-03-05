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

#pragma once

// Include this file instead of including zlib.h directly
// because zconf.h (included by zlib.h) defines WINDOWS and WIN32 - those
// definitions conflict with CryEngine's definitions.

#if defined(CRY_TMP_DEFINED_WINDOWS) || defined(CRY_TMP_DEFINED_WIN32)
#   error CRY_TMP_DEFINED_WINDOWS and/or CRY_TMP_DEFINED_WIN32 already defined
#endif

#if defined(WINDOWS)
#   define CRY_TMP_DEFINED_WINDOWS 1
#endif
#if defined(WIN32)
#   define CRY_TMP_DEFINED_WIN32 1
#endif

#include <zlib.h>

#if !defined(CRY_TMP_DEFINED_WINDOWS)
#  undef WINDOWS
#endif
#undef CRY_TMP_DEFINED_WINDOWS
#if !defined(CRY_TMP_DEFINED_WIN32)
#   undef WIN32
#endif
#undef CRY_TMP_DEFINED_WIN32
