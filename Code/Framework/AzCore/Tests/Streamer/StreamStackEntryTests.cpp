/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Streamer/StreamStackEntryConformityTests.h>

namespace AZ::IO
{
    class StreamStackEntryTestDescription :
        public StreamStackEntryConformityTestsDescriptor<StreamStackEntry>
    {
    public:
        StreamStackEntry CreateInstance() override
        {
            return StreamStackEntry("Name");
        }

        bool UsesSlots() const override
        {
            return false;
        }
    };

    INSTANTIATE_TYPED_TEST_CASE_P(Streamer_StreamStackEntryConformityTests, StreamStackEntryConformityTests, StreamStackEntryTestDescription);
} // namespace AZ::IO
