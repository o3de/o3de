/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
