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

#pragma once

#include "ProjectDefines.h"

#ifdef INCLUDE_LIBTOMCRYPT

#include "CryMemoryManager.h"

#define USE_LTM
#define LTM_DESC
#define LTC_EXPORT
#define LTC_NO_PROTOTYPES

#if defined(AZ_COMPILER_MSVC)
#define LTC_CALL __cdecl
#else
#define LTC_CALL
#endif

LTC_EXPORT void* LTC_CALL tomcrypt_Malloc(size_t size);
LTC_EXPORT void* LTC_CALL tomcrypt_Realloc(void* ptr, size_t size);
LTC_EXPORT void* LTC_CALL tomcrypt_Calloc(size_t num, size_t size);
LTC_EXPORT void LTC_CALL tomcrypt_Free(void* ptr);

#define XMALLOC tomcrypt_Malloc
#define XREALLOC tomcrypt_Realloc
#define XCALLOC tomcrypt_Calloc
#define XFREE   tomcrypt_Free

#include <tomcrypt.h>
#undef byte // tomcrypt defines a byte macro which conflicts with out byte data type
#define STREAM_CIPHER_NAME "twofish"
extern prng_state g_yarrow_prng_state;
extern rsa_key g_rsa_key_public_for_sign;
#endif //INCLUDE_LIBTOMCRYPT
