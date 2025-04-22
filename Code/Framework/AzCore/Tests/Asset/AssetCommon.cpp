/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetInternal/WeakAsset.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::Data;

    using AssetIdTest = LeakDetectionFixture;

    TEST_F(AssetIdTest, AssetId_PrintDecimalSubId_SubIdIsDecimal)
    {
        // Arbitrary GUID, sub ID picked that will be different in decimal and hexadecimal.
        AssetId id("{A9F596D7-9913-4BA4-AD4E-7E477FB9B542}", 20);
        AZStd::string asString = id.ToString<AZStd::string>(AZ::Data::AssetId::SubIdDisplayType::Decimal);

        ASSERT_EQ(asString.size(), 41);
        ASSERT_EQ(asString[39], '2');
        ASSERT_EQ(asString[40], '0');
    }

    TEST_F(AssetIdTest, AssetId_PrintHexadecimalSubId_SubIdIsHex)
    {
        // Arbitrary GUID, sub ID picked that will be different in decimal and hexadecimal.
        AssetId id("{A9F596D7-9913-4BA4-AD4E-7E477FB9B542}", 20);
        AZStd::string asString = id.ToString<AZStd::string>(AZ::Data::AssetId::SubIdDisplayType::Hex);
        ASSERT_EQ(asString.size(), 41);

        ASSERT_EQ(asString[39], '1');
        ASSERT_EQ(asString[40], '4');
    }

    TEST_F(AssetIdTest, AssetIdLessThanOperator_LHSEqualsRHS_ReturnsFalse)
    {
        AssetId left("{88888888-4444-4444-4444-CCCCCCCCCCCC}", 1);
        AssetId right("{88888888-4444-4444-4444-CCCCCCCCCCCC}", 1);
        ASSERT_FALSE(left < right);
    }

    TEST_F(AssetIdTest, AssetIdLessThanOperator_GuidsEqualLHSSubIdLessThanRHS_ReturnsTrue)
    {
        AssetId left("{EEEEEEEE-EEEE-BBBB-BBBB-CCCCCCCCCCCC}", 0);
        AssetId right("{EEEEEEEE-EEEE-BBBB-BBBB-CCCCCCCCCCCC}", 1);
        ASSERT_TRUE(left < right);
    }

    TEST_F(AssetIdTest, AssetIdLessThanOperator_GuidsEqualLHSSubIdGreaterThanRHS_ReturnsFalse)
    {
        AssetId left("{66666666-2222-4444-3333-CCCCCCCCCCCC}", 4);
        AssetId right("{66666666-2222-4444-3333-CCCCCCCCCCCC}", 2);
        ASSERT_FALSE(left < right);
    }

    TEST_F(AssetIdTest, AssetIdLessThanOperator_LHSGuidLessThanRHS_ReturnsTrue)
    {
        AssetId left("{00000000-4444-4444-4444-CCCCCCCCCCCC}", 1);
        AssetId right("{10000000-4444-4444-4444-CCCCCCCCCCCC}", 1);
        ASSERT_TRUE(left < right);
    }

    TEST_F(AssetIdTest, AssetIdLessThanOperator_LHSGuidGreaterThanRHS_ReturnsFalse)
    {
        AssetId left("{10200000-4444-4444-4444-CCCCCCCCCCCC}", 1);
        AssetId right("{10000000-4444-4444-4444-CCCCCCCCCCCC}", 1);
        ASSERT_FALSE(left < right);
    }

    TEST_F(AssetIdTest, ToFixedString_ResultIsAccurate_Succeeds)
    {
        AssetId id("{A9F596D7-9913-4BA4-AD4E-7E477FB9B542}", 0xFEDC1234);
        const AZStd::string dynamic = id.ToString<AZStd::string>(AZ::Data::AssetId::SubIdDisplayType::Hex);
        const AssetId::FixedString fixed = id.ToFixedString();
        EXPECT_STREQ(dynamic.c_str(), fixed.c_str());
    }

    TEST_F(AssetIdTest, ToFixedString_FormatSpecifier_Succeeds)
    {
        AssetId source("{A9F596D7-9913-4BA4-AD4E-7E477FB9B542}", 0xFEDC1234);
        const AZStd::string dynamic = AZStd::string::format("%s", source.ToString<AZStd::string>().c_str());
        const AZStd::string fixed = AZStd::string::format("%s", source.ToFixedString().c_str());
        EXPECT_EQ(dynamic, fixed);
    }

    TEST_F(AssetIdTest, CreateString_SubIdIsNotNegative_Succeeds)
    {
        auto assetId = AssetId::CreateString("{A9F596D7-FFFF-4BA4-AD4E-7E477FB9B542}:0xFFFFFFFF");
        AZStd::string asString = assetId.ToString<AZStd::string>(AZ::Data::AssetId::SubIdDisplayType::Decimal);
        auto subId = asString.substr(asString.find_first_of(':') + 1);
        EXPECT_STRCASEEQ(subId.c_str(), "4294967295");
    }

    using AssetTest = LeakDetectionFixture;

    TEST_F(AssetTest, AssetPreserveHintTest_Const_Copy)
    {
        // test to make sure that when we copy asset<T>s around using copy operations
        // that the asset Hint is preserved in the case of assets being copied from things missing asset hints.
        AssetId id("{52C79B55-B5AA-4841-AFC8-683D77716287}", 1);
        AssetId idWithDifferentAssetId("{EA554205-D887-4A01-9E39-A318DDE4C0FC}", 1);

        AssetType typeOfExample("{A99E8722-1F1D-4CA9-B89B-921EB3D907A9}");
        Asset<AssetData> assetWithHint(id, typeOfExample, "an asset hint");
        Asset<AssetData> differentAssetEntirely(idWithDifferentAssetId, typeOfExample, "");
        Asset<AssetData> sameAssetWithoutHint(id, typeOfExample, "");
        Asset<AssetData> sameAssetWithDifferentHint(id, typeOfExample, "a different hint");

        // if we copy an asset from one with the same id, but no hint, preserve the sources hint.
        assetWithHint = sameAssetWithoutHint;
        ASSERT_STREQ(assetWithHint.GetHint().c_str(), "an asset hint");

        // if we copy from an asset with same id, but with a different hint, we do not preserve the hint.
        assetWithHint = sameAssetWithDifferentHint;
        ASSERT_STREQ(assetWithHint.GetHint().c_str(), "a different hint");

        // if we copy different assets (different id or sub) the hint must be copied.
        // even if its empty.
        assetWithHint = Asset<AssetData>(id, typeOfExample, "an asset hint");
        assetWithHint = differentAssetEntirely;
        ASSERT_STREQ(assetWithHint.GetHint().c_str(), "");

        // ensure copy construction copies the hint.
        Asset<AssetData> copied(sameAssetWithDifferentHint);
        ASSERT_STREQ(copied.GetHint().c_str(), "a different hint");
    }

    TEST_F(AssetTest, AssetPreserveHintTest_Rvalue_Ref_Move)
    {
        // test to make sure that when we move asset<T>s around using move operators 
        // that the asset Hint is preserved in the case of assets being moved from things missing asset hints.
        AssetId id("{52C79B55-B5AA-4841-AFC8-683D77716287}", 1);
        AssetId idWithDifferentAssetId("{EA554205-D887-4A01-9E39-A318DDE4C0FC}", 1);

        AssetType typeOfExample("{A99E8722-1F1D-4CA9-B89B-921EB3D907A9}");
        Asset<AssetData> assetWithHint(id, typeOfExample, "an asset hint");
        Asset<AssetData> differentAssetEntirely(idWithDifferentAssetId, typeOfExample, "");
        Asset<AssetData> sameAssetWithoutHint(id, typeOfExample, "");
        Asset<AssetData> sameAssetWithDifferentHint(id, typeOfExample, "a different hint");

        // if we move an asset from one with the same id, but no hint, preserve the sources hint.
        assetWithHint = AZStd::move(sameAssetWithoutHint);
        ASSERT_STREQ(assetWithHint.GetHint().c_str(), "an asset hint");

        // if we move from an asset with same id, but with a different hint, we do not preserve the hint.
        assetWithHint = AZStd::move(sameAssetWithDifferentHint);
        ASSERT_STREQ(assetWithHint.GetHint().c_str(), "a different hint");

        // if we move different assets (different id or sub) the hint must be copied.
        // even if its empty.
        assetWithHint = Asset<AssetData>(id, typeOfExample, "an asset hint");
        assetWithHint = AZStd::move(differentAssetEntirely);
        ASSERT_STREQ(assetWithHint.GetHint().c_str(), "");

        // ensure move construction copies the hint.
        Asset<AssetData> copied(Asset<AssetData>(id, typeOfExample, "a different hint"));
        ASSERT_STREQ(copied.GetHint().c_str(), "a different hint");
    }


    TEST_F(LeakDetectionFixture, AssetReleaseRetainsAssetState_Id_Type_Hint)
    {
        const AssetId id("{52C79B55-B5AA-4841-AFC8-683D77716287}", 1);
        const AssetType type("{A99E8722-1F1D-4CA9-B89B-921EB3D907A9}");
        const AZStd::string hint("an asset hint");

        Asset<AssetData> assetWithHint(id, type, hint);

        assetWithHint.Release();

        EXPECT_EQ(assetWithHint.GetId(), id);
        EXPECT_EQ(assetWithHint.GetType(), type);
        EXPECT_EQ(assetWithHint.GetHint(), hint);
    }

    TEST_F(LeakDetectionFixture, AssetResetDefaultsAssetState_Id_Type_Hint)
    {
        const AssetId id("{52C79B55-B5AA-4841-AFC8-683D77716287}", 1);
        const AssetType type("{A99E8722-1F1D-4CA9-B89B-921EB3D907A9}");
        const AZStd::string hint("an asset hint");

        Asset<AssetData> assetWithHint(id, type, hint);

        assetWithHint.Reset();

        EXPECT_EQ(assetWithHint.GetId(), AssetId());
        EXPECT_EQ(assetWithHint.GetType(), AssetType::CreateNull());
        EXPECT_EQ(assetWithHint.GetHint(), AZStd::string{});
    }

    using WeakAssetTest = LeakDetectionFixture;

    // Expose the internal weak use count of AssetData for verification in unit tests.
    class TestAssetData : public AssetData
    {
    public:
        static inline const AssetId ArbitraryAssetId{ "{E14BD18D-A933-4CBD-B64E-25F14D5E69E4}", 1 };

        TestAssetData(const AssetId& assetId = ArbitraryAssetId) : AssetData(assetId) {}
        int GetWeakUseCount() { return m_weakUseCount.load(); }
    };

    // Asset cancellation is temporarily disabled, re-enable this test when cancellation is more stable
    TEST_F(WeakAssetTest, DISABLED_WeakAsset_ConstructionAndDestruction_UpdatesAssetDataWeakRefCount)
    {
        TestAssetData testData;
        EXPECT_EQ(testData.GetWeakUseCount(), 0);

        // When transitioning the weak use count from 1 to 0, one assert will fire due to the asset manager not being initialized.
        AZ_TEST_START_TRACE_SUPPRESSION;
        {
            AssetInternal::WeakAsset<TestAssetData> weakAsset(&testData, AssetLoadBehavior::Default);
            EXPECT_EQ(testData.GetWeakUseCount(), 1);
        }
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        EXPECT_EQ(testData.GetWeakUseCount(), 0);
    }

    // Asset cancellation is temporarily disabled, re-enable this test when cancellation is more stable
    TEST_F(WeakAssetTest, DISABLED_WeakAsset_MoveOperatorWithDifferentData_UpdatesOldAssetDataWeakRefCount)
    {
        TestAssetData testData;
        EXPECT_EQ(testData.GetWeakUseCount(), 0);

        AssetInternal::WeakAsset<TestAssetData> weakAsset(&testData, AssetLoadBehavior::Default);
        EXPECT_EQ(testData.GetWeakUseCount(), 1);

        // When transitioning the weak use count from 1 to 0, one assert will fire due to the asset manager not being initialized.
        AZ_TEST_START_TRACE_SUPPRESSION;
        weakAsset = {};
        EXPECT_EQ(testData.GetWeakUseCount(), 0);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    // Asset cancellation is temporarily disabled, re-enable this test when cancellation is more stable
    TEST_F(WeakAssetTest, DISABLED_WeakAsset_MoveOperatorWithSameData_PreservesAssetDataWeakRefCount)
    {
        TestAssetData testData;
        EXPECT_EQ(testData.GetWeakUseCount(), 0);

        // When transitioning the weak use count from 1 to 0, one assert will fire due to the asset manager not being initialized.
        AZ_TEST_START_TRACE_SUPPRESSION;
        {
            AssetInternal::WeakAsset<TestAssetData> weakAsset(&testData, AssetLoadBehavior::Default);
            EXPECT_EQ(testData.GetWeakUseCount(), 1);

            weakAsset = { &testData, AssetLoadBehavior::Default };
            EXPECT_EQ(testData.GetWeakUseCount(), 1);
        }
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    // Asset cancellation is temporarily disabled, re-enable this test when cancellation is more stable
    TEST_F(WeakAssetTest, DISABLED_WeakAsset_AssignmentOperator_CopiesDataAndIncrementsWeakRefCount)
    {
        TestAssetData testData;
        EXPECT_EQ(testData.GetWeakUseCount(), 0);

        // When transitioning the weak use count from 1 to 0, one assert will fire due to the asset manager not being initialized.
        AZ_TEST_START_TRACE_SUPPRESSION;
        {
            const AssetInternal::WeakAsset<TestAssetData> weakAsset(&testData, AssetLoadBehavior::PreLoad);
            EXPECT_EQ(testData.GetWeakUseCount(), 1);

            AssetInternal::WeakAsset<TestAssetData> weakAsset2;

            weakAsset2 = weakAsset;
            EXPECT_EQ(testData.GetWeakUseCount(), 2);
            EXPECT_EQ(weakAsset.GetId(), weakAsset2.GetId());
        }
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }
}

