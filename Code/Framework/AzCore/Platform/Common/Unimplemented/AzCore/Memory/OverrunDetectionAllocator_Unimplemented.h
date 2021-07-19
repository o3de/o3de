/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
