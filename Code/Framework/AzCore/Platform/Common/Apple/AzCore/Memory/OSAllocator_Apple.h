/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

inline void* memalign(size_t blocksize, size_t bytes)
{
    void* ptr = nullptr;
    int result = posix_memalign(&ptr, blocksize < sizeof(void*) ? sizeof(void*) : blocksize, bytes);
    AZ_Assert(result == 0, "Posix memalign failed %d", result);
    (void)result;   // avoid unused variable warning
    return ptr;
}

# define AZ_OS_MALLOC(byteSize, alignment) memalign(alignment, byteSize)
# define AZ_OS_FREE(pointer) ::free(pointer)
# define AZ_MALLOC_TRIM(pad) AZ_UNUSED(pad)
