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

#include "LmbrCentral_precompiled.h"

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <Rendering/MeshAssetHandler.h>


#include "Source/Rendering/EditorMeshComponent.h"

namespace UnitTest
{
    // base physical entity which can be derived from to detect other specific use-cases
    struct PhysicalEntityPlaceHolder
        : public IPhysicalEntity
    {
        pe_type GetType() const override { return PE_NONE; } 
        int AddRef() override { return 0; }
        int Release() override { return 0; }
        int SetParams([[maybe_unused]] const pe_params* params, [[maybe_unused]] int bThreadSafe = 0) override { return 0; }
        int GetParams([[maybe_unused]] pe_params* params) const override { return 0; } 
        int GetStatus([[maybe_unused]] pe_status* status) const override { return 0; } 
        int Action(const pe_action*, [[maybe_unused]] int bThreadSafe = 0) override { return 0; } 
        int AddGeometry([[maybe_unused]] phys_geometry* pgeom, [[maybe_unused]] pe_geomparams* params, [[maybe_unused]] int id = -1, [[maybe_unused]] int bThreadSafe = 0) override { return 0; }
        void RemoveGeometry([[maybe_unused]] int id, [[maybe_unused]] int bThreadSafe = 0) override {}
        PhysicsForeignData GetForeignData([[maybe_unused]] int itype = 0) const override { return PhysicsForeignData{}; }
        int GetiForeignData() const override { return 0; }
        int GetStateSnapshot([[maybe_unused]] class CStream& stm, [[maybe_unused]] float time_back = 0, [[maybe_unused]] int flags = 0) override { return 0; }
        int GetStateSnapshot([[maybe_unused]] TSerialize ser, [[maybe_unused]] float time_back = 0, [[maybe_unused]] int flags = 0) override { return 0; }
        int SetStateFromSnapshot([[maybe_unused]] class CStream& stm, [[maybe_unused]] int flags = 0) override { return 0; }
        int PostSetStateFromSnapshot() override { return 0; }
        unsigned int GetStateChecksum() override { return 0; }
        void SetNetworkAuthority([[maybe_unused]] int authoritive = -1, [[maybe_unused]] int paused = -1) override {}
        int SetStateFromSnapshot([[maybe_unused]] TSerialize ser, [[maybe_unused]] int flags = 0) override { return 0; }
        int SetStateFromTypedSnapshot([[maybe_unused]] TSerialize ser, [[maybe_unused]] int type, [[maybe_unused]] int flags = 0) override { return 0; }
        int GetStateSnapshotTxt([[maybe_unused]] char* txtbuf, [[maybe_unused]] int szbuf, [[maybe_unused]] float time_back = 0) override { return 0; }
        void SetStateFromSnapshotTxt([[maybe_unused]] const char* txtbuf, [[maybe_unused]] int szbuf) override {}
        int DoStep([[maybe_unused]] float time_interval) override { return 0; }
        int DoStep([[maybe_unused]] float time_interval, [[maybe_unused]] int iCaller) override { return 0; }
        void StartStep([[maybe_unused]] float time_interval) override {}
        void StepBack([[maybe_unused]] float time_interval) override {}
        void GetMemoryStatistics([[maybe_unused]] ICrySizer* pSizer) const override {}
    };

    // special test fake to validate incoming pe_params
    struct PhysicalEntitySetParamsCheck
        : public PhysicalEntityPlaceHolder
    {
        int SetParams(const pe_params* params, [[maybe_unused]] int bThreadSafe = 0) override
        {
            if (params->type == pe_params_pos::type_id)
            {
                pe_params_pos* params_pos = (pe_params_pos*)params;

                Vec3 s;
                if (Matrix34* m34 = params_pos->pMtx3x4)
                {
                    s.Set(m34->GetColumn(0).len(), m34->GetColumn(1).len(), m34->GetColumn(2).len());
                    Matrix33 m33(m34->GetColumn(0) / s.x, m34->GetColumn(1) / s.y, m34->GetColumn(2) / s.z);
                    // ensure passed in params_pos->pMtx3x4 is orthonormal
                    // ref - see Cry_Quat.h - explicit ILINE Quat_tpl<F>(const Matrix33_tpl<F>&m)
                    m_isOrthonormal = m33.IsOrthonormalRH(0.1f);
                }
            }

            return 0;
        }

        bool m_isOrthonormal = false;
    };

    class TestEditorMeshComponent
        : public LmbrCentral::EditorMeshComponent
    {
    public:
        AZ_EDITOR_COMPONENT(TestEditorMeshComponent, "{6C6B593A-1946-4239-AE16-E8B96D9835E5}", LmbrCentral::EditorMeshComponent)

        static void Reflect(AZ::ReflectContext* context);

        TestEditorMeshComponent() = default;

    };

