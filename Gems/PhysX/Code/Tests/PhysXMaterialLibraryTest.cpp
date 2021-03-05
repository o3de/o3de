/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <PhysX_precompiled.h>

#include <AzTest/AzTest.h>
#include <PhysX/SystemComponentBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/PhysicsSystem.h>

namespace PhysX
{
    class MaterialLibraryTest_MockCatalog
        : public AZ::Data::AssetCatalog
        , public AZ::Data::AssetCatalogRequestBus::Handler
    {

    private:
        AZ::Uuid m_randomUuid = AZ::Uuid::CreateRandom();
        AZStd::vector<AZ::Data::AssetId> m_mockAssetIds;

    public:
        AZ_CLASS_ALLOCATOR(MaterialLibraryTest_MockCatalog, AZ::SystemAllocator, 0);

        MaterialLibraryTest_MockCatalog()
        {
            AZ::Data::AssetCatalogRequestBus::Handler::BusConnect();
        }

        ~MaterialLibraryTest_MockCatalog()
        {
            AZ::Data::AssetCatalogRequestBus::Handler::BusDisconnect();
        }

        AZ::Data::AssetId GenerateMockAssetId()
        {
            AZ::Data::AssetId assetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);
            m_mockAssetIds.push_back(assetId);
            return assetId;
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetCatalogRequestBus
        AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& id) override
        {
            AZ::Data::AssetInfo result;
            result.m_assetType = AZ::AzTypeInfo<Physics::MaterialLibraryAsset>::Uuid();
            auto foundId = AZStd::find(m_mockAssetIds.begin(), m_mockAssetIds.end(), id);
            if (foundId != m_mockAssetIds.end())
            {
                result.m_assetId = *foundId;
            }

            return result;
        }
        //////////////////////////////////////////////////////////////////////////

        AZ::Data::AssetStreamInfo GetStreamInfoForLoad(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override
        {
            EXPECT_TRUE(type == AZ::AzTypeInfo<Physics::MaterialLibraryAsset>::Uuid());
            AZ::Data::AssetStreamInfo info;
            info.m_dataOffset = 0;
            info.m_streamFlags = AZ::IO::OpenMode::ModeRead;

            for (int i = 0; i < m_mockAssetIds.size(); ++i)
            {
                if (m_mockAssetIds[i] == id)
                {
                    info.m_streamName = AZStd::string::format("MaterialLibraryAssetName%d", i);
                }
            }

            if (!info.m_streamName.empty())
            {
                // this ensures tha parallel running unit tests do not overlap their files that they use.
                AZStd::string fullName = AZStd::string::format("%s-%s", m_randomUuid.ToString<AZStd::string>().c_str(), info.m_streamName.c_str());
                info.m_streamName = fullName;
                info.m_dataLen = static_cast<size_t>(AZ::IO::SystemFile::Length(info.m_streamName.c_str()));
            }
            else
            {
                info.m_dataLen = 0;
            }

            return info;
        }

        AZ::Data::AssetStreamInfo GetStreamInfoForSave(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override
        {
            AZ::Data::AssetStreamInfo info;
            info = GetStreamInfoForLoad(id, type);
            info.m_streamFlags = AZ::IO::OpenMode::ModeWrite;
            return info;
        }

        bool SaveAsset(AZ::Data::Asset<Physics::MaterialLibraryAsset>& asset)
        {
            volatile bool isDone = false;
            volatile bool succeeded = false;
            AZ::Data::AssetBusCallbacks callbacks;
            callbacks.SetCallbacks(nullptr, nullptr, nullptr,
                [&isDone, &succeeded](const AZ::Data::Asset<AZ::Data::AssetData>& /*asset*/, bool isSuccessful, AZ::Data::AssetBusCallbacks& /*callbacks*/)
                {
                    isDone = true;
                    succeeded = isSuccessful;
                }, nullptr, nullptr, nullptr);

            callbacks.BusConnect(asset.GetId());
            asset.Save();

            while (!isDone)
            {
                AZ::Data::AssetManager::Instance().DispatchEvents();
            }
            return succeeded;
        }
    };

    class DISABLED_PhysXMaterialLibraryTest
        : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_catalog = AZStd::make_unique<MaterialLibraryTest_MockCatalog>();
            AZ::Data::AssetManager::Instance().RegisterCatalog(m_catalog.get(), AZ::AzTypeInfo<Physics::MaterialLibraryAsset>::Uuid());
        }

        void TearDown() override
        {
            AZ::Data::AssetManager::Instance().UnregisterCatalog(m_catalog.get());
        }

        AZStd::unique_ptr<MaterialLibraryTest_MockCatalog> m_catalog;
    };
    
    TEST_F(DISABLED_PhysXMaterialLibraryTest, DISABLED_DefaultMaterialLibrary_CorrectMaterialLibraryIsInferred)
    {
        AZ::Data::Asset<Physics::MaterialLibraryAsset> materialLibrary = AZ::Interface<AzPhysics::SystemInterface>::Get()->GetDefaultMaterialLibrary();

        AZ::Data::AssetId dummyAssetId = AZ::Data::AssetId(AZ::Uuid::CreateName("DummyLibrary.physmaterial"));
        AZ::Data::Asset<Physics::MaterialLibraryAsset> dummyMaterialLibAsset = AZ::Data::AssetManager::Instance().GetAsset<Physics::MaterialLibraryAsset>(dummyAssetId, AZ::Data::AssetLoadBehavior::Default);
        materialLibrary = dummyMaterialLibAsset;
        AZ::Interface<AzPhysics::SystemInterface>::Get()->UpdateDefaultMaterialLibrary(materialLibrary);

        // We must have now a default material library setup
        ASSERT_TRUE(materialLibrary.GetId().IsValid());

        AZ::Data::AssetId otherDummyAssetId = AZ::Data::AssetId(AZ::Uuid::CreateName("OtherDummyLibrary.physmaterial"));
        AZ::Data::Asset<Physics::MaterialLibraryAsset> otherDummyMaterialLibAsset = AZ::Data::AssetManager::Instance().GetAsset<Physics::MaterialLibraryAsset>(otherDummyAssetId, AZ::Data::AssetLoadBehavior::Default);

        // Set selection's material library to a different one than default material library
        Physics::MaterialSelection selectionTest;
        selectionTest.SetMaterialLibrary(otherDummyAssetId);

        ASSERT_TRUE(selectionTest.GetMaterialLibraryAssetId().IsValid());
        ASSERT_EQ(selectionTest.GetMaterialLibraryAssetId(), selectionTest.GetMaterialLibraryAssetId());
        ASSERT_NE(selectionTest.GetMaterialLibraryAssetId(), materialLibrary.GetId());

        // By reseting the selection, now it should infer to the default material library set in the global configuration
        selectionTest.ResetToDefaultMaterialLibrary();

        ASSERT_TRUE(selectionTest.GetMaterialLibraryAssetId().IsValid());
        ASSERT_EQ(selectionTest.GetMaterialLibraryAssetId(), materialLibrary.GetId());

        // Release material library so we exit gracefully
        materialLibrary = {};
        AZ::Interface<AzPhysics::SystemInterface>::Get()->UpdateDefaultMaterialLibrary(materialLibrary);
    }
}
