/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/PropertyEditor/PropertyAssetCtrl.hxx>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace UnitTest
{
    class TestPropertyAssetCtrl : public AzToolsFramework::PropertyAssetCtrl
    {
    public:
        bool CanAccept(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType) const
        {
            return CanAcceptAsset(assetId, assetType);
        }
    };

    class PropertyAssetCtrlTest : public ToolsApplicationFixture<>
    {
    };

    TEST_F(PropertyAssetCtrlTest, CanAcceptAsset_EmptySelectableTypes_AcceptsAnyValidAssetType)
    {
        TestPropertyAssetCtrl propertyAssetCtrl;
        const AZ::Data::AssetId assetId(AZ::Uuid::CreateRandom(), 1);
        const AZ::Data::AssetType assetType = AZ::Uuid::CreateRandom();

        EXPECT_TRUE(propertyAssetCtrl.CanAccept(assetId, assetType));
    }

    TEST_F(PropertyAssetCtrlTest, CanAcceptAsset_EmptySelectableTypes_RejectsInvalidAssetData)
    {
        TestPropertyAssetCtrl propertyAssetCtrl;
        const AZ::Data::AssetId assetId(AZ::Uuid::CreateRandom(), 1);
        const AZ::Data::AssetType assetType = AZ::Uuid::CreateRandom();

        EXPECT_FALSE(propertyAssetCtrl.CanAccept(AZ::Data::AssetId(), assetType));
        EXPECT_FALSE(propertyAssetCtrl.CanAccept(assetId, AZ::Data::AssetType::CreateNull()));
    }

    TEST_F(PropertyAssetCtrlTest, CanAcceptAsset_NonEmptySelectableTypes_OnlyAcceptsListedTypes)
    {
        TestPropertyAssetCtrl propertyAssetCtrl;
        const AZ::Data::AssetId assetId(AZ::Uuid::CreateRandom(), 1);
        const AZ::Data::AssetType supportedAssetType = AZ::Uuid::CreateRandom();
        const AZ::Data::AssetType unsupportedAssetType = AZ::Uuid::CreateRandom();
        propertyAssetCtrl.SetSupportedAssetTypes({ supportedAssetType });

        EXPECT_TRUE(propertyAssetCtrl.CanAccept(assetId, supportedAssetType));
        EXPECT_FALSE(propertyAssetCtrl.CanAccept(assetId, unsupportedAssetType));
    }
} // namespace UnitTest
