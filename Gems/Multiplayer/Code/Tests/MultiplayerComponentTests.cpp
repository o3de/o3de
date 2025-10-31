/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CommonNetworkEntitySetup.h>
#include <MockInterfaces.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzNetworking/Serialization/StringifySerializer.h>
#include <AzTest/AzTest.h>
#include <Multiplayer/Components/MultiplayerComponent.h>

namespace Multiplayer
{
    using MultiplayerComponentTests = NetworkEntityTests;

    TEST_F(MultiplayerComponentTests, SerializeNetworkPropertyHelperArrayCreatesUniqueEntriesForEachValue)
    {
        constexpr size_t NumTestEntries = 5;

        AzNetworking::StringifySerializer serializer;
        AzNetworking::FixedSizeVectorBitset<NumTestEntries> bitset;
        AZStd::array<int32_t, NumTestEntries> testValues = { 5, 10, 15, 20, 25 };
        NetComponentId componentId = aznumeric_cast<NetComponentId>(0);
        PropertyIndex propertyIndex = aznumeric_cast<PropertyIndex>(0);
        MultiplayerStats stats;

        // Mark all the values as changed so that they get serialized.
        bitset.AddBits(NumTestEntries);
        for (uint32_t index = 0; index < NumTestEntries; index++)
        {
            bitset.SetBit(index, true);
        }

        AzNetworking::FixedSizeBitsetView bitsetView(bitset, 0, NumTestEntries);
        SerializeNetworkPropertyHelperArray(serializer, bitsetView, testValues, componentId, propertyIndex, stats);

        // Each entry in the array should have been serialized to a unique key/value pair.
        auto valueMap = serializer.GetValueMap();
        EXPECT_EQ(valueMap.size(), NumTestEntries);
    }

    TEST_F(MultiplayerComponentTests, SerializeNetworkPropertyHelperVectorCreatesUniqueEntriesForEachValue)
    {
        constexpr size_t NumTestEntries = 5;
        constexpr size_t NumTestEntriesPlusSize = NumTestEntries + 1;

        AzNetworking::StringifySerializer serializer;
        // We need an extra bit for tracking the currently-used size of the fixed_vector.
        AzNetworking::FixedSizeVectorBitset<NumTestEntriesPlusSize> bitset;
        AZStd::fixed_vector<int32_t, NumTestEntries> testValues = { 5, 10, 15, 20, 25 };
        NetComponentId componentId = aznumeric_cast<NetComponentId>(0);
        PropertyIndex propertyIndex = aznumeric_cast<PropertyIndex>(0);
        MultiplayerStats stats;

        // Mark all the values as changed (including "size") so that they get serialized.
        bitset.AddBits(NumTestEntriesPlusSize);
        for (uint32_t index = 0; index < NumTestEntriesPlusSize; index++)
        {
            bitset.SetBit(index, true);
        }

        AzNetworking::FixedSizeBitsetView bitsetView(bitset, 0, NumTestEntriesPlusSize);
        SerializeNetworkPropertyHelperVector(serializer, bitsetView, testValues, componentId, propertyIndex, stats);

        // Each entry in the fixed_vector should have been serialized to a unique key/value pair, along with an extra entry for "Size".
        auto valueMap = serializer.GetValueMap();
        EXPECT_EQ(valueMap.size(), NumTestEntriesPlusSize);
    }

} // namespace Multiplayer
