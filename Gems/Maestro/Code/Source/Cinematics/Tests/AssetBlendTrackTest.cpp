/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Maestro_precompiled.h"

#if !defined(_RELEASE)

#include <AnimKey.h>
#include <Maestro/Types/AssetBlendKey.h>

#include <Cinematics/AssetBlendTrack.h>

#include <AzTest/AzTest.h>

namespace AssetBlendTrackTest
{
    const AZ::Data::AssetId KEY1_ASSET_ID = AZ::Data::AssetId(AZ::Uuid("{86CE36B5-D996-4CEF-943E-3F12008694E1}"), 1);
    const AZ::Data::AssetId KEY2_ASSET_ID = AZ::Data::AssetId(AZ::Uuid("{94D54D20-BACC-4A60-8A03-0DC9B5033E03}"), 2);
    const AZ::Data::AssetId KEY3_ASSET_ID = AZ::Data::AssetId(AZ::Uuid("{94D54D20-BACC-4A60-8A03-0DC9B5033E03}"), 3);
    const float KEY_TIME = 1.0f;

    /////////////////////////////////////////////////////////////////////////////////////
    // Testing sub-class
    class CAssetBlendTrackTest : public ::testing::Test
    {
    public:
        CAssetBlendTrackTest();

        void CreateAssetBlendTestKeys();

        CAssetBlendTrack m_assetBlendTrack;
    };

    /////////////////////////////////////////////////////////////////////////////////////
    // Testing setup methods
    CAssetBlendTrackTest::CAssetBlendTrackTest()
    {
        CreateAssetBlendTestKeys();
    }

    void CAssetBlendTrackTest::CreateAssetBlendTestKeys()
    {
        int keyIndex = 0;

        AZ::IAssetBlendKey key1;
        key1.time = 1.0f;
        key1.m_duration = 1.0f;
        key1.m_assetId = KEY1_ASSET_ID;
        m_assetBlendTrack.CreateKey(key1.time);
        m_assetBlendTrack.SetKey(keyIndex++, &key1);

        AZ::IAssetBlendKey key2;
        key2.time = 2.0f;
        key2.m_duration = 1.0f;
        key2.m_assetId = KEY2_ASSET_ID;
        m_assetBlendTrack.CreateKey(key2.time);
        m_assetBlendTrack.SetKey(keyIndex++, &key2);

        AZ::IAssetBlendKey key3;
        key3.time = 3.0f;
        key3.m_duration = 1.0f;
        key3.m_assetId = KEY3_ASSET_ID;
        m_assetBlendTrack.CreateKey(key3.time);
        m_assetBlendTrack.SetKey(keyIndex++, &key3);

    }

    /////////////////////////////////////////////////////////////////////////////////////
    // Tests

    // Asset Blend Track
    TEST_F(CAssetBlendTrackTest, Maestro_CAssetBlendTrackTest_Test01)
    {
        Maestro::AssetBlends<AZ::Data::AssetData> value;

        m_assetBlendTrack.GetValue(0.0f, value);
        ASSERT_GT(value.m_assetBlends.size(), 0) << "Expected to find at least one AssetBlend at time 0.0f.";
        ASSERT_EQ(value.m_assetBlends.at(0).m_assetId, KEY1_ASSET_ID) << "Expected KEY1_ASSET_ID at time 0.0f.";

        m_assetBlendTrack.GetValue(1.5f, value);
        ASSERT_GT(value.m_assetBlends.size(), 0) << "Expected to find at least one AssetBlend at time 1.5f.";
        ASSERT_EQ(value.m_assetBlends.at(0).m_assetId, KEY1_ASSET_ID) << "Expected KEY1_ASSET_ID at time 1.5f.";

        m_assetBlendTrack.GetValue(2.5f, value);
        ASSERT_GT(value.m_assetBlends.size(), 0) << "Expected to find at least one AssetBlend at time 1.5f.";
        ASSERT_EQ(value.m_assetBlends.at(0).m_assetId, KEY2_ASSET_ID) << "Expected KEY2_ASSET_ID at time 1.5f.";

        m_assetBlendTrack.GetValue(3.5f, value);
        ASSERT_GT(value.m_assetBlends.size(), 0) << "Expected to find at least one AssetBlend at time 3.0f.";
        ASSERT_EQ(value.m_assetBlends.at(0).m_assetId, KEY3_ASSET_ID) << "Expected KEY3_ASSET_ID at time 3.0f.";
    };

}; // namespace AssetBlendTrackTest

#endif // !defined(_RELEASE)
