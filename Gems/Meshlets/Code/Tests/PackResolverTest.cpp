/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzTest/AzTest.h>
#include <Meshlets/PackResolver.h>

namespace UnitTest
{
    using namespace AZ::Meshlets;

    TEST(PackResolver, ReturnsInvalidIdForUnknownModel)
    {
        PackResolver r;
        AZ::Data::AssetId unknown(AZ::Uuid::CreateRandom(), 0);
        EXPECT_FALSE(r.Find(unknown).IsValid());
    }

    TEST(PackResolver, EmptyAtConstruction)
    {
        PackResolver r;
        EXPECT_EQ(0u, r.GetMappingCount());
    }
}
