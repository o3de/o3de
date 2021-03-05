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

#include <AzCore/Memory/Memory.h>
#include <AzCore/IO/Streamer/DedicatedCache.h>
#include <Tests/Streamer/StreamStackEntryConformityTests.h>

namespace AZ::IO
{
    class DedicatedCacheTestDescription :
        public StreamStackEntryConformityTestsDescriptor<DedicatedCache>
    {
    public:
        DedicatedCache CreateInstance() override
        {
            return DedicatedCache(1 * 1024 * 1024, 64 * 1024, AZCORE_GLOBAL_NEW_ALIGNMENT, false);
        }

        bool UsesSlots() const override
        {
            return false;
        }
    };

    INSTANTIATE_TYPED_TEST_CASE_P(Streamer_DedicatedCacheConformityTests, StreamStackEntryConformityTests, DedicatedCacheTestDescription);
} // namespace AZ::IO
