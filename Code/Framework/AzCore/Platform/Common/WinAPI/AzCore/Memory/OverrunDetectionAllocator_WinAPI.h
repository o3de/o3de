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
        static const int ODS_PAGES_PER_ALLOCATION = 16;
        static const int ODS_PAGES_PER_ALLOCATION_SHIFT = 4;
        static const int ODS_ALLOCATION_SIZE = ODS_PAGE_SIZE * ODS_PAGES_PER_ALLOCATION;

        class WinAPIOverrunDetectionSchema : public OverrunDetectionSchema::PlatformAllocator
        {
        public:
            virtual SystemInformation GetSystemInformation() override
            {
                SystemInformation result;
                SYSTEM_INFO info;
                GetSystemInfo(&info);
                result.m_pageSize = info.dwPageSize;
                result.m_minimumAllocationSize = info.dwAllocationGranularity;

                return result;
            }

            virtual void* ReserveBytes(size_t amount) override
            {
                void* result = VirtualAlloc(0, amount, MEM_RESERVE, PAGE_NOACCESS);

                if (!result)
                {
                    // Out of memory
                    AZ_Crash();
                }

                return result;
            }

            virtual void ReleaseReservedBytes(void* base) override
            {
                VirtualFree(base, 0, MEM_RELEASE);
            }

            virtual void* CommitBytes(void* base, size_t amount) override
            {
                void* result = VirtualAlloc(base, amount, MEM_COMMIT, PAGE_READWRITE);

                if (!result)
                {
                    // Out of memory
                    AZ_Crash();
                }

                return result;
            }

            virtual void DecommitBytes(void* base, size_t amount) override
            {
                VirtualFree(base, amount, MEM_DECOMMIT);
            }
        };

        using PlatformOverrunDetectionSchema = WinAPIOverrunDetectionSchema;
    }
}
