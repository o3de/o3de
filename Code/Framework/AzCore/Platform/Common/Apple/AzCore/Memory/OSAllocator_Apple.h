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