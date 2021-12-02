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
        static const int ODS_PAGES_PER_ALLOCATION = 16;
        static const int ODS_PAGES_PER_ALLOCATION_SHIFT = 4;
        static const int ODS_ALLOCATION_SIZE = ODS_PAGE_SIZE * ODS_PAGES_PER_ALLOCATION;

        class WinAPIOverrunDetectionSchema : public OverrunDetectionSchema::PlatformAllocator
        {
        public:
            SystemInformation GetSystemInformation() override
            {
                SystemInformation result;
                SYSTEM_INFO info;
                GetSystemInfo(&info);
                result.m_pageSize = info.dwPageSize;
                result.m_minimumAllocationSize = info.dwAllocationGranularity;

                return result;
            }

            void* ReserveBytes(size_t amount) override
            {
                void* result = VirtualAlloc(0, amount, MEM_RESERVE, PAGE_NOACCESS);

                if (!result)
                {
                    // Out of memory
                    AZ_Crash();
                }

                return result;
            }

            void ReleaseReservedBytes(void* base) override
            {
                VirtualFree(base, 0, MEM_RELEASE);
            }

            void* CommitBytes(void* base, size_t amount) override
            {
                void* result = VirtualAlloc(base, amount, MEM_COMMIT, PAGE_READWRITE);

                if (!result)
                {
                    // Out of memory
                    AZ_Crash();
                }

                return result;
            }

            void DecommitBytes(void* base, size_t amount) override
            {
                VirtualFree(base, amount, MEM_DECOMMIT);
            }
        };

        using PlatformOverrunDetectionSchema = WinAPIOverrunDetectionSchema;
    }
}
