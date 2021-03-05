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

#include "CrySystem_precompiled.h"

#include "CryTomcrypt.h"

//////////////////////////////////////////////////////////////////////////
#ifdef INCLUDE_LIBTOMCRYPT

prng_state g_yarrow_prng_state;
// Main public RSA key used for verifying Cry Pak comments
rsa_key g_rsa_key_public_for_sign;

void* LTC_CALL tomcrypt_Malloc(size_t size)
{
    return CryModuleMalloc(size);
}

void* LTC_CALL tomcrypt_Realloc(void* ptr, size_t size)
{
    return CryModuleRealloc(ptr, size);
}

void* LTC_CALL tomcrypt_Calloc(size_t num, size_t size)
{
    return CryModuleCalloc(num, size);
}

void LTC_CALL tomcrypt_Free(void* ptr)
{
    CryModuleFree(ptr);
}

#endif // INCLUDE_LIBTOMCRYPT