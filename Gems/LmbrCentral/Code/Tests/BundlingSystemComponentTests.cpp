/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Archive/IArchive.h>

#include <Bundling/BundlingSystemComponent.h>
#include <LmbrCentral/Bundling/BundlingSystemComponentBus.h>
#include <LmbrCentral.h>
#include <ISystem.h>

#include <AzCore/UnitTest/TestTypes.h>


namespace UnitTest
{

    class BundlingSystemComponentFixture :
        public ::testing::Test

    {
    public:
        BundlingSystemComponentFixture() = default;

        bool TestAsset(const char* assetPath)
        {
            AZ::IO::HandleType readHandle;
            AZ::IO::FileIOBase::GetInstance()->Open(assetPath, AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, readHandle);

            bool testResult = (readHandle != AZ::IO::InvalidHandle);
            AZ::IO::FileIOBase::GetInstance()->Close(readHandle);
            return testResult;
        }

        bool TestAssetId(const char* assetPath)
        {
            AZ::Data::AssetId testAssetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(testAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, assetPath, AZ::Data::s_invalidAssetType, false);

            if (!testAssetId.IsValid())
            {
                return false;
            }

            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, testAssetId);
            return assetInfo.m_relativePath == assetPath;
        }
    };

    TEST_F(BundlingSystemComponentFixture, DISABLED_HasBundle_LoadBundles_Success)
    {
        // This asset lives only within LmbrCentral/Assets/Test/Bundle/staticdata.pak which is copied to our
        // cache as test/bundle/staticdata.pak and should be loaded below
        const char testAssetPath[] = "staticdata/csv/bundlingsystemtestgameproperties.csv";

        EXPECT_FALSE(TestAsset(testAssetPath));
        LmbrCentral::BundlingSystemRequestBus::Broadcast(&LmbrCentral::BundlingSystemRequestBus::Events::LoadBundles, "test/bundle", ".pak");
        EXPECT_TRUE(TestAsset(testAssetPath));
        LmbrCentral::BundlingSystemRequestBus::Broadcast(&LmbrCentral::BundlingSystemRequestBus::Events::UnloadBundles);
        EXPECT_FALSE(TestAsset(testAssetPath));
    }

    TEST_F(BundlingSystemComponentFixture, DISABLED_HasBundle_LoadBundlesCatalogChecks_Success)
    {
        // This asset lives only within LmbrCentral/Assets/Test/Bundle/staticdata.pak which is copied to our
        // cache as test/bundle/staticdata.pak and should be loaded below
        // The Pak has a catalog describing the contents which should automatically update our central asset catalog
        const char testAssetPath[] = "staticdata/csv/bundlingsystemtestgameproperties.csv";
        const char noCatalogAsset[] = "staticdata/csv/gameproperties.csv";

        EXPECT_FALSE(TestAssetId(testAssetPath));
        EXPECT_FALSE(TestAssetId(noCatalogAsset));
        LmbrCentral::BundlingSystemRequestBus::Broadcast(&LmbrCentral::BundlingSystemRequestBus::Events::LoadBundles, "test/bundle", ".pak");
        EXPECT_TRUE(TestAssetId(testAssetPath));
        EXPECT_FALSE(TestAssetId(noCatalogAsset));
        EXPECT_TRUE(TestAsset(noCatalogAsset));
        LmbrCentral::BundlingSystemRequestBus::Broadcast(&LmbrCentral::BundlingSystemRequestBus::Events::UnloadBundles);
        EXPECT_FALSE(TestAssetId(testAssetPath));
        EXPECT_FALSE(TestAssetId(noCatalogAsset));
        EXPECT_FALSE(TestAsset(noCatalogAsset));
    }

    TEST_F(BundlingSystemComponentFixture, DISABLED_BundleSystemComponent_SingleUnloadCheckCatalog_Success)
    {
        // This asset lives only within LmbrCentral/Assets/Test/Bundle/staticdata.pak which is copied to our
        // cache as test/bundle/staticdata.pak and should be loaded below
        // The Pak has a catalog describing the contents which should automatically update our central asset catalog
        const char testCSVAsset[] = "staticdata/csv/bundlingsystemtestgameproperties.csv";
        const char testCSVAssetPak[] = "test/bundle/staticdata.pak";

        // This asset lives only within LmbrCentral/Assets/Test/Bundle/ping.pak
        const char testDDSAsset[] = "textures/test/ping.dds";
        const char testDDSAssetPak[] = "test/bundle/ping.pak";

        EXPECT_FALSE(TestAssetId(testCSVAsset));
        EXPECT_FALSE(TestAssetId(testDDSAsset));
        EXPECT_TRUE(gEnv->pCryPak->OpenPack("@products@", testDDSAssetPak));
        EXPECT_FALSE(TestAssetId(testCSVAsset));
        EXPECT_TRUE(TestAssetId(testDDSAsset));
        EXPECT_TRUE(gEnv->pCryPak->ClosePack(testDDSAssetPak));
        EXPECT_FALSE(TestAssetId(testCSVAsset));
        EXPECT_FALSE(TestAssetId(testDDSAsset));
        EXPECT_TRUE(gEnv->pCryPak->OpenPack("@products@", testCSVAssetPak));
        EXPECT_TRUE(TestAssetId(testCSVAsset));
        EXPECT_FALSE(TestAssetId(testDDSAsset));
        EXPECT_TRUE(gEnv->pCryPak->OpenPack("@products@", testDDSAssetPak));
        EXPECT_TRUE(TestAssetId(testCSVAsset));
        EXPECT_TRUE(TestAssetId(testDDSAsset));
        EXPECT_TRUE(gEnv->pCryPak->ClosePack(testDDSAssetPak));
        EXPECT_TRUE(TestAssetId(testCSVAsset));
        EXPECT_FALSE(TestAssetId(testDDSAsset));
        EXPECT_TRUE(gEnv->pCryPak->OpenPack("@products@", testDDSAssetPak));
        EXPECT_TRUE(TestAssetId(testCSVAsset));
        EXPECT_TRUE(TestAssetId(testDDSAsset));
        EXPECT_TRUE(gEnv->pCryPak->ClosePack(testCSVAssetPak));
        EXPECT_FALSE(TestAssetId(testCSVAsset));
        EXPECT_TRUE(TestAssetId(testDDSAsset));
        EXPECT_TRUE(gEnv->pCryPak->ClosePack(testDDSAssetPak));
        EXPECT_FALSE(TestAssetId(testCSVAsset));
        EXPECT_FALSE(TestAssetId(testDDSAsset));
    }

    TEST_F(BundlingSystemComponentFixture, DISABLED_BundleSystemComponent_SingleLoadAndBundleMode_Success)
    {
        // This asset lives only within LmbrCentral/Assets/Test/Bundle/staticdata.pak which is copied to our
        // cache as test/bundle/staticdata.pak and should be loaded below
        // The Pak has a catalog describing the contents which should automatically update our central asset catalog
        const char testCSVAsset[] = "staticdata/csv/bundlingsystemtestgameproperties.csv";

        const char testMTLAsset[] = "materials/water_test.mtl";
        const char testMTLAssetPak[] = "test/TestMaterials.pak";
        EXPECT_FALSE(TestAssetId(testCSVAsset));
        EXPECT_FALSE(TestAssetId(testMTLAsset));
        EXPECT_TRUE(gEnv->pCryPak->OpenPack("@products@", testMTLAssetPak));
        EXPECT_FALSE(TestAssetId(testCSVAsset));
        EXPECT_TRUE(TestAssetId(testMTLAsset));
        LmbrCentral::BundlingSystemRequestBus::Broadcast(&LmbrCentral::BundlingSystemRequestBus::Events::LoadBundles, "test/bundle", ".pak");
        EXPECT_TRUE(TestAssetId(testCSVAsset));
        EXPECT_TRUE(TestAssetId(testMTLAsset));
        LmbrCentral::BundlingSystemRequestBus::Broadcast(&LmbrCentral::BundlingSystemRequestBus::Events::UnloadBundles);
        EXPECT_FALSE(TestAssetId(testCSVAsset));
        EXPECT_TRUE(TestAssetId(testMTLAsset));
        EXPECT_TRUE(gEnv->pCryPak->ClosePack(testMTLAssetPak));
        EXPECT_FALSE(TestAssetId(testCSVAsset));
        EXPECT_FALSE(TestAssetId(testMTLAsset));
    }

    TEST_F(BundlingSystemComponentFixture, DISABLED_BundleSystemComponent_OpenClosePackCount_Match)
    {
        // This asset lives only within LmbrCentral/Assets/Test/Bundle/staticdata.pak which is copied to our
        // cache as test/bundle/staticdata.pak and should be loaded below
        // The Pak has a catalog describing the contents which should automatically update our central asset catalog
        const char testCSVAsset[] = "staticdata/csv/bundlingsystemtestgameproperties.csv";
        const char testCSVAssetPak[] = "test/bundle/staticdata.pak";

        // This asset lives only within LmbrCentral/Assets/Test/Bundle/ping.pak
        const char testDDSAssetPak[] = "test/bundle/ping.pak";

        size_t bundleCount{ 0 };
        LmbrCentral::BundlingSystemRequestBus::BroadcastResult(bundleCount,&LmbrCentral::BundlingSystemRequestBus::Events::GetOpenedBundleCount);
        EXPECT_EQ(bundleCount, 0);
        EXPECT_FALSE(TestAssetId(testCSVAsset));
        EXPECT_TRUE(gEnv->pCryPak->OpenPack("@products@", testDDSAssetPak));
        LmbrCentral::BundlingSystemRequestBus::BroadcastResult(bundleCount, &LmbrCentral::BundlingSystemRequestBus::Events::GetOpenedBundleCount);
        EXPECT_EQ(bundleCount, 1);
        EXPECT_TRUE(gEnv->pCryPak->ClosePack(testDDSAssetPak));
        LmbrCentral::BundlingSystemRequestBus::BroadcastResult(bundleCount, &LmbrCentral::BundlingSystemRequestBus::Events::GetOpenedBundleCount);
        EXPECT_EQ(bundleCount, 0);
        EXPECT_TRUE(gEnv->pCryPak->OpenPack("@products@", testCSVAssetPak));
        LmbrCentral::BundlingSystemRequestBus::BroadcastResult(bundleCount, &LmbrCentral::BundlingSystemRequestBus::Events::GetOpenedBundleCount);
        EXPECT_EQ(bundleCount, 1);
        EXPECT_TRUE(gEnv->pCryPak->OpenPack("@products@", testDDSAssetPak));
        LmbrCentral::BundlingSystemRequestBus::BroadcastResult(bundleCount, &LmbrCentral::BundlingSystemRequestBus::Events::GetOpenedBundleCount);
        EXPECT_EQ(bundleCount, 2);
        EXPECT_TRUE(gEnv->pCryPak->ClosePack(testDDSAssetPak));
        LmbrCentral::BundlingSystemRequestBus::BroadcastResult(bundleCount, &LmbrCentral::BundlingSystemRequestBus::Events::GetOpenedBundleCount);
        EXPECT_EQ(bundleCount, 1);
        EXPECT_TRUE(gEnv->pCryPak->OpenPack("@products@", testDDSAssetPak));
        LmbrCentral::BundlingSystemRequestBus::BroadcastResult(bundleCount, &LmbrCentral::BundlingSystemRequestBus::Events::GetOpenedBundleCount);
        EXPECT_EQ(bundleCount, 2);
        EXPECT_TRUE(gEnv->pCryPak->ClosePack(testCSVAssetPak));
        LmbrCentral::BundlingSystemRequestBus::BroadcastResult(bundleCount, &LmbrCentral::BundlingSystemRequestBus::Events::GetOpenedBundleCount);
        EXPECT_EQ(bundleCount, 1);
        EXPECT_TRUE(gEnv->pCryPak->ClosePack(testDDSAssetPak));
        LmbrCentral::BundlingSystemRequestBus::BroadcastResult(bundleCount, &LmbrCentral::BundlingSystemRequestBus::Events::GetOpenedBundleCount);
        EXPECT_EQ(bundleCount, 0);
    }

    TEST_F(BundlingSystemComponentFixture, DISABLED_BundleSystemComponent_SplitPakTestWithAsset_Success)
    {
        // This asset lives only within LmbrCentral/Assets/Test/SplitBundleTest/splitbundle__1.pak which is a dependent bundle of splitbundle.pak
        const char testDDSAsset_split[] = "textures/milestone2/am_floor_tile_ddna_test.dds.7";
        const char testDDSAssetPak[] = "test/splitbundletest/splitbundle.pak";

        size_t bundleCount{ 0 };
        LmbrCentral::BundlingSystemRequestBus::BroadcastResult(bundleCount, &LmbrCentral::BundlingSystemRequestBus::Events::GetOpenedBundleCount);
        EXPECT_EQ(bundleCount, 0);
        EXPECT_FALSE(TestAssetId(testDDSAsset_split));
        EXPECT_TRUE(gEnv->pCryPak->OpenPack("@products@", testDDSAssetPak));
        LmbrCentral::BundlingSystemRequestBus::BroadcastResult(bundleCount, &LmbrCentral::BundlingSystemRequestBus::Events::GetOpenedBundleCount);
        EXPECT_EQ(bundleCount, 2);
        EXPECT_TRUE(TestAssetId(testDDSAsset_split));
        EXPECT_TRUE(gEnv->pCryPak->ClosePack(testDDSAssetPak));
        LmbrCentral::BundlingSystemRequestBus::BroadcastResult(bundleCount, &LmbrCentral::BundlingSystemRequestBus::Events::GetOpenedBundleCount);
        EXPECT_EQ(bundleCount, 0);
        EXPECT_FALSE(TestAssetId(testDDSAsset_split));

        EXPECT_TRUE(gEnv->pCryPak->OpenPack("@products@", testDDSAssetPak));
        LmbrCentral::BundlingSystemRequestBus::BroadcastResult(bundleCount, &LmbrCentral::BundlingSystemRequestBus::Events::GetOpenedBundleCount);
        EXPECT_EQ(bundleCount, 2);
        EXPECT_TRUE(TestAssetId(testDDSAsset_split));
        EXPECT_TRUE(gEnv->pCryPak->ClosePack(testDDSAssetPak));
        LmbrCentral::BundlingSystemRequestBus::BroadcastResult(bundleCount, &LmbrCentral::BundlingSystemRequestBus::Events::GetOpenedBundleCount);
        EXPECT_EQ(bundleCount, 0);
        EXPECT_FALSE(TestAssetId(testDDSAsset_split));
    }

    // Verify that our bundles using catalogs of the same name work properly
    TEST_F(BundlingSystemComponentFixture, DISABLED_BundleSystemComponent_SharedCatalogName_Success)
    {
        // This bundle was built for PC but is generic and the test should work fine on other platforms
        // gamepropertioessmall_pc.pak has a smaller version of the gameproperties csv
        // gamepropertiesuserrequest_pc.pak has a bigger version of gameproperties and adds userrequest.csv
        const char testGamePropertiesAsset[] = "staticdata/test/gameproperties.csv";
        const char testUserRequestAsset[] = "staticdata/test/userrequest.csv";
        const char testGamePropertiesAssetPak[] = "test/bundle/gamepropertiessmall_pc.pak";
        const char testUserRequestAssetPak[] = "test/bundle/gamepropertiesuserrequest_pc.pak";

        EXPECT_FALSE(TestAssetId(testGamePropertiesAsset));
        EXPECT_FALSE(TestAssetId(testUserRequestAsset));
        EXPECT_TRUE(gEnv->pCryPak->OpenPack("@products@", testGamePropertiesAssetPak));
        EXPECT_TRUE(TestAssetId(testGamePropertiesAsset));
        EXPECT_FALSE(TestAssetId(testUserRequestAsset));
        AZ::Data::AssetId testAssetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(testAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, testGamePropertiesAsset, AZ::Data::s_invalidAssetType, false);

        EXPECT_TRUE(testAssetId.IsValid());

        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, testAssetId);
        EXPECT_NE(assetInfo.m_sizeBytes, 0);
        AZ::u64 assetSize1 = assetInfo.m_sizeBytes;

        EXPECT_TRUE(gEnv->pCryPak->OpenPack("@products@", testUserRequestAssetPak));
        EXPECT_TRUE(TestAssetId(testGamePropertiesAsset));
        EXPECT_TRUE(TestAssetId(testUserRequestAsset));

        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, testAssetId);
        AZ::u64 assetSize2 = assetInfo.m_sizeBytes;
        EXPECT_NE(assetSize1, assetSize2);

        EXPECT_TRUE(gEnv->pCryPak->ClosePack(testUserRequestAssetPak));
        EXPECT_TRUE(TestAssetId(testGamePropertiesAsset));
        EXPECT_FALSE(TestAssetId(testUserRequestAsset));
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, testAssetId);
        EXPECT_EQ(assetSize1, assetInfo.m_sizeBytes);

        EXPECT_TRUE(gEnv->pCryPak->ClosePack(testGamePropertiesAssetPak));
        EXPECT_FALSE(TestAssetId(testGamePropertiesAsset));
        EXPECT_FALSE(TestAssetId(testUserRequestAsset));

        EXPECT_TRUE(gEnv->pCryPak->OpenPack("@products@", testUserRequestAssetPak));

        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, testAssetId);
        AZ::u64 assetSize3 = assetInfo.m_sizeBytes;
        EXPECT_EQ(assetSize3, assetSize2);

        EXPECT_TRUE(gEnv->pCryPak->OpenPack("@products@", testGamePropertiesAssetPak));
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, testAssetId);
        AZ::u64 assetSize4 = assetInfo.m_sizeBytes;
        EXPECT_EQ(assetSize4, assetSize1);
    }
}
