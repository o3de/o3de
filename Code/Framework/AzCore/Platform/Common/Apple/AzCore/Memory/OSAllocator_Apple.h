/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <cstdlib>
#include <cstring>
#include <malloc/malloc.h>

inline void* memalign(size_t blocksize, size_t bytes)
{
    void* ptr = nullptr;
    [[maybe_unused]] int result = posix_memalign(&ptr, blocksize < sizeof(void*) ? sizeof(void*) : blocksize, bytes);
    AZ_Assert(result == 0, "Posix memalign failed %d", result);
    return ptr;
}

# define AZ_OS_MALLOC(byteSize, alignment) memalign(alignment, byteSize)
# define AZ_OS_FREE(pointer) ::free(pointer)
# define AZ_OS_REALLOC(pointer, byteSize, alignment) aligned_realloc(pointer, byteSize, alignment)
# define AZ_OS_MSIZE(pointer, alignment) malloc_size(pointer)
#define AZ_MALLOC_TRIM(pad) AZ_UNUSED(pad)

inline void* aligned_realloc(void* ptr, size_t size, size_t alignment)
{
    void* newPtr = AZ_OS_MALLOC(size, alignment);
    AZ_Assert(newPtr, "aligned_alloc failed in realloc");
#if defined(az_has_builtin_memcpy)
    __builtin_memcpy(newPtr, ptr, std::min(size, AZ_OS_MSIZE(ptr, alignment)));
#else
    std::memcpy(newPtr, ptr, std::min(size, AZ_OS_MSIZE(ptr, alignment)));
#endif
    AZ_OS_FREE(ptr);
    return newPtr;
}
