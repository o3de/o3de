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

#include <AzCore/Memory/OverrunDetectionAllocator.h>

namespace AZ
{
    namespace Internal
    {
        static const int ODS_PAGE_SIZE = 4096;
        static const int ODS_PAGE_SHIFT = 12;
        static const int ODS_PAGES_PER_ALLOCATION = 1;
        static const int ODS_PAGES_PER_ALLOCATION_SHIFT = 0;
        static const int ODS_ALLOCATION_SIZE = ODS_PAGE_SIZE * ODS_PAGES_PER_ALLOCATION;

        class UnimplementedOverrunDetectionSchema : public OverrunDetectionSchema::PlatformAllocator
        {
        public:
            virtual SystemInformation GetSystemInformation() override
            {
                SystemInformation result;
                return result;
            }

            virtual void* ReserveBytes(size_t amount) override
            {
                AZ_UNUSED(amount);
                AZ_Assert(false, "No implementation of OverrunDetectionSchema for this platform");
                return nullptr;
            }

            virtual void ReleaseReservedBytes(void* base) override
            {
                AZ_UNUSED(base);
                AZ_Assert(false, "No implementation of OverrunDetectionSchema for this platform");
            }

            virtual void* CommitBytes(void* base, size_t amount) override
            {
                AZ_UNUSED(base);
                AZ_UNUSED(amount);
                AZ_Assert(false, "No implementation of OverrunDetectionSchema for this platform");
                return nullptr;
            }

            virtual void DecommitBytes(void* base, size_t amount) override
            {
                AZ_UNUSED(base);
                AZ_UNUSED(amount);
                AZ_Assert(false, "No implementation of OverrunDetectionSchema for this platform");
            }
        };

        using PlatformOverrunDetectionSchema = UnimplementedOverrunDetectionSchema;
    }
}
