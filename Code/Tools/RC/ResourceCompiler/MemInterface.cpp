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


// Description : Memory management functions used by CryMemoryManager
//               it looks for them first in the executable, then in CrySystem.dll

// TEMPORARY FIX - define this macro to allow app to be built without using VS2005 SP1 CRT libraries - we
// want to continue using the pre-SP1 libraries (at least for now), since non-programmers don't have the new
// libraries as yet.

#include <stdlib.h>
#include <AzCore/PlatformDef.h>


extern "C" {
AZ_DLL_EXPORT void* CryMalloc(size_t size) {
    return malloc(size);
}
AZ_DLL_EXPORT void* CryRealloc(void* memblock, size_t size) {
    return realloc(memblock, size);
}
AZ_DLL_EXPORT void* CryReallocSize(void* memblock, size_t oldsize, size_t size) {
    return realloc(memblock, size);
}
AZ_DLL_EXPORT void CryFree(void* p) { free(p); }
AZ_DLL_EXPORT void CryFreeSize(void* p, size_t size) { free(p); }
}
