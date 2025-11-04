/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/UnitTest/MockComponentApplication.h>
#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>

#include <AzFramework/API/ApplicationAPI.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/RootAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>
#include <AzToolsFramework/AssetBrowser/Favorites/AssetBrowserFavoritesManager.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <Editor/AzAssetBrowser/AzAssetBrowserRequestHandler.h>
#include <AzQtComponents/DragAndDrop/ViewportDragAndDrop.h>

#include <gmock/gmock.h>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>

namespace UnitTest
{
    // --------------------- MOCKS ----- The tests are at the bottom of this file -------------------

    // a mock component that has the bare minimum interface of an editor component.
    class MockEditorComponent : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        static inline constexpr AZ::TypeId s_MockEditorComponentTypeId{ "{1D01FB53-1453-4250-A561-B2A657816B03}" };
        AZ_COMPONENT(MockEditorComponent, s_MockEditorComponentTypeId, AzToolsFramework::Components::EditorComponentBase);

        void SetPrimaryAsset(const AZ::Data::AssetId& assetId) override
        {
            m_primaryAssetSet = assetId;
        }
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType&) {}
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType&){}
        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(context);

            if (serializeContext)
            {
                serializeContext->Class<MockEditorComponent, AzToolsFramework::Components::EditorComponentBase>();
            }
        }

        AZ::Data::AssetId m_primaryAssetSet;
    };

    // a mock Asset Type Info provider that always provides the above component type
    // in response to a query about what kind of a component should be spawned
    // for a given asset type.
    class AssetTypeInfoBusHandlerMock : public AZ::AssetTypeInfoBus::Handler
    {
    public:
        MOCK_CONST_METHOD0(GetComponentTypeId, AZ::Uuid());
        MOCK_CONST_METHOD0(GetAssetType, AZ::Data::AssetType());
        MOCK_CONST_METHOD0(GetAssetTypeDragAndDropCreationPriority, int());

        void InstallHandlersFor(AZ::Uuid componentType, AZ::Data::AssetType assetType, int priority = 0)
        {
            ON_CALL(*this, GetComponentTypeId)
                .WillByDefault(
                    [componentType]()
                    {
                        return componentType;
                    });

            ON_CALL(*this, GetAssetType)
                .WillByDefault(
                    [assetType]()
                    {
                        return assetType;
                    });

            ON_CALL(*this, GetAssetTypeDragAndDropCreationPriority)
                .WillByDefault(
                    [priority]()
                    {
                        return priority;
                    });

            BusConnect(assetType);
        }
    };
   
    class MockAzFrameworkApplicationRequestBusHandler : public AzFramework::ApplicationRequests::Bus::Handler
    {
    public:
        // AzFramework Application API mock:
        MOCK_METHOD1(NormalizePath, void(AZStd::string& /*path*/));
        MOCK_METHOD1(NormalizePathKeepCase, void(AZStd::string& /*path*/));
        MOCK_CONST_METHOD1(CalculateBranchTokenForEngineRoot, void(AZStd::string& /*token*/));
    };

    class MockEditorRequestBusHandler : public AzToolsFramework::EditorRequestBus::Handler
    {
    public:
        MOCK_METHOD1(BrowseForAssets, void(AzToolsFramework::AssetBrowser::AssetSelectionModel& /*selection*/));
        MOCK_METHOD2(CreateNewEntityAtPosition, AZ::EntityId (const AZ::Vector3&, AZ::EntityId));
    };

    class MockEntityCompositionRequestBus
        : public AzToolsFramework::EntityCompositionRequestBus::Handler
    {
    public:
        MOCK_METHOD2(AddComponentsToEntities, AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome(const AzToolsFramework::EntityIdList&, const AZ::ComponentTypeList&));
        MOCK_METHOD1(CutComponents, void(AZStd::span<AZ::Component* const>));
        MOCK_METHOD1(CopyComponents, void(AZStd::span<AZ::Component* const>));
        MOCK_METHOD1(PasteComponentsToEntity, void (AZ::EntityId ));
        MOCK_METHOD0(HasComponentsToPaste, bool ());
        MOCK_METHOD1(EnableComponents,void (AZStd::span<AZ::Component* const>));
        MOCK_METHOD1(DisableComponents, void (AZStd::span<AZ::Component* const>));
        MOCK_METHOD2(AddExistingComponentsToEntityById,  AzToolsFramework::EntityCompositionRequests::AddExistingComponentsOutcome(const AZ::EntityId&, AZStd::span<AZ::Component* const>));
        MOCK_METHOD1(RemoveComponents,AzToolsFramework::EntityCompositionRequests::RemoveComponentsOutcome(AZStd::span<AZ::Component* const>));
        MOCK_METHOD1(ScrubEntities, AzToolsFramework::EntityCompositionRequests::ScrubEntitiesOutcome(const AzToolsFramework::EntityList&));
        MOCK_METHOD1(GetPendingComponentInfo, AzToolsFramework::EntityCompositionRequests::PendingComponentInfo(const AZ::Component*));
        MOCK_METHOD1(GetComponentName, AZStd::string(const AZ::Component*));
    };

    class AzAssetBrowserRequestHandlerFixture : public LeakDetectionFixture
    {
    public:
        using MockComponentApplicationBusHandler = UnitTest::MockComponentApplication;
        void SetUp() override
        {
            using namespace AzToolsFramework;
            using namespace AzToolsFramework::AssetBrowser;
            using namespace AzToolsFramework::AssetDatabase;

            LeakDetectionFixture::SetUp();

            m_editorComponentDescriptor = MockEditorComponent::CreateDescriptor();
            m_assetTypeOfModel = AZ::Data::AssetType("{8ABC6797-2DB6-4AC1-975B-5B344ABD9105}");
            m_assetTypeOfActor = AZ::Data::AssetType("{2C9B7713-8C78-43AA-ABC9-B1FEC964ECFC}");

            m_fileIOMock = AZStd::make_unique<testing::NiceMock<AZ::IO::MockFileIOBase>>();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_componentApplicationMock = AZStd::make_unique<testing::NiceMock<MockComponentApplicationBusHandler>>();
            m_frameworkApplicationMock = AZStd::make_unique<testing::NiceMock<MockAzFrameworkApplicationRequestBusHandler>>();
            m_editorRequestHandlerMock = AZStd::make_unique<testing::NiceMock<MockEditorRequestBusHandler>>();
            m_entityCompositionRequestBusMock = AZStd::make_unique<testing::NiceMock<MockEntityCompositionRequestBus>>();

            m_frameworkApplicationMock.get()->BusConnect();
            m_editorRequestHandlerMock.get()->BusConnect();
            m_entityCompositionRequestBusMock.get()->BusConnect();

            // Swap out current file io instance for our mock
            m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_fileIOMock.get());

            // Setup the default returns for our mock file io calls
            AZ::IO::MockFileIOBase::InstallDefaultReturns(*m_fileIOMock.get());

            MockEditorComponent::Reflect(m_serializeContext.get());

            // override the file IO mocks's ISDirectory function to return true
            // only if the folder name is "C:/whatever"
            ON_CALL(*m_fileIOMock.get(), IsDirectory(::testing::_))
                .WillByDefault(
                    [](const char* pathName) -> bool
                    {
                        return azstricmp(pathName, "C:/whatever") == 0;
                    });

            // override the editor request handler's CreateNewEntityAtPosition mock
            // to create a new entity and add it to this class's m_createdEntities list.
            ON_CALL(*m_editorRequestHandlerMock.get(), CreateNewEntityAtPosition(::testing::_, ::testing::_))
                .WillByDefault(
                    [&](const AZ::Vector3&, AZ::EntityId)
                    {
                        AZ::Entity* newEntity = new AZ::Entity(AZ::EntityId(static_cast<AZ::u64>(m_createdEntities.size()) + 1));
                        m_createdEntities.push_back(newEntity);
                        newEntity->Init();
                        newEntity->Activate();
                        return newEntity->GetId();
                    });

            ON_CALL(*m_componentApplicationMock.get(), GetSerializeContext())
                .WillByDefault(::testing::Return(m_serializeContext.get()));

            // AddEntity should just return true - to avoid asserts, etc.
            ON_CALL(*m_componentApplicationMock.get(), AddEntity(::testing::_))
                .WillByDefault(::testing::Return(true));

            // override the component application's mock to respond to FindEntity
            // by searching this class's m_entities list.
            ON_CALL(*m_componentApplicationMock.get(), FindEntity(::testing::_))
                .WillByDefault(
                    [&](const AZ::EntityId& entity) -> AZ::Entity*
                    {
                        auto found = AZStd::find_if(
                            m_createdEntities.begin(),
                            m_createdEntities.end(),
                            [&](const AZ::Entity* check)
                            {
                                return ((check) && (check->GetId() == entity));
                            });
                        if (found != m_createdEntities.end())
                        {
                            return *found;
                        }
                        return nullptr;
                    });

            // respond to requests to create components on entities by
            // checking that its always the editor component, and that its always
            // the correct entity.
            // store the components created on m_componentsAddedToEntites
            ON_CALL(*m_entityCompositionRequestBusMock.get(), AddComponentsToEntities(::testing::_, ::testing::_))
                .WillByDefault(
                    [&](const AzToolsFramework::EntityIdList& entities,
                        const AZ::ComponentTypeList& componentsToAdd)
                        -> EntityCompositionRequests::AddComponentsOutcome
                    {
                        using namespace AzToolsFramework;
                        using ThisOutcome = EntityCompositionRequests::AddComponentsOutcome;
                        for (auto comptype : componentsToAdd)
                        {
                            if (comptype != AZ::Uuid(MockEditorComponent::s_MockEditorComponentTypeId))
                            {
                                return ThisOutcome(AZStd::unexpect, AZStd::string("Failed - wrong component"));
                            }
                        }
                        if (componentsToAdd.size() != 1)
                        {
                            return ThisOutcome(AZStd::unexpect, AZStd::string("Failed - too many components"));
                        }
                        // to 1 entity
                        if (entities.size() != 1)
                        {
                            return ThisOutcome(AZStd::unexpect, AZStd::string("Failed - requires exactly 1 entity"));
                        }
                        // record what components were attempted to be added:
                        m_componentsAddedToEntites.assign(componentsToAdd.begin(), componentsToAdd.end());
                        if (AZ::Entity* ent = m_componentApplicationMock.get()->FindEntity(entities[0]); ent)
                        {
                            ent->AddComponent(new MockEditorComponent());
                        }

                        return AZ::Success(ThisOutcome::ValueType());
                    });

            m_rootAssetBrowserEntry = AZStd::make_unique<RootAssetBrowserEntry>();

            // add some asset browser entries to work with in tests.
            // (root)
            //    "c:/whatever" (scan folder)
            //          "asset1.fbx" (source)
            //             "asset_zzzz.model" (model product)
            //             "asset_aaaa.model" (modelproduct)
            //          "asset2.fbx" (source)
            //             "testmodel_aaaa.model" (model product)
            //             "testmodel_zzzz.actor" (actor product)

            ScanFolderDatabaseEntry scanFolder = AssetDatabase::ScanFolderDatabaseEntry();
            scanFolder.m_scanFolderID = 0;
            scanFolder.m_scanFolder = "C:/whatever";
            scanFolder.m_displayName = "ScanFolder1";
            scanFolder.m_isRoot = true;
            m_rootAssetBrowserEntry->AddScanFolder(scanFolder);

            // 2 files - one for each source file.
            AssetDatabase::FileDatabaseEntry entry = AssetDatabase::FileDatabaseEntry();
            entry.m_scanFolderPK = 0;
            entry.m_fileID = 1;
            entry.m_fileName = "asset1.fbx";
            entry.m_isFolder = false;
            m_rootAssetBrowserEntry->AddFile(entry);

            AssetDatabase::FileDatabaseEntry entry2 = AssetDatabase::FileDatabaseEntry();
            entry2.m_scanFolderPK = 0;
            entry2.m_fileID = 2;
            entry2.m_fileName = "asset2.fbx";
            entry2.m_isFolder = false;
            m_rootAssetBrowserEntry->AddFile(entry2);

            // 2 source files - one for each above file.
            AssetBrowser::SourceWithFileID entrySource = AssetBrowser::SourceWithFileID();
            entrySource.first = entry.m_fileID;
            entrySource.second = AzToolsFramework::AssetDatabase::SourceDatabaseEntry();
            entrySource.second.m_scanFolderPK = 0;
            entrySource.second.m_sourceName = "asset1.fbx";
            entrySource.second.m_sourceID = 1;
            entrySource.second.m_sourceGuid = AZ::Uuid::CreateRandom();
            m_rootAssetBrowserEntry->AddSource(entrySource);
            m_uuidOfSource1 = entrySource.second.m_sourceGuid;

            AssetBrowser::SourceWithFileID entrySource2 = AssetBrowser::SourceWithFileID();
            entrySource2.first = entry2.m_fileID;
            entrySource2.second = AzToolsFramework::AssetDatabase::SourceDatabaseEntry();
            entrySource2.second.m_scanFolderPK = 0;
            entrySource2.second.m_sourceName = "asset2.fbx";
            entrySource2.second.m_sourceID = 2; // database PK, must be unique across all sources.
            entrySource2.second.m_sourceGuid = AZ::Uuid::CreateRandom();
            m_rootAssetBrowserEntry->AddSource(entrySource2);
            m_uuidOfSource2 = entrySource2.second.m_sourceGuid;

            // 2 products for the first source
            AssetBrowser::ProductWithUuid product = AssetBrowser::ProductWithUuid();
            product.first = entrySource.second.m_sourceGuid;
            product.second = AzToolsFramework::AssetDatabase::ProductDatabaseEntry();
            product.second.m_productID = 1; // database PK, must be unique across all products.
            product.second.m_subID = 1;
            product.second.m_assetType = m_assetTypeOfModel;
            product.second.m_productName = "asset_zzzz.model";
            m_rootAssetBrowserEntry->AddProduct(product);

            AssetBrowser::ProductWithUuid product2 = AssetBrowser::ProductWithUuid();
            product2.first = entrySource.second.m_sourceGuid;
            product2.second = AzToolsFramework::AssetDatabase::ProductDatabaseEntry();
            product2.second.m_productID = 2;
            product2.second.m_subID = 2;
            product2.second.m_assetType = m_assetTypeOfModel;
            product2.second.m_productName = "asset_aaaa.model";
            // note, the second one in the list is alphabetically first.
            m_rootAssetBrowserEntry->AddProduct(product2);

            // 2 products for the second source
            // the 2nd one is alphabetically after the first one and is of type actor
            AssetBrowser::ProductWithUuid product3 = AssetBrowser::ProductWithUuid();
            product3.first = entrySource2.second.m_sourceGuid;
            product3.second = AzToolsFramework::AssetDatabase::ProductDatabaseEntry();
            product3.second.m_productID = 3;
            product3.second.m_subID = 1;
            product3.second.m_assetType = m_assetTypeOfModel;

            product3.second.m_productName = "testmodel_aaaa.model";
            m_rootAssetBrowserEntry->AddProduct(product3);

            AssetBrowser::ProductWithUuid product4 = AssetBrowser::ProductWithUuid();
            product4.first = entrySource2.second.m_sourceGuid;
            product4.second = AzToolsFramework::AssetDatabase::ProductDatabaseEntry();
            product4.second.m_productID = 4;
            product4.second.m_subID = 123;
            product4.second.m_assetType = m_assetTypeOfActor;
            product4.second.m_productName = "testmodel_zzzz.actor";
            // note, the second one in this source is alphabetically last.
            m_rootAssetBrowserEntry->AddProduct(product4);
        }

        void TearDown() override
        {
            m_frameworkApplicationMock.get()->BusDisconnect();
            m_editorRequestHandlerMock.get()->BusDisconnect();
            m_entityCompositionRequestBusMock.get()->BusDisconnect();

            for (AZ::Entity* targetEntity : m_createdEntities)
            {
                targetEntity->Deactivate();
                delete targetEntity;
            }
            m_createdEntities.clear();

            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
            m_rootAssetBrowserEntry.reset();
            AzToolsFramework::AssetBrowser::EntryCache::DestroyInstance();
            AzToolsFramework::AssetBrowser::AssetBrowserFavoritesManager::DestroyInstance();

            m_editorComponentDescriptor->ReleaseDescriptor();

            m_serializeContext.reset();
            m_componentApplicationMock.reset();
            m_frameworkApplicationMock.reset();
            m_editorRequestHandlerMock.reset();
            m_entityCompositionRequestBusMock.reset();

            LeakDetectionFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<testing::NiceMock<AZ::IO::MockFileIOBase>> m_fileIOMock;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<testing::NiceMock<MockComponentApplicationBusHandler>> m_componentApplicationMock;
        AZStd::unique_ptr<testing::NiceMock<MockAzFrameworkApplicationRequestBusHandler>> m_frameworkApplicationMock;
        AZStd::unique_ptr<testing::NiceMock<MockEditorRequestBusHandler>> m_editorRequestHandlerMock;
        AZStd::unique_ptr<testing::NiceMock<MockEntityCompositionRequestBus>> m_entityCompositionRequestBusMock;
        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZStd::unique_ptr<AzToolsFramework::AssetBrowser::RootAssetBrowserEntry> m_rootAssetBrowserEntry;
        AZ::Uuid m_uuidOfSource1;
        AZ::Uuid m_uuidOfSource2;
        AZ::Data::AssetType m_assetTypeOfModel;
        AZ::Data::AssetType m_assetTypeOfActor;
        AZStd::vector<AZ::Entity*> m_createdEntities;
        AZ::ComponentTypeList m_componentsAddedToEntites;
    private:
        AZ::ComponentDescriptor* m_editorComponentDescriptor = nullptr;
    };

    TEST_F(AzAssetBrowserRequestHandlerFixture, DragEnterTest)
    {
        using namespace AzQtComponents;
        using namespace AzQtComponents::DragAndDropContexts;
        AzAssetBrowserRequestHandler browser;
        AzQtComponents::ViewportDragContext ctx;
        ctx.m_hitLocation = AZ::Vector3::CreateZero();
        QMimeData mimeData;
        QDragEnterEvent enter(
            QPoint(0, 0), Qt::DropAction::CopyAction, &mimeData, Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier);

        // empty mime data:  No response.
        DragAndDropEventsBus::Event(EditorViewport, &DragAndDropEvents::DragEnter, &enter, ctx);

        EXPECT_EQ(false, enter.isAccepted());
    }

    // in this test, we give it valid drag and drop assets, but they don't have any default behavior
    // and they don't have any association between components and asset types to spawn.
    // This should result in the drag and drop not being accepted and no entities being spawned.
    TEST_F(AzAssetBrowserRequestHandlerFixture, DragEnterTest_DragSource_NoDefaultBehavior_DoesNotSpawnAnything)
    {
        using namespace AzToolsFramework;
        using namespace AzToolsFramework::AssetBrowser;
        using namespace AzQtComponents;
        using namespace AzQtComponents::DragAndDropContexts;

        AzAssetBrowserRequestHandler browser;
        AzQtComponents::ViewportDragContext ctx;
        ctx.m_hitLocation = AZ::Vector3::CreateZero();

        // simulate dragging and dropping the source file:
        QMimeData mimeData;
        AZStd::vector<const AssetBrowserEntry*> entries;
        entries.push_back(SourceAssetBrowserEntry::GetSourceByUuid(m_uuidOfSource1));
        Utils::ToMimeData(&mimeData, entries);

        QDropEvent dropEvent(
            QPoint(0, 0), Qt::DropAction::CopyAction, &mimeData, Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier);

        DragAndDropEventsBus::Event(EditorViewport, &DragAndDropEvents::Drop, &dropEvent, ctx);

        // Because nobody opted in to the Asset Type Info bus to say what components
        // should spawn when certain asset types are dropped, we shouldn't see any entities spawned at all.
        EXPECT_EQ(false, dropEvent.isAccepted());
        EXPECT_TRUE(m_createdEntities.empty());
    }

    // In this case, we set the default component for the 'model' asset to be our mock component
    // this should cause it to attempt to spawn 1 entity, with 1 'mock' component on it.
    // note that the above mocks create an asset browser with the following structure
    // (root)
    //    asset1.fbx
    //       (product) asset_zzzz.model
    //       (product) asset_aaaa.model
    // intentionally, the zzzz asset is in the model before the aaaa one, but, due to the
    // heuristic, we expect to still spawn the aaaa one.  
    TEST_F(AzAssetBrowserRequestHandlerFixture, DragEnterTest_DragSource_WithDefaultBehavior_SpawnsOneEntity)
    {
        using namespace AzToolsFramework;
        using namespace AzToolsFramework::AssetBrowser;
        using namespace AzQtComponents;
        using namespace AzQtComponents::DragAndDropContexts;

        AzAssetBrowserRequestHandler browser;
        AzQtComponents::ViewportDragContext ctx;
        ctx.m_hitLocation = AZ::Vector3::CreateZero();

        // simulate dragging and dropping the source file:
        QMimeData mimeData;
        AZStd::vector<const AssetBrowserEntry*> entries;
        entries.push_back(SourceAssetBrowserEntry::GetSourceByUuid(m_uuidOfSource1));
        Utils::ToMimeData(&mimeData, entries);

        QDropEvent dropEvent(
            QPoint(0, 0),
            Qt::DropAction::CopyAction,
            &mimeData,
            Qt::MouseButton::LeftButton,
            Qt::KeyboardModifier::NoModifier);

        // in this case, we actually register the asset type of the product
        // this creates an AssetTypeInfo handler which says
        // when asset of type m_assetTypeOfmodel is dragged, spawn a s_MockEditorComponentTypeId.
        testing::NiceMock<AssetTypeInfoBusHandlerMock> mockHandler;
        mockHandler.InstallHandlersFor(AZ::Uuid(MockEditorComponent::s_MockEditorComponentTypeId), m_assetTypeOfModel);

        DragAndDropEventsBus::Event(EditorViewport, &DragAndDropEvents::Drop, &dropEvent, ctx);

        mockHandler.BusDisconnect();

        EXPECT_TRUE(dropEvent.isAccepted());
        ASSERT_EQ(1, m_createdEntities.size());

        for (auto entity : m_createdEntities)
        {
            MockEditorComponent* mockComponent = entity->FindComponent<MockEditorComponent>();
            ASSERT_NE(nullptr, mockComponent);

            // we expect the 'aaaa' asset, not the zzzz one.
            EXPECT_EQ(AZ::Data::AssetId(m_uuidOfSource1, 2), mockComponent->m_primaryAssetSet)
                << "Invalid component spawned.  Should have spawned the aaa one alphabetically.";
        }
    }

    // this is the same test as above, but this time, picks the second source file
    // the one with 2 different types of assets.
    // it should pick the second asset, despite it being later alphabetically, due to the priority override bus.
    TEST_F(AzAssetBrowserRequestHandlerFixture, DragEnterTest_DragSource_WithDefaultBehavior_PrioritySystemWorks)
    {
        using namespace AzToolsFramework;
        using namespace AzToolsFramework::AssetBrowser;
        using namespace AzQtComponents;
        using namespace AzQtComponents::DragAndDropContexts;

        AzAssetBrowserRequestHandler browser;
        AzQtComponents::ViewportDragContext ctx;
        ctx.m_hitLocation = AZ::Vector3::CreateZero();

        // simulate dragging and dropping the source file:
        QMimeData mimeData;
        AZStd::vector<const AssetBrowserEntry*> entries;
        // use the 2nd source file - the one that has an actor and a model as its children
        entries.push_back(SourceAssetBrowserEntry::GetSourceByUuid(m_uuidOfSource2));
        Utils::ToMimeData(&mimeData, entries);

        QDropEvent dropEvent(
            QPoint(0, 0), Qt::DropAction::CopyAction, &mimeData, Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier);

        // when asset of type m_assetTypeOfModel is dragged, spawn a s_MockEditorComponentTypeId.
        // when asset of type m_assetTypeOfActor is dragged, spawn a s_MockEditorComponentTypeId also - but high priority
        testing::NiceMock<AssetTypeInfoBusHandlerMock> mockHandler1;
        testing::NiceMock<AssetTypeInfoBusHandlerMock> mockHandler2;

        // the actor handler should have a higher priority, so it should always 'win'
        mockHandler1.InstallHandlersFor(AZ::Uuid(MockEditorComponent::s_MockEditorComponentTypeId), m_assetTypeOfModel);
        mockHandler2.InstallHandlersFor(AZ::Uuid(MockEditorComponent::s_MockEditorComponentTypeId), m_assetTypeOfActor, 10);

        DragAndDropEventsBus::Event(EditorViewport, &DragAndDropEvents::Drop, &dropEvent, ctx);

        mockHandler1.BusDisconnect();
        mockHandler2.BusDisconnect();

        EXPECT_TRUE(dropEvent.isAccepted());
        ASSERT_EQ(1, m_createdEntities.size());

        for (auto entity : m_createdEntities)
        {
            MockEditorComponent* mockComponent = entity->FindComponent<MockEditorComponent>();
            ASSERT_NE(nullptr, mockComponent);

            // we expect the 'actor' asset, not the zzzz one.
            EXPECT_EQ(AZ::Data::AssetId(m_uuidOfSource2, 123), mockComponent->m_primaryAssetSet)
                << "Invalid component spawned.  Should have spawned the actor one due to higher priority.";
        }
    }

    // this test checks to make sure multi-select works (even though the current asset browser does not support it
    // the API should).   It selects BOTH sources, and expects 2 different entities to be created.
    TEST_F(AzAssetBrowserRequestHandlerFixture, DragEnterTest_DragSource_WithDefaultBehavior_MultiSelect)
    {
        using namespace AzToolsFramework;
        using namespace AzToolsFramework::AssetBrowser;
        using namespace AzQtComponents;
        using namespace AzQtComponents::DragAndDropContexts;

        AzAssetBrowserRequestHandler browser;
        AzQtComponents::ViewportDragContext ctx;
        ctx.m_hitLocation = AZ::Vector3::CreateZero();

        // simulate dragging and dropping the source file:
        QMimeData mimeData;
        AZStd::vector<const AssetBrowserEntry*> entries;
        // use both sources:
        entries.push_back(SourceAssetBrowserEntry::GetSourceByUuid(m_uuidOfSource1));
        entries.push_back(SourceAssetBrowserEntry::GetSourceByUuid(m_uuidOfSource2));
        Utils::ToMimeData(&mimeData, entries);

        QDropEvent dropEvent(
            QPoint(0, 0), Qt::DropAction::CopyAction, &mimeData, Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier);

        // when asset of type m_assetTypeOfModel is dragged, spawn a s_MockEditorComponentTypeId.
        // when asset of type m_assetTypeOfActor is dragged, spawn a s_MockEditorComponentTypeId also - but high priority
        testing::NiceMock<AssetTypeInfoBusHandlerMock> mockHandler1;
        testing::NiceMock<AssetTypeInfoBusHandlerMock> mockHandler2;

        // the actor handler should have a higher priority, so it should always 'win'
        mockHandler1.InstallHandlersFor(AZ::Uuid(MockEditorComponent::s_MockEditorComponentTypeId), m_assetTypeOfModel);
        mockHandler2.InstallHandlersFor(AZ::Uuid(MockEditorComponent::s_MockEditorComponentTypeId), m_assetTypeOfActor, 10);

        DragAndDropEventsBus::Event(EditorViewport, &DragAndDropEvents::Drop, &dropEvent, ctx);

        mockHandler1.BusDisconnect();
        mockHandler2.BusDisconnect();

        EXPECT_TRUE(dropEvent.isAccepted());
        EXPECT_FALSE(m_createdEntities.empty());

        // inspect the created entities:
        ASSERT_EQ(2, m_createdEntities.size());

        // One entity should have the first source, subid 2
        // The other entity should have the second source, subid 123
        // The order of entities is not defined, so we can't make assumptions.
        // So put the expected values in an unordered_set, and pluck them out as they are found.
        // it will be an error if we find one not in the set, or if there are still remainders after
        // we've checked all entities.
        AZStd::unordered_set<AZ::Data::AssetId> expectedAssetIds;
        expectedAssetIds.insert(AZ::Data::AssetId(m_uuidOfSource1, 2));
        expectedAssetIds.insert(AZ::Data::AssetId(m_uuidOfSource2, 123));
        
        for (auto entity : m_createdEntities)
        {
            auto allComponents = entity->FindComponents<MockEditorComponent>();
            ASSERT_EQ(1, allComponents.size()); // exactly 1 component per entty
            MockEditorComponent* mockComponent = allComponents[0];
            ASSERT_NE(nullptr, mockComponent);

            AZ::Data::AssetId currentAsset = mockComponent->m_primaryAssetSet;

            ASSERT_TRUE(expectedAssetIds.find(currentAsset) != expectedAssetIds.end());
            expectedAssetIds.erase(currentAsset);
        }
        EXPECT_TRUE(expectedAssetIds.empty()) << "Did not find all expected assets!";
    }

    // this test checks to make sure that when custom handlers are installed, and eat the event, the default
    // processing does not happen:
    TEST_F(AzAssetBrowserRequestHandlerFixture, DragEnterTest_DragSource_WithDefaultBehavior_OverridableByHandlers)
    {
        using namespace AzToolsFramework;
        using namespace AzToolsFramework::AssetBrowser;
        using namespace AzQtComponents;
        using namespace AzQtComponents::DragAndDropContexts;

        AzAssetBrowserRequestHandler browser;
        AzQtComponents::ViewportDragContext ctx;
        ctx.m_hitLocation = AZ::Vector3::CreateZero();

        // simulate dragging and dropping the source file:
        QMimeData mimeData;
        AZStd::vector<const AssetBrowserEntry*> entries;
        entries.push_back(SourceAssetBrowserEntry::GetSourceByUuid(m_uuidOfSource1));
        Utils::ToMimeData(&mimeData, entries);

        QDropEvent dropEvent(
            QPoint(0, 0), Qt::DropAction::CopyAction, &mimeData, Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier);

        testing::NiceMock<AssetTypeInfoBusHandlerMock> mockHandler1;
        mockHandler1.InstallHandlersFor(AZ::Uuid(MockEditorComponent::s_MockEditorComponentTypeId), m_assetTypeOfModel);

        // pre-accept the event.  This should cause the azAssetBrowserRequestHandler to ignore it.
        dropEvent.accept();

        DragAndDropEventsBus::Event(EditorViewport, &DragAndDropEvents::Drop, &dropEvent, ctx);

        mockHandler1.BusDisconnect();

        // because we intercept the event at a pretty high level,
        // no entities should be spawned.
        EXPECT_TRUE(dropEvent.isAccepted());
        EXPECT_TRUE(m_createdEntities.empty());
    }
} // namespace UnitTest
