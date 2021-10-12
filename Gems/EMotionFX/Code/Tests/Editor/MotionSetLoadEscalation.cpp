/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetDataStream.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Tests/SystemComponentFixture.h>
#include <Source/Integration/Assets/MotionSetAsset.h>
#include <AzFramework/Asset/AssetSystemBus.h>

namespace EMotionFX
{
    struct RegisterHandler
        : AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Handler
    {
        RegisterHandler()
        {
            BusConnect();
        }

        ~RegisterHandler()
        {
            BusDisconnect();

            for(auto* handler : m_handlers)
            {
                delete handler;
            }
        }

        void RegisterPropertyType(AzToolsFramework::PropertyHandlerBase* pHandler) override
        {
            m_handlers.insert(pHandler);
        }

        void UnregisterPropertyType(AzToolsFramework::PropertyHandlerBase* pHandler) override
        {
            m_handlers.erase(pHandler);
        }

        AzToolsFramework::PropertyHandlerBase* ResolvePropertyHandler([[maybe_unused]] AZ::u32 handlerName, [[maybe_unused]] const AZ::Uuid& handlerType) override
        {
            return {};
        }

        AZStd::unordered_set<AzToolsFramework::PropertyHandlerBase*> m_handlers{};
    };

    struct MotionSetFixture
        : SystemComponentFixtureWithCatalog
    {
        RegisterHandler m_registerHandler;


        void TearDown() override
        {
            SystemComponentFixtureWithCatalog::TearDown();

            m_app.Stop();
        }
    };

    struct MockAssetSystemRequests
        : AzFramework::AssetSystemRequestBus::Handler
    {
        MockAssetSystemRequests()
        {
            BusConnect();
        }

        ~MockAssetSystemRequests() override
        {
            BusDisconnect();
        }


        MOCK_METHOD1(CompileAssetSync, AzFramework::AssetSystem::AssetStatus(const AZStd::string& assetPath));
        MOCK_METHOD1(EscalateAssetBySearchTerm, bool(AZStd::string_view searchTerm));

        MOCK_METHOD1(EscalateAssetByUuid, bool (const AZ::Uuid&));
        MOCK_METHOD1(CompileAssetSync_FlushIO, AzFramework::AssetSystem::AssetStatus (const AZStd::string&));
        MOCK_METHOD1(CompileAssetSyncById, AzFramework::AssetSystem::AssetStatus (const AZ::Data::AssetId&));
        MOCK_METHOD1(CompileAssetSyncById_FlushIO, AzFramework::AssetSystem::AssetStatus (const AZ::Data::AssetId&));
        MOCK_METHOD4(ConfigureSocketConnection, bool (const AZStd::string&, const AZStd::string&, const AZStd::string&, const AZStd::string&));
        MOCK_METHOD1(Connect, bool (const char*));
        MOCK_METHOD2(ConnectWithTimeout, bool (const char*, AZStd::chrono::duration<float>));
        MOCK_METHOD0(Disconnect, bool ());
        MOCK_METHOD0(GetAssetProcessorPingTimeMilliseconds, float ());
        MOCK_METHOD1(GetAssetStatus, AzFramework::AssetSystem::AssetStatus (const AZStd::string&));
        MOCK_METHOD1(GetAssetStatus_FlushIO, AzFramework::AssetSystem::AssetStatus (const AZStd::string&));
        MOCK_METHOD2(GetAssetStatusSearchType, AzFramework::AssetSystem::AssetStatus(const AZStd::string&, int));
        MOCK_METHOD2(GetAssetStatusSearchType_FlushIO, AzFramework::AssetSystem::AssetStatus(const AZStd::string&, int));
        MOCK_METHOD1(GetAssetStatusById, AzFramework::AssetSystem::AssetStatus (const AZ::Data::AssetId&));
        MOCK_METHOD1(GetAssetStatusById_FlushIO, AzFramework::AssetSystem::AssetStatus (const AZ::Data::AssetId&));
        MOCK_METHOD3(GetUnresolvedProductReferences, void (AZ::Data::AssetId, AZ::u32&, AZ::u32&));
        MOCK_METHOD0(SaveCatalog, bool ());
        MOCK_METHOD1(SetAssetProcessorIP, void (const AZStd::string&));
        MOCK_METHOD1(SetAssetProcessorPort, void (AZ::u16));
        MOCK_METHOD1(SetBranchToken, void (const AZStd::string&));
        MOCK_METHOD1(SetProjectName, void (const AZStd::string&));
        MOCK_METHOD0(ShowAssetProcessor, void());
        MOCK_METHOD1(ShowInAssetProcessor, void(const AZStd::string&));
        MOCK_METHOD1(WaitUntilAssetProcessorReady, bool(AZStd::chrono::duration<float>));
        MOCK_METHOD1(WaitUntilAssetProcessorConnected, bool(AZStd::chrono::duration<float>));
        MOCK_METHOD1(WaitUntilAssetProcessorDisconnected, bool(AZStd::chrono::duration<float>));
        MOCK_METHOD0(AssetProcessorIsReady, bool());
        MOCK_METHOD0(ConnectedWithAssetProcessor, bool());
        MOCK_METHOD0(DisconnectedWithAssetProcessor, bool());
        MOCK_METHOD0(NegotiationWithAssetProcessorFailed, bool());
        MOCK_METHOD0(StartDisconnectingAssetProcessor, void());
        MOCK_METHOD1(EstablishAssetProcessorConnection, bool(const AzFramework::AssetSystem::ConnectionSettings&));

        MOCK_METHOD3(AppendAssetToPrioritySet, bool (const AZStd::string&, const AZ::Uuid&, uint32_t));
        MOCK_METHOD3(AppendAssetsToPrioritySet, bool (const AZStd::string&, const AZStd::vector<AZ::Uuid>&, uint32_t));
        MOCK_METHOD2(RemoveAssetFromPrioritySet, bool (const AZStd::string&, const AZ::Uuid&));
        MOCK_METHOD2(RemoveAssetsFromPrioritySet, bool (const AZStd::string&, const AZStd::vector<AZ::Uuid>&));
    };

