/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Asset/AssetRegistry.h>

namespace UnitTest
{
    using AssetRegistry = LeakDetectionFixture;

    TEST_F(AssetRegistry, LegacyIdMappingTest)
    {
        using namespace ::testing;

        AzFramework::AssetRegistry registry;

        AZ::Data::AssetId realId("{914F8E72-5EBB-461E-A029-90B07DD7D0E4}", 1);
        AZ::Data::AssetId legacyId1("{C94A4B65-5F1E-48C6-9704-42BB6CF61E11}", 2);
        AZ::Data::AssetId legacyId2("{9CF3C5F2-62AF-4B87-A26E-F8714AB91D38}", 3);
        AZ::Data::AssetId realId2("{8735EA11-CB48-41D5-8D63-2A5AFB269952}", 4);
        AZ::Data::AssetId legacyId3("{153CC980-91FC-4766-92CE-67222DF91F3C}", 5);

        registry.RegisterLegacyAssetMapping(legacyId1, realId);
        registry.RegisterLegacyAssetMapping(legacyId2, realId);
        registry.RegisterLegacyAssetMapping(legacyId3, realId2);

        auto fullSet = registry.GetLegacyMappingSubsetFromRealIds({ realId, realId2 });

        EXPECT_THAT(fullSet, ::testing::UnorderedElementsAre(Pair(legacyId1, realId), Pair(legacyId2, realId), Pair(legacyId3, realId2)));

        auto id1Set = registry.GetLegacyMappingSubsetFromRealIds({ realId });

        EXPECT_THAT(id1Set, ::testing::UnorderedElementsAre(Pair(legacyId1, realId), Pair(legacyId2, realId)));

        auto id2Set = registry.GetLegacyMappingSubsetFromRealIds({ realId2 });

        EXPECT_THAT(id2Set, ::testing::UnorderedElementsAre(Pair(legacyId3, realId2)));

        registry.UnregisterLegacyAssetMappingsForAsset(realId);
        id1Set = registry.GetLegacyMappingSubsetFromRealIds({ realId });

        EXPECT_THAT(id1Set, ::testing::UnorderedElementsAre());

        registry.UnregisterLegacyAssetMappingsForAsset(realId2);
        id2Set = registry.GetLegacyMappingSubsetFromRealIds({ realId2 });

        EXPECT_THAT(id2Set, ::testing::UnorderedElementsAre());
    }
}
