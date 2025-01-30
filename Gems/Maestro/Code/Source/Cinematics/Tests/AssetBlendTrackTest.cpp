/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(_RELEASE)

#include <AnimKey.h>
#include <Maestro/Types/AssetBlendKey.h>

#include <Cinematics/AssetBlendTrack.h>

#include <AzTest/AzTest.h>

namespace Maestro
{

    namespace AssetBlendTrackTest
    {
        const AZ::Data::AssetId KEY1_ASSET_ID = AZ::Data::AssetId(AZ::Uuid("{86CE36B5-D996-4CEF-943E-3F12008694E1}"), 1);
        const AZ::Data::AssetId KEY2_ASSET_ID = AZ::Data::AssetId(AZ::Uuid("{94D54D20-BACC-4A60-8A03-0DC9B5033E03}"), 2);
        const AZ::Data::AssetId KEY3_ASSET_ID = AZ::Data::AssetId(AZ::Uuid("{94D54D20-BACC-4A60-8A03-0DC9B5033E03}"), 3);
        const AZ::Data::AssetId ZERO_ASSET_ID = AZ::Data::AssetId();

        // Testing sub-class
        class CAssetBlendTrackTest : public ::testing::Test
        {
        public:
            CAssetBlendTrackTest();

            void CreateAssetBlendTestKeys();

            CAssetBlendTrack m_assetBlendTrack;

            AZStd::vector<AssetBlend> m_vectorBlends8EvaluatingTo6 = {
                //                  m_assetId,  m_time, m_blendInTime, m_blendOutTime
                AssetBlend(KEY1_ASSET_ID, 0.0f, 0.1f, 0.1f),
                AssetBlend(KEY1_ASSET_ID, 0.0f, 0.1f, 0.1f), // item is to be filtered out due to the ambiguous key time
                AssetBlend(ZERO_ASSET_ID, 0.5f, 0.1f, 0.1f), // item is to be filtered out due to the invalid asset Id
                AssetBlend(KEY1_ASSET_ID, 0.5f, 0.1f, 0.1f),
                AssetBlend(KEY2_ASSET_ID, 1.0f, 0.1f, 0.1f),
                AssetBlend(KEY2_ASSET_ID, 1.5f, 0.1f, 0.1f),
                AssetBlend(
                    KEY3_ASSET_ID,
                    2.5f,
                    0.3f,
                    0.3f), // item is to be sorted down, its duration cannot be determined other than 0.3f + 0.3f + tolerance
                AssetBlend(KEY3_ASSET_ID, 2.0f, 0.1f, 0.1f)
            };

            AssetBlends<AZ::Data::AssetData> m_AssetBlends8EvaluatingTo6 = {
                m_vectorBlends8EvaluatingTo6 // AZStd::vector<AssetBlend> m_assetBlends
            };
        };

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

        // Tests

        // Asset Blend Track
        TEST_F(CAssetBlendTrackTest, Maestro_CAssetBlendTrackTest_Test01)
        {
            AssetBlends<AZ::Data::AssetData> value;

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

        TEST_F(CAssetBlendTrackTest, SetValue_EmptyBlends_ExpectNoKeys)
        {
            AssetBlends<AZ::Data::AssetData> emptyAssetBlends;

            m_assetBlendTrack.SetValue(0.f, emptyAssetBlends, false);
            ASSERT_EQ(m_assetBlendTrack.GetNumKeys(), 0) << "Expected no keys.";

            CreateAssetBlendTestKeys();
            m_assetBlendTrack.SetValue(0.f, emptyAssetBlends, true);
            ASSERT_EQ(m_assetBlendTrack.GetNumKeys(), 0) << "Expected no keys.";

            CreateAssetBlendTestKeys();
            m_assetBlendTrack.SetDefaultValue(emptyAssetBlends);
            ASSERT_EQ(m_assetBlendTrack.GetNumKeys(), 0) << "Expected no keys.";
        }

        TEST_F(CAssetBlendTrackTest, SetValue_Default_8Blends_Expect6Keys)
        {
            constexpr float timeOffset = 1.0f;
            m_assetBlendTrack.SetValue(
                timeOffset, m_AssetBlends8EvaluatingTo6, true); // save default blends and then reconstruct keys from these

            AssetBlends<AZ::Data::AssetData> resultingDefaultBlends;
            m_assetBlendTrack.GetDefaultValue(resultingDefaultBlends);
            // Invalid elements (with invalid AssetId) and ambiguous elements (with the repeating AssetId and time key) are filtered out
            ASSERT_EQ(resultingDefaultBlends.m_assetBlends.size(), 6) << "Expected 6 blends, 2 of 8 were to be filtered out.";

            // Setting blends reconstructs keys accordingly, filtering out invalid and ambiguous elements
            auto numKeys = m_assetBlendTrack.GetNumKeys();
            ASSERT_EQ(numKeys, 6) << "Expected 6 keys, 2 of 8 blends were to be filtered out.";

            AZ::IAssetBlendKey key;
            m_assetBlendTrack.GetKey(4, &key);
            ASSERT_LE(fabs(key.time - 2.0f - timeOffset), AZ::Constants::FloatEpsilon) << "Wrong key time.";
            ASSERT_LE(fabs(key.m_duration - 0.5f), AZ::Constants::FloatEpsilon) << "Wrong key duration.";
            m_assetBlendTrack.GetKey(5, &key); // last key
            ASSERT_LE(fabs(key.time - 2.5f - timeOffset), AZ::Constants::FloatEpsilon) << "Wrong key time.";
            ASSERT_LE(fabs(key.m_duration - 0.6f), AZ::Constants::Tolerance + AZ::Constants::FloatEpsilon);
            ASSERT_LE(fabs(m_assetBlendTrack.GetEndTime() - key.m_endTime), AZ::Constants::FloatEpsilon) << "Wrong sequence end time.";

            m_assetBlendTrack.SetValue(0.f, m_AssetBlends8EvaluatingTo6, false); // reconstruct current keys from given blends

            // Setting blends reconstructs keys accordingly, filtering out invalid and ambiguous elements
            numKeys = m_assetBlendTrack.GetNumKeys();
            ASSERT_EQ(numKeys, 6) << "Expected 6 keys, 2 of 8 possible keys were to be filtered out.";

            m_assetBlendTrack.GetKey(4, &key);
            ASSERT_LE(fabs(key.time - 2.0f), AZ::Constants::FloatEpsilon) << "Wrong key time.";
            ASSERT_LE(fabs(key.m_duration - 0.5f), AZ::Constants::FloatEpsilon) << "Wrong key duration.";
            m_assetBlendTrack.GetKey(5, &key); // last key
            ASSERT_LE(fabs(key.time - 2.5f), AZ::Constants::FloatEpsilon) << "Wrong key time.";
            ASSERT_LE(fabs(m_assetBlendTrack.GetEndTime() - key.m_endTime), AZ::Constants::FloatEpsilon) << "Wrong sequence end time.";
        }
    }; // namespace AssetBlendTrackTest

} // namespace Maestro
#endif // !defined(_RELEASE)