    TEST_F(MotionSetFixture, MeshLoadTest)
    {
        using testing::_;

        const AZStd::string fileName = "@engroot@/Gems/EMotionFX/Code/Tests/TestAssets/EMotionFXBuilderTestAssets/MotionSetExample.motionset";

        MockAssetSystemRequests assetSystem;
        EXPECT_CALL(assetSystem, CompileAssetSync(_))
            .Times(2)
            .WillRepeatedly(testing::Return(AzFramework::AssetSystem::AssetStatus_Queued));
        EXPECT_CALL(assetSystem, EscalateAssetBySearchTerm(_))
            .Times(2)
            .WillRepeatedly(testing::Return(false));

        auto* handler = static_cast<Integration::MotionSetAssetHandler*>(AZ::Data::AssetManager::Instance().GetHandler(azrtti_typeid<Integration::MotionSetAsset>()));
        AZ::Data::AssetPtr assetPointer = handler->CreateAsset(AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0), AZ::Data::AssetType::CreateRandom());
        
        AZ::Data::Asset<AZ::Data::AssetData> asset(assetPointer, AZ::Data::AssetLoadBehavior::NoLoad);
        AZ::u64 fileLength = 0;
        AZ::IO::FileIOBase::GetInstance()->Size(fileName.c_str(), fileLength);
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream = AZStd::make_shared<AZ::Data::AssetDataStream>();
        stream->Open(fileName, 0, fileLength);
        stream->BlockUntilLoadComplete();
        handler->LoadAssetData(asset, stream, &AZ::Data::AssetFilterNoAssetLoading);
        handler->OnInitAsset(asset);
    }
} // end namespace EMotionFX
