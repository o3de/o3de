/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/RapidJsonAllocator.h>

#if USING_AZCORE_RAPIDJSON_ALLOCATION_OVERRIDES
    namespace AZ::JSON
    {
        pointer_type do_malloc(
            size_type byteSize,
            size_type alignment,
            int flags,
            const char* name,
            const char* fileName,
            int lineNum,
            unsigned int suppressStackRecord)
        {
            return AllocatorInstance<RapidJSONAllocator>::Get().Allocate(
                byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord);
        }

        pointer_type do_realloc(pointer_type ptr, size_type newSize, size_type newAlignment)
        {
            return AllocatorInstance<RapidJSONAllocator>::Get().ReAllocate(ptr, newSize, newAlignment);
        }

        void do_free(pointer_type ptr)
        {
            return AllocatorInstance<RapidJSONAllocator>::Get().DeAllocate(ptr);
        }
    } // namespace AZ::JSON

#endif // USING_AZCORE_RAPIDJSON_ALLOCATION_OVERRIDES