    void TestEditorMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TestEditorMeshComponent>()
                ->Version(0);
        }
    }

    class EditorMeshComponentTestFixture
        : public ToolsApplicationFixture
    {
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_testMeshComponentDescriptor;

    public:
        void SetUpEditorFixtureImpl() override
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            m_testMeshComponentDescriptor =
                AZStd::unique_ptr<AZ::ComponentDescriptor>(TestEditorMeshComponent::CreateDescriptor());
            m_testMeshComponentDescriptor->Reflect(serializeContext);
        }

        void TearDownEditorFixtureImpl() override
        {
            m_testMeshComponentDescriptor.reset();
        }
    };

    struct MeshAssetHandlerFixture
        : ScopedAllocatorSetupFixture
    {
    protected:
        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            AZ::Data::AssetManager::Create(AZ::Data::AssetManager::Descriptor());
            AZ::Data::AssetManager::Instance().SetAssetInfoUpgradingEnabled(false);

            m_handler.Register();
        }

        void TearDown() override
        {
            m_handler.Unregister();
            AZ::Data::AssetManager::Destroy();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
        }

        LmbrCentral::MeshAssetHandler m_handler;
    };

    struct MockAssetSystemRequestHandler
        : AzFramework::AssetSystemRequestBus::Handler
    {
        MockAssetSystemRequestHandler()
        {
            BusConnect();
        }

        ~MockAssetSystemRequestHandler()
        {
            BusDisconnect();
        }

        AzFramework::AssetSystem::AssetStatus GetAssetStatusById([[maybe_unused]] const AZ::Data::AssetId& assetId) override
        {
            m_statusRequest = true;

            return AzFramework::AssetSystem::AssetStatus_Queued;
        }

        MOCK_METHOD1(CompileAssetSync, AzFramework::AssetSystem::AssetStatus (const AZStd::string&));
        MOCK_METHOD1(CompileAssetSync_FlushIO, AzFramework::AssetSystem::AssetStatus (const AZStd::string&));
        MOCK_METHOD1(CompileAssetSyncById, AzFramework::AssetSystem::AssetStatus (const AZ::Data::AssetId&));
        MOCK_METHOD1(CompileAssetSyncById_FlushIO, AzFramework::AssetSystem::AssetStatus (const AZ::Data::AssetId&));
        MOCK_METHOD4(ConfigureSocketConnection, bool (const AZStd::string&, const AZStd::string&, const AZStd::string&, const AZStd::string&));
        MOCK_METHOD1(Connect, bool (const char*));
        MOCK_METHOD2(ConnectWithTimeout, bool (const char*, AZStd::chrono::duration<float>));
        MOCK_METHOD0(Disconnect, bool ());
        MOCK_METHOD1(EscalateAssetBySearchTerm, bool (AZStd::string_view));
        MOCK_METHOD1(EscalateAssetByUuid, bool (const AZ::Uuid&));
        MOCK_METHOD0(GetAssetProcessorPingTimeMilliseconds, float ());
        MOCK_METHOD1(GetAssetStatus, AzFramework::AssetSystem::AssetStatus (const AZStd::string&));
        MOCK_METHOD1(GetAssetStatus_FlushIO, AzFramework::AssetSystem::AssetStatus (const AZStd::string&));
        MOCK_METHOD2(GetAssetStatusSearchType, AzFramework::AssetSystem::AssetStatus(const AZStd::string&, int));
        MOCK_METHOD2(GetAssetStatusSearchType_FlushIO, AzFramework::AssetSystem::AssetStatus(const AZStd::string&, int));
        MOCK_METHOD1(GetAssetStatusById_FlushIO, AzFramework::AssetSystem::AssetStatus (const AZ::Data::AssetId&));
        MOCK_METHOD3(GetUnresolvedProductReferences, void (AZ::Data::AssetId, AZ::u32&, AZ::u32&));
        MOCK_METHOD0(SaveCatalog, bool ());
        MOCK_METHOD1(SetAssetProcessorIP, void (const AZStd::string&));
        MOCK_METHOD1(SetAssetProcessorPort, void (AZ::u16));
        MOCK_METHOD1(SetBranchToken, void (const AZStd::string&));
        MOCK_METHOD1(SetProjectName, void (const AZStd::string&));
        MOCK_METHOD0(ShowAssetProcessor, void ());
        MOCK_METHOD1(ShowInAssetProcessor, void (const AZStd::string&));
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
        bool m_statusRequest = false;
    };

    struct MockCatalog
        : AZ::Data::AssetCatalogRequestBus::Handler
    {
        MockCatalog()
        {
            BusConnect();
        }

        ~MockCatalog()
        {
            BusDisconnect();
        }

        AZ::Data::AssetId GetAssetIdByPath(const char*, const AZ::Data::AssetType&, bool) override
        {
            m_generatedId = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 1234);

            return m_generatedId;
        }

        MOCK_METHOD1(GetAssetInfoById, AZ::Data::AssetInfo (const AZ::Data::AssetId&));
        MOCK_METHOD1(AddAssetType, void (const AZ::Data::AssetType&));
        MOCK_METHOD1(AddDeltaCatalog, bool (AZStd::shared_ptr<AzFramework::AssetRegistry>));
        MOCK_METHOD1(AddExtension, void (const char*));
        MOCK_METHOD0(ClearCatalog, void ());
        MOCK_METHOD5(CreateBundleManifest, bool (const AZStd::string&, const AZStd::vector<AZStd::string>&, const AZStd::string&, int, const AZStd::vector<AZStd::string>&));
        MOCK_METHOD2(CreateDeltaCatalog, bool (const AZStd::vector<AZStd::string>&, const AZStd::string&));
        MOCK_METHOD0(DisableCatalog, void ());
        MOCK_METHOD1(EnableCatalogForAsset, void (const AZ::Data::AssetType&));
        MOCK_METHOD3(EnumerateAssets, void (BeginAssetEnumerationCB, AssetEnumerationCB, EndAssetEnumerationCB));
        MOCK_METHOD1(GenerateAssetIdTEMP, AZ::Data::AssetId (const char*));
        MOCK_METHOD1(GetAllProductDependencies, AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> (const AZ::Data::AssetId&));
        MOCK_METHOD3(GetAllProductDependenciesFilter, AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> (const AZ::Data::AssetId&, const AZStd::unordered_set<AZ::Data::AssetId>&, const AZStd::vector<AZStd::string>&));
        MOCK_METHOD1(GetAssetPathById, AZStd::string (const AZ::Data::AssetId&));
        MOCK_METHOD1(GetDirectProductDependencies, AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> (const AZ::Data::AssetId&));
        MOCK_METHOD1(GetHandledAssetTypes, void (AZStd::vector<AZ::Data::AssetType>&));
        MOCK_METHOD0(GetRegisteredAssetPaths, AZStd::vector<AZStd::string> ());
        MOCK_METHOD2(InsertDeltaCatalog, bool (AZStd::shared_ptr<AzFramework::AssetRegistry>, size_t));
        MOCK_METHOD2(InsertDeltaCatalogBefore, bool (AZStd::shared_ptr<AzFramework::AssetRegistry>, AZStd::shared_ptr<AzFramework::AssetRegistry>));
        MOCK_METHOD1(LoadCatalog, bool (const char*));
        MOCK_METHOD2(RegisterAsset, void (const AZ::Data::AssetId&, AZ::Data::AssetInfo&));
        MOCK_METHOD1(RemoveDeltaCatalog, bool (AZStd::shared_ptr<AzFramework::AssetRegistry>));
        MOCK_METHOD1(SaveCatalog, bool (const char*));
        MOCK_METHOD0(StartMonitoringAssets, void ());
        MOCK_METHOD0(StopMonitoringAssets, void ());
        MOCK_METHOD1(UnregisterAsset, void (const AZ::Data::AssetId&));

        AZ::Data::AssetId m_generatedId{};
    };

    struct MockAssetData
        : LmbrCentral::MeshAsset
    {
        MockAssetData(AZ::Data::AssetId assetId)
        {
            m_assetId = assetId;
        }
    };

    TEST_F(MeshAssetHandlerFixture, LoadAsset_StillInQueue_LoadsSubstituteAsset)
    {
        MockAssetSystemRequestHandler assetSystem;
        MockCatalog catalog;
        AZ::Data::AssetId assetId(AZ::Uuid::CreateRandom(), 0);

        AZ::Data::Asset<AZ::Data::AssetData> asset(aznew MockAssetData(assetId), AZ::Data::AssetLoadBehavior::Default);
        auto substituteAssetId = m_handler.AssetMissingInCatalog(asset);

        ASSERT_TRUE(assetSystem.m_statusRequest);
        ASSERT_TRUE(catalog.m_generatedId.IsValid());
        ASSERT_EQ(substituteAssetId, catalog.m_generatedId);
    }



} // namespace UnitTest
