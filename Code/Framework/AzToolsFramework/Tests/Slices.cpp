/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/base.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/Script/ScriptSystemComponent.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>
#include <AzToolsFramework/Slice/SliceCompilation.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>

#include "EntityTestbed.h"

#include <AzCore/Serialization/Utils.h>
#include <AzCore/IO/FileIO.h>

namespace UnitTest
{
    using namespace AZ;

    class SliceInteractiveWorkflowTest
        : public EntityTestbed
        , AZ::Data::AssetBus::MultiHandler
    {
    public:

        class TestComponent1
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(TestComponent1, "{54BA51C3-41BD-4BB6-B1ED-7F6CEFAC2F9F}");

            void Init() override
            {
            }
            void Activate() override {}
            void Deactivate() override {}

            static void Reflect(ReflectContext* context)
            {
                auto* serialize = azrtti_cast<AZ::SerializeContext*>(context);
                if (serialize)
                {
                    serialize->Class<TestComponent1, AZ::Component>()
                        ->Version(1)
                        ->Field("SomeFlag", &TestComponent1::m_someFlag)
                    ;

                    AZ::EditContext* ec = serialize->GetEditContext();
                    if (ec)
                    {
                        ec->Class<TestComponent1>("Another component", "A component.")
                            ->DataElement("CheckBox", &TestComponent1::m_someFlag, "SomeFlag", "");
                    }
                }
            }

            bool m_someFlag = false;
        };

        class TestComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(TestComponent, "{F146074C-152E-483C-AD33-6D1945B4261A}");

            void Init() override
            {
                m_rootElement = aznew Entity("Blah");
                m_rootElement->CreateComponent<TestComponent1>();
            }
            void Activate() override {}
            void Deactivate() override {}

            static void Reflect(ReflectContext* context)
            {
                auto* serialize = azrtti_cast<AZ::SerializeContext*>(context);
                if (serialize)
                {
                    serialize->Class<TestComponent, AZ::Component>()
                        ->Version(1)
                        ->Field("RootElement", &TestComponent::m_rootElement)
                        ->Field("LastElement", &TestComponent::m_lastElementId)
                        ->Field("DrawOrder", &TestComponent::m_drawOrder)
                        ->Field("IsPixelAligned", &TestComponent::m_isPixelAligned)
                    ;

                    AZ::EditContext* ec = serialize->GetEditContext();
                    if (ec)
                    {
                        ec->Class<TestComponent>("Ui Canvas", "A component.")
                            ->DataElement("CheckBox", &TestComponent::m_isPixelAligned, "IsPixelAligned", "Is pixel aligned.");
                    }
                }
            }

            AZ::Entity* m_rootElement = nullptr;
            unsigned int m_lastElementId = 0;
            int m_drawOrder = 0;
            bool m_isPixelAligned = false;
        };

        AZ::Data::AssetId m_instantiatingSliceAsset;
        AZStd::atomic_int m_stressLoadPending;
        AZStd::vector<AZ::Data::Asset<AZ::SliceAsset> > m_stressTestSliceAssets;

        enum
        {
            Stress_Descendents = 3,
            Stress_Generations = 5,
        };

        SliceInteractiveWorkflowTest()
        {
        }

        ~SliceInteractiveWorkflowTest() override
        {
            EntityTestbed::Destroy();
        }

        void OnSetup() override
        {
            auto* catalogBus = AZ::Data::AssetCatalogRequestBus::FindFirstHandler();
            if (catalogBus)
            {
                // Register asset types the asset DB should query our catalog for.
                catalogBus->AddAssetType(AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());
                catalogBus->AddAssetType(AZ::AzTypeInfo<AZ::ScriptAsset>::Uuid());

                // Build the catalog (scan).
                catalogBus->AddExtension(".xml");
                catalogBus->AddExtension(".lua");
            }
        }

        void OnReflect(AZ::SerializeContext& context, AZ::Entity& systemEntity) override
        {
            (void)context;
            (void)systemEntity;

            TestComponent::Reflect(&context);
            TestComponent1::Reflect(&context);
        }

        void OnAddButtons(QHBoxLayout& layout) override
        {
            QPushButton* sliceSelected = new QPushButton(QString("New Slice"));
            QPushButton* sliceInherit = new QPushButton(QString("Inherit Slice"));
            QPushButton* sliceInstance = new QPushButton(QString("Instantiate Slice"));
            QPushButton* saveRoot = new QPushButton(QString("Save Root"));
            QPushButton* stressGen = new QPushButton(QString("Stress Gen"));
            QPushButton* stressLoad = new QPushButton(QString("Stress Load"));
            QPushButton* stressInst = new QPushButton(QString("Stress Inst"));
            QPushButton* stressAll = new QPushButton(QString("Stress All"));
            stressInst->setEnabled(false);
            layout.addWidget(sliceSelected);
            layout.addWidget(sliceInherit);
            layout.addWidget(sliceInstance);
            layout.addWidget(saveRoot);
            layout.addWidget(stressGen);
            layout.addWidget(stressLoad);
            layout.addWidget(stressInst);
            layout.addWidget(stressAll);

            m_qtApplication->connect(sliceSelected, &QPushButton::pressed, [ this ]() { this->CreateSlice(false); });
            m_qtApplication->connect(sliceInherit, &QPushButton::pressed, [this](){this->CreateSlice(true); });
            m_qtApplication->connect(sliceInstance, &QPushButton::pressed, [ this ]() { this->InstantiateSlice(); });
            m_qtApplication->connect(saveRoot, &QPushButton::pressed, [ this ]() { this->SaveRoot(); });
            m_qtApplication->connect(stressGen, &QPushButton::pressed, [ this ]() { this->StressGen(); });
            m_qtApplication->connect(stressLoad, &QPushButton::pressed, [ this, stressInst ]()
                {
                    if (this->StressLoad())
                    {
                        stressInst->setEnabled(true);
                    }
                });
            m_qtApplication->connect(stressInst, &QPushButton::pressed, [ this ]() { this->StressInst(); });
            m_qtApplication->connect(stressAll, &QPushButton::pressed,
                [ this ]()
                {
                    this->StressGen();
                    this->StressLoad();
                    this->StressInst();
                }
                );
        }

        void OnEntityAdded(AZ::Entity& entity) override
        {
            (void)entity;

            entity.CreateComponent<TestComponent>();
        }

        void StressGenDrill(const AZ::Data::Asset<AZ::SliceAsset>& parent, size_t& nextSliceIndex, size_t generation, size_t& slicesCreated)
        {
            AZ::Data::Asset<AZ::SliceAsset> descendents[Stress_Descendents];

            for (size_t i = 0; i < Stress_Descendents; ++i)
            {
                AZ::Entity* entity = new AZ::Entity;
                auto* slice = entity->CreateComponent<AZ::SliceComponent>();
                {
                    slice->AddSlice(parent);
                    AZ::SliceComponent::EntityList entities;
                    slice->GetEntities(entities);

                    entities[0]->SetName(AZStd::string::format("Gen%zu_Descendent%zu_%zu", generation, i, nextSliceIndex));
                    entities[1]->SetName(AZStd::string::format("Gen%zu_Descendent%zu_%zu", generation, i, nextSliceIndex + 1));
                    //entities[0]->FindComponent<TestComponent>()->m_floatValue = float(nextSliceIndex) + 0.1f;
                    //entities[0]->FindComponent<TestComponent>()->m_intValue = int(generation);
                    //entities[1]->FindComponent<TestComponent>()->m_floatValue = float(nextSliceIndex) + 0.2f;
                }

                char assetFile[512];
                azsnprintf(assetFile, AZ_ARRAY_SIZE(assetFile), "GeneratedSlices/Gen%zu_Descendent%zu_%zu.xml", generation, i, nextSliceIndex++);

                AZ::Data::AssetId assetId;
                EBUS_EVENT_RESULT(assetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, assetFile, azrtti_typeid<AZ::SliceAsset>(), true);

                AZ::Utils::SaveObjectToFile(assetFile, AZ::DataStream::ST_XML, entity);

                slicesCreated += 1;

                descendents[i].Create(assetId, false);
                descendents[i].Get()->SetData(entity, slice, false);
            }

            // Drill down on next generation of inheritence.
            if (generation + 1 < Stress_Generations)
            {
                for (size_t i = 0; i < Stress_Descendents; ++i)
                {
                    StressGenDrill(descendents[i], nextSliceIndex, generation + 1, slicesCreated);
                }
            }
        }

        void StressGen()
        {
            ResetRoot();

            // Build a base slice containing two entities.
            AZ::Entity* e1 = new AZ::Entity();
            e1->SetName("Gen0_Left");
            //auto* c1 = e1->CreateComponent<TestComponent>();
            //c1->m_floatValue = 0.1f;

            AZ::Entity* e2 = new AZ::Entity();
            e2->SetName("Gen0_Right");
            //auto* c2 = e2->CreateComponent<TestComponent>();
            //c2->m_floatValue = 0.2f;

            AZ::Entity* root = new AZ::Entity();
            auto* slice = root->CreateComponent<AZ::SliceComponent>();
            slice->AddEntity(e1);
            slice->AddEntity(e2);

            AZ::Utils::SaveObjectToFile("GeneratedSlices/Gen0.xml", AZ::DataStream::ST_XML, root);

            // Build a deep binary tree, where we create two branches of each slice, each with a different
            // override from the parent.

            AZ::Data::AssetId assetId;
            EBUS_EVENT_RESULT(assetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, "GeneratedSlices/Gen0.xml", azrtti_typeid<AZ::SliceAsset>(), true);

            AZ::Data::Asset<AZ::SliceAsset> baseSliceAsset;
            baseSliceAsset.Create(assetId, false);
            baseSliceAsset.Get()->SetData(root, slice);

            // Generate tree to Stress_Generations # of generations.
            size_t nextSliceIndex = 1;
            size_t slicesCreated = 1;
            (void)nextSliceIndex;
            (void)slicesCreated;
            StressGenDrill(baseSliceAsset, nextSliceIndex, 1, slicesCreated);

            AZ_TracePrintf("Debug", "Done generating %u assets\n", slicesCreated);
        }

        void StressLoadDrill(size_t& nextSliceIndex, size_t generation, AZStd::atomic_int& pending, size_t& assetsLoaded)
        {
            for (size_t i = 0; i < Stress_Descendents; ++i)
            {
                char assetFile[512];
                azsnprintf(assetFile, AZ_ARRAY_SIZE(assetFile), "GeneratedSlices/Gen%zu_Descendent%zu_%zu.xml", generation, i, nextSliceIndex++);

                AZ::Data::AssetId assetId;
                EBUS_EVENT_RESULT(assetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, assetFile, azrtti_typeid<AZ::SliceAsset>(), true);

                if (assetId.IsValid())
                {
                    ++pending;
                    AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);

                    AZ::Data::Asset<AZ::SliceAsset> asset;
                    if (!asset.Create(assetId, true))
                    {
                        AZ_Error("Debug", false, "Asset %s could not be created.", assetFile);
                        --pending;
                    }

                    ++assetsLoaded;
                }
                else
                {
                    AZ_Error("Debug", false, "Asset %s could not be found.", assetFile);
                }
            }

            if (generation + 1 < Stress_Generations)
            {
                for (size_t i = 0; i < Stress_Descendents; ++i)
                {
                    StressLoadDrill(nextSliceIndex, generation + 1, pending, assetsLoaded);
                }
            }
        }

        void StressInstDrill(const AZ::Data::Asset<AZ::SliceAsset>& asset, size_t& nextSliceIndex, size_t generation, size_t& slicesInstantiated)
        {
            // Recurse...
            if (generation < Stress_Generations)
            {
                for (size_t i = 0; i < Stress_Descendents; ++i)
                {
                    char assetFile[512];
                    azsnprintf(assetFile, AZ_ARRAY_SIZE(assetFile), "GeneratedSlices/Gen%zu_Descendent%zu_%zu.xml", generation, i, nextSliceIndex++);

                    AZ_Error("Debug", asset.IsReady(), "Asset %s not ready?", assetFile);

                    StressInstDrill(asset, nextSliceIndex, generation + 1, slicesInstantiated);
                }
            }

            if (asset.IsReady())
            {
                AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Broadcast(
                    &AzToolsFramework::SliceEditorEntityOwnershipServiceRequests::InstantiateEditorSlice,
                    asset, AZ::Transform::CreateIdentity());

                ++slicesInstantiated;
            }
        }

        bool StressLoad()
        {
            m_instantiatingSliceAsset.SetInvalid();
            m_stressTestSliceAssets.clear();
            m_stressLoadPending = 0;

            ResetRoot();

            // Preload all slice assets.
            AZ::Data::AssetId rootAssetId;
            EBUS_EVENT_RESULT(rootAssetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, "GeneratedSlices/Gen0.xml", azrtti_typeid<AZ::SliceAsset>(), true);
            if (rootAssetId.IsValid())
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(rootAssetId);

                ++m_stressLoadPending;

                AZ::Data::Asset<AZ::SliceAsset> baseSliceAsset;
                if (!baseSliceAsset.Create(rootAssetId, true))
                {
                    return false;
                }

                const AZStd::chrono::steady_clock::time_point startTime = AZStd::chrono::steady_clock::now();

                size_t nextIndex = 1;
                size_t assetsLoaded = 1;
                StressLoadDrill(nextIndex, 1, m_stressLoadPending, assetsLoaded);

                while (m_stressLoadPending > 0)
                {
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
                    EBUS_EVENT(AZ::TickBus, OnTick, 0.3f, AZ::ScriptTimePoint());
                }

                const AZStd::chrono::steady_clock::time_point assetLoadFinishTime = AZStd::chrono::steady_clock::now();

                AZ_Printf("StressTest", "All Assets Loaded: %u assets, took %.2f ms\n", assetsLoaded,
                    float(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(assetLoadFinishTime - startTime).count()) * 0.001f);

                return true;
            }

            return false;
        }

        bool StressInst()
        {
            ResetRoot();

            // Instantiate from the bottom generation up.
            {
                AZ::Data::AssetId assetId;
                EBUS_EVENT_RESULT(assetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, "GeneratedSlices/Gen0.xml", azrtti_typeid<AZ::SliceAsset>(), true);

                AZ::Data::Asset<AZ::SliceAsset> baseSliceAsset;
                baseSliceAsset.Create(assetId, false);

                if (baseSliceAsset.IsReady())
                {
                    size_t nextIndex = 1;
                    size_t slices = 0;
                    size_t liveAllocs = 0;
                    [[maybe_unused]] size_t totalAllocs = 0;

                    auto cb = [&liveAllocs](void*, const AZ::Debug::AllocationInfo&, unsigned char)
                        {
                            ++liveAllocs;
                            return true;
                        };

                    AZ::AllocatorInstance<AZ::SystemAllocator>::GetAllocator().GetRecords()->EnumerateAllocations(cb);
                    totalAllocs = AZ::AllocatorInstance<AZ::SystemAllocator>::GetAllocator().GetRecords()->RequestedAllocs();
                    AZ_TracePrintf("StressTest", "Allocs Before Inst: %u live, %u total\n", liveAllocs, totalAllocs);

                    const AZStd::chrono::steady_clock::time_point startTime = AZStd::chrono::steady_clock::now();
                    StressInstDrill(baseSliceAsset, nextIndex, 1, slices);
                    const AZStd::chrono::steady_clock::time_point instantiateFinishTime = AZStd::chrono::steady_clock::now();

                    liveAllocs = 0;
                    totalAllocs = 0;
                    AZ::AllocatorInstance<AZ::SystemAllocator>::GetAllocator().GetRecords()->EnumerateAllocations(cb);
                    totalAllocs = AZ::AllocatorInstance<AZ::SystemAllocator>::GetAllocator().GetRecords()->RequestedAllocs();
                    AZ_TracePrintf("StressTest", "Allocs AfterInst: %u live, %u total\n", liveAllocs, totalAllocs);
                    // 1023 slices, 2046 entities
                    // Before         -> After          = Delta
                    // (Live)|(Total) -> (Live)|(Total) = (Live)|(Total)
                    // 28626 | 171792 -> 53157 | 533638 = 24531 | 361846
                    // 38842 | 533654 -> 53157 | 716707 = 14315 | 183053
                    // 38842 | 716723 -> 53157 | 899776 = 14315 | 183053
                    AZ::SliceComponent* rootSlice;
                    AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
                        rootSlice, &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
                    AZ::SliceComponent::EntityList entities;
                    entities.reserve(128);
                    rootSlice->GetEntities(entities);

                    AZ_Printf("StressTest", "All Assets Instantiated: %u slices, %u entities, took %.2f ms\n", slices, entities.size(),
                        float(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(instantiateFinishTime - startTime).count()) * 0.001f);

                    return true;
                }
            }

            return false;
        }

        void CreateSlice(bool inherit)
        {
            (void)inherit;

            static AZ::u32 sliceCounter = 1;

            AzToolsFramework::EntityIdList selected;
            EBUS_EVENT_RESULT(selected, AzToolsFramework::ToolsApplicationRequests::Bus, GetSelectedEntities);

            AZ::SliceComponent* rootSlice = nullptr;
            AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
                rootSlice, &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
            AZ_Assert(rootSlice, "Failed to get root slice.");

            if (!selected.empty())
            {
                AZ::Entity newEntity(AZStd::string::format("Slice%u", sliceCounter).c_str());
                AZ::SliceComponent* newSlice = newEntity.CreateComponent<AZ::SliceComponent>();

                AZStd::vector<AZ::Entity*> reclaimFromSlice;
                AZStd::vector<AZ::SliceComponent::SliceInstanceAddress> sliceInstances;

                // Add all selected entities.
                for (AZ::EntityId id : selected)
                {
                    AZ::Entity* entity = nullptr;
                    EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, id);
                    if (entity)
                    {
                        AZ::SliceComponent::SliceInstanceAddress sliceAddress = rootSlice->FindSlice(entity);
                        if (sliceAddress.IsValid())
                        {
                            // This entity already belongs to a slice instance, so inherit that instance (the whole thing for now).
                            if (sliceInstances.end() == AZStd::find(sliceInstances.begin(), sliceInstances.end(), sliceAddress))
                            {
                                sliceInstances.push_back(sliceAddress);
                            }
                        }
                        else
                        {
                            // Otherwise add loose.
                            newSlice->AddEntity(entity);
                            reclaimFromSlice.push_back(entity);
                        }
                    }
                }

                for (AZ::SliceComponent::SliceInstanceAddress& info : sliceInstances)
                {
                    info = newSlice->AddSliceInstance(info.GetReference(), info.GetInstance());
                }

                const QString saveAs = QFileDialog::getSaveFileName(nullptr,
                        QString("Save As..."), QString("."), QString("Xml Files (*.xml)"));
                if (!saveAs.isEmpty())
                {
                    AZ::Utils::SaveObjectToFile(saveAs.toUtf8().constData(), AZ::DataStream::ST_XML, &newEntity);
                }

                // Reclaim entities.
                for (AZ::Entity* entity : reclaimFromSlice)
                {
                    newSlice->RemoveEntity(entity, false);
                }

                // Reclaim slices.
                for (AZ::SliceComponent::SliceInstanceAddress& info : sliceInstances)
                {
                    rootSlice->AddSliceInstance(info.GetReference(), info.GetInstance());
                }

                ++sliceCounter;
            }
        }

        void InstantiateSlice()
        {
            const QString loadFrom = QFileDialog::getOpenFileName(nullptr,
                    QString("Select Slice..."), QString("."), QString("Xml Files (*.xml)"));

            if (!loadFrom.isEmpty())
            {
                AZ::Data::AssetId assetId;
                EBUS_EVENT_RESULT(assetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, loadFrom.toUtf8().constData(), azrtti_typeid<AZ::SliceAsset>(), true);

                AZ::Data::Asset<AZ::SliceAsset> baseSliceAsset;
                baseSliceAsset.Create(assetId, true);
                m_instantiatingSliceAsset = baseSliceAsset.GetId();

                AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
            }
        }
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

            if (asset.GetId() == m_instantiatingSliceAsset)
            {
            }
            else
            {
                --m_stressLoadPending;
            }
        }

        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

            if (asset.GetId() == m_instantiatingSliceAsset)
            {
                if (asset.Get() == nullptr)
                {
                    return;
                }

                m_instantiatingSliceAsset.SetInvalid();

                // Just add the slice to the level slice.
                AZ::Data::Asset<AZ::SliceAsset> sliceAsset = asset;
                AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Broadcast(
                    &AzToolsFramework::SliceEditorEntityOwnershipServiceRequests::InstantiateEditorSlice,
                    asset, AZ::Transform::CreateIdentity());

                // Init everything in the slice.
                AZ::SliceComponent* rootSlice = nullptr;
                AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
                    rootSlice, &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
                AZ_Assert(rootSlice, "Failed to get root slice.");
                AZ::SliceComponent::EntityList entities;
                rootSlice->GetEntities(entities);
                for (AZ::Entity* entity : entities)
                {
                    if (entity->GetState() == AZ::Entity::State::Constructed)
                    {
                        entity->Init();
                    }
                }

                m_entityCounter += AZ::u32(entities.size());
            }
            else
            {
                m_stressTestSliceAssets.push_back(asset);
                --m_stressLoadPending;
            }
        }

        void run()
        {
            int argc = 0;
            char* argv = nullptr;
            Run(argc, &argv);
        }
    }; // class SliceInteractiveWorkflowTest

    TEST_F(SliceInteractiveWorkflowTest, DISABLED_Test)
    {
        run();
    }

    class MinimalEntityWorkflowTester
        : public EntityTestbed
    {
    public:
        void run()
        {
            int argc = 0;
            char* argv = nullptr;
            Run(argc, &argv);
        }

        void OnEntityAdded(AZ::Entity& entity) override
        {
            // Add your components.
            entity.CreateComponent<AzToolsFramework::Components::TransformComponent>();
        }
    };

    TEST_F(MinimalEntityWorkflowTester, DISABLED_Test)
    {
        run();
    }

    class SortTransformParentsBeforeChildrenTest
        : public ScopedAllocatorSetupFixture
    {
    protected:
        AZStd::vector<AZ::Entity*> m_unsorted;
        AZStd::vector<AZ::Entity*> m_sorted;

        void TearDown() override
        {
            for (AZ::Entity* entity : m_unsorted)
            {
                delete entity;
            }
            m_unsorted.clear();
            m_sorted.clear();
        }

        // Entity IDs to use in tests
        AZ::EntityId E1 = AZ::EntityId(1);
        AZ::EntityId E2 = AZ::EntityId(2);
        AZ::EntityId E3 = AZ::EntityId(3);
        AZ::EntityId E4 = AZ::EntityId(4);
        AZ::EntityId E5 = AZ::EntityId(5);
        AZ::EntityId E6 = AZ::EntityId(6);
        AZ::EntityId MissingNo = AZ::EntityId(999);

        // add entity to m_unsorted
        void AddEntity(AZ::EntityId id, AZ::EntityId parentId = AZ::EntityId())
        {
            m_unsorted.push_back(aznew AZ::Entity(id));
            m_unsorted.back()->CreateComponent<AzFramework::TransformComponent>()->SetParent(parentId);
        }

        void SortAndSanityCheck()
        {
            m_sorted = m_unsorted;
            AzToolsFramework::SortTransformParentsBeforeChildren(m_sorted);

            // sanity check that all entries are still there
            EXPECT_TRUE(DoSameEntriesExistAfterSort());
        }

        bool DoSameEntriesExistAfterSort()
        {
            if (m_sorted.size() != m_unsorted.size())
            {
                return false;
            }

            for (AZ::Entity* entity : m_unsorted)
            {
                // compare counts in case multiple entries are identical (ex: 2 nullptrs)
                size_t unsortedCount = Count(entity, m_unsorted);
                size_t sortedCount = Count(entity, m_sorted);
                if (sortedCount < 1 || sortedCount != unsortedCount)
                {
                    return false;
                }
            }

            return true;
        }

        int Count(const AZ::Entity* value, const AZStd::vector<AZ::Entity*>& entityList)
        {
            int count = 0;
            for (const AZ::Entity* entity : entityList)
            {
                if (entity == value)
                {
                    ++count;
                }
            }
            return count;
        }

        bool IsChildAfterParent(AZ::EntityId childId, AZ::EntityId parentId)
        {
            int parentIndex = -1;
            int childIndex = -1;
            for (int i = 0; i < m_sorted.size(); ++i)
            {
                if (m_sorted[i] && (m_sorted[i]->GetId() == parentId) && (parentIndex == -1))
                {
                    parentIndex = i;
                }
                if (m_sorted[i] && (m_sorted[i]->GetId() == childId) && (childIndex == -1))
                {
                    childIndex = i;
                }
            }

            EXPECT_NE(childIndex, -1);
            EXPECT_NE(parentIndex, -1);
            return childIndex > parentIndex;
        }
    };

    TEST_F(SortTransformParentsBeforeChildrenTest, 0Entities_IsOk)
    {
        SortAndSanityCheck();
    }

    TEST_F(SortTransformParentsBeforeChildrenTest, 1Entity_IsOk)
    {
        AddEntity(E1);

        SortAndSanityCheck();
    }

    TEST_F(SortTransformParentsBeforeChildrenTest, ParentAndChild_SortsCorrectly)
    {
        AddEntity(E2, E1);
        AddEntity(E1);

        SortAndSanityCheck();

        EXPECT_TRUE(IsChildAfterParent(E2, E1));
    }

    TEST_F(SortTransformParentsBeforeChildrenTest, 6EntitiesWith2Roots_SortsCorrectly)
    {
        // Hierarchy looks like:
        // 1
        // + 2
        //   + 3
        // 4
        // + 5
        // + 6
        // The entities are added in "randomish" order on purpose
        AddEntity(E3, E2);
        AddEntity(E1);
        AddEntity(E6, E4);
        AddEntity(E5, E4);
        AddEntity(E2, E1);
        AddEntity(E4);

        SortAndSanityCheck();

        EXPECT_TRUE(IsChildAfterParent(E2, E1));
        EXPECT_TRUE(IsChildAfterParent(E3, E2));
        EXPECT_TRUE(IsChildAfterParent(E5, E4));
        EXPECT_TRUE(IsChildAfterParent(E6, E4));
    }

    TEST_F(SortTransformParentsBeforeChildrenTest, ParentNotFound_ChildTreatedAsRoot)
    {
        AddEntity(E1);
        AddEntity(E2, E1);
        AddEntity(E3, MissingNo); // E3's parent not found
        AddEntity(E4, E3);

        SortAndSanityCheck();

        EXPECT_TRUE(IsChildAfterParent(E2, E1));
        EXPECT_TRUE(IsChildAfterParent(E4, E2));
    }

    TEST_F(SortTransformParentsBeforeChildrenTest, NullptrEntry_IsToleratedButNotSorted)
    {
        AddEntity(E2, E1);
        m_unsorted.push_back(nullptr);
        AddEntity(E1);

        SortAndSanityCheck();

        EXPECT_TRUE(IsChildAfterParent(E2, E1));
    }

    TEST_F(SortTransformParentsBeforeChildrenTest, DuplicateEntityId_IsToleratedButNotSorted)
    {
        AddEntity(E2, E1);
        AddEntity(E1);
        AddEntity(E1); // duplicate EntityId

        SortAndSanityCheck();

        EXPECT_TRUE(IsChildAfterParent(E2, E1));
    }

    TEST_F(SortTransformParentsBeforeChildrenTest, DuplicateEntityPtr_IsToleratedButNotSorted)
    {
        AddEntity(E2, E1);
        AddEntity(E1);
        m_unsorted.push_back(m_unsorted.back()); // duplicate Entity pointer

        SortAndSanityCheck();

        m_unsorted.pop_back(); // remove duplicate ptr so we don't double-delete during teardown

        EXPECT_TRUE(IsChildAfterParent(E2, E1));
    }

    TEST_F(SortTransformParentsBeforeChildrenTest, LoopingHierarchy_PicksAnyParentAsRoot)
    {
        // loop: E1 -> E2 -> E3 -> E1 -> ...
        AddEntity(E2, E1);
        AddEntity(E3, E2);
        AddEntity(E1, E3);

        SortAndSanityCheck();

        AZ::EntityId first = m_sorted.front()->GetId();

        if (first == E1)
        {
            EXPECT_TRUE(IsChildAfterParent(E2, E1));
            EXPECT_TRUE(IsChildAfterParent(E3, E2));
        }
        else if (first == E2)
        {
            EXPECT_TRUE(IsChildAfterParent(E3, E2));
            EXPECT_TRUE(IsChildAfterParent(E1, E3));
        }
        else // if (first == E3)
        {
            EXPECT_TRUE(IsChildAfterParent(E1, E3));
            EXPECT_TRUE(IsChildAfterParent(E2, E1));
        }
    }

    TEST_F(SortTransformParentsBeforeChildrenTest, EntityLackingTransformComponent_IsTreatedLikeItHasNoParent)
    {
        AddEntity(E2, E1);
        m_unsorted.push_back(aznew AZ::Entity(E1)); // E1 has no components

        SortAndSanityCheck();

        EXPECT_TRUE(IsChildAfterParent(E2, E1));
    }

    TEST_F(SortTransformParentsBeforeChildrenTest, EntityParentedToSelf_IsTreatedLikeItHasNoParent)
    {
        AddEntity(E2, E1);
        AddEntity(E1, E1); // parented to self

        SortAndSanityCheck();

        EXPECT_TRUE(IsChildAfterParent(E2, E1));
    }

    TEST_F(SortTransformParentsBeforeChildrenTest, EntityWithInvalidId_IsToleratedButNotSorted)
    {
        AddEntity(E2, E1);
        AddEntity(E1);
        AddEntity(AZ::EntityId()); // entity using invalid ID as its own ID

        SortAndSanityCheck();

        EXPECT_TRUE(IsChildAfterParent(E2, E1));
    }

    class TestExportRuntimeComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(TestExportRuntimeComponent, "{C984534F-C907-4968-B9D3-AF2A99CBD678}", AZ::Component);

        TestExportRuntimeComponent() {}

        TestExportRuntimeComponent(bool returnPointerToSelf, bool exportHandled) :
            m_returnPointerToSelf(returnPointerToSelf),
            m_exportHandled(exportHandled)
        {}

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestExportRuntimeComponent, AZ::Component>()
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<TestExportRuntimeComponent>(
                        "Test Export Runtime Component", "Validate different options for exporting runtime components")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::RuntimeExportCallback, &TestExportRuntimeComponent::ExportComponent)
                        ;
                }
            }
        }

        AZ::ExportedComponent ExportComponent(AZ::Component* thisComponent, const AZ::PlatformTagSet& /*platformTags*/)
        {
            return AZ::ExportedComponent(m_returnPointerToSelf ? thisComponent : nullptr, false, m_exportHandled);
        }

        bool m_returnPointerToSelf = false;
        bool m_exportHandled = false;
    };

    class TestExportOtherRuntimeComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(TestExportOtherRuntimeComponent, "{7EEDCE0A-2D5F-4017-A20B-9224E52D75B8}");

        void Activate() override {}
        void Deactivate() override {}
        static void Reflect(ReflectContext* context)
        {
            auto* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<TestExportOtherRuntimeComponent, AZ::Component>()
                    ;
            }
        }
    };


    class SliceTestExportEditorComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_COMPONENT(SliceTestExportEditorComponent, "{8FA877A2-38E6-49AD-B31E-71B86DC8BB03}", AzToolsFramework::Components::EditorComponentBase);

        enum ExportComponentType
        {
            EXPORT_EDITOR_COMPONENT,
            EXPORT_RUNTIME_COMPONENT,
            EXPORT_OTHER_RUNTIME_COMPONENT,
            EXPORT_NULL_COMPONENT
        };

        SliceTestExportEditorComponent() {}

        SliceTestExportEditorComponent(ExportComponentType exportType, bool exportHandled) :
            m_exportType(exportType),
            m_exportHandled(exportHandled)
        {}

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SliceTestExportEditorComponent, AzToolsFramework::Components::EditorComponentBase>()
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<SliceTestExportEditorComponent>(
                        "Test Export Editor Component", "Validate different options for exporting editor components")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::RuntimeExportCallback, &SliceTestExportEditorComponent::ExportComponent)
                        ;
                }
            }
        }

        AZ::ExportedComponent ExportComponent(AZ::Component* thisComponent, const AZ::PlatformTagSet& /*platformTags*/)
        {
            switch (m_exportType)
            {
                case EXPORT_EDITOR_COMPONENT:
                    return AZ::ExportedComponent(thisComponent, false, m_exportHandled);
                case EXPORT_RUNTIME_COMPONENT:
                    return AZ::ExportedComponent(aznew TestExportRuntimeComponent(true, true), true, m_exportHandled);
                case EXPORT_OTHER_RUNTIME_COMPONENT:
                    return AZ::ExportedComponent(aznew TestExportOtherRuntimeComponent(), true, m_exportHandled);
                case EXPORT_NULL_COMPONENT:
                    return AZ::ExportedComponent(nullptr, false, m_exportHandled);
            }

            return AZ::ExportedComponent();
        }

        void BuildGameEntity(AZ::Entity* gameEntity) override
        {
            gameEntity->CreateComponent<TestExportRuntimeComponent>(true, true);
        }

        ExportComponentType m_exportType = EXPORT_NULL_COMPONENT;
        bool m_exportHandled = false;
    };


    class SliceCompilerTest
        : public UnitTest::AllocatorsTestFixture
    {
    protected:
        AzToolsFramework::ToolsApplication m_app;

        AZ::Data::Asset<AZ::SliceAsset> m_editorSliceAsset;
        AZ::SliceComponent* m_editorSliceComponent = nullptr;

        AZ::Data::Asset<AZ::SliceAsset> m_compiledSliceAsset;
        AZ::SliceComponent* m_compiledSliceComponent = nullptr;

        void SetUp() override
        {
            AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
            auto projectPathKey =
                AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
            AZ::IO::FixedMaxPath enginePath;
            registry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
            registry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

            m_app.Start(AzFramework::Application::Descriptor());

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            m_app.RegisterComponentDescriptor(TestExportRuntimeComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(TestExportOtherRuntimeComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(SliceTestExportEditorComponent::CreateDescriptor());

            m_editorSliceAsset = Data::AssetManager::Instance().CreateAsset<SliceAsset>(Data::AssetId(Uuid::CreateRandom()));

            AZ::Entity* editorSliceEntity = aznew AZ::Entity();
            m_editorSliceComponent = editorSliceEntity->CreateComponent<AZ::SliceComponent>();
            m_editorSliceAsset.Get()->SetData(editorSliceEntity, m_editorSliceComponent);
        }

        void TearDown() override
        {
            m_compiledSliceComponent = nullptr;
            m_compiledSliceAsset.Release();
            m_editorSliceComponent = nullptr;
            m_editorSliceAsset.Release();
            m_app.Stop();
        }

        // create entity with a given parent in the editor slice
        void CreateEditorEntity(AZ::u64 id, const char* name, AZ::u64 parentId = (AZ::u64)AZ::EntityId())
        {
            AZ::Entity* entity = aznew AZ::Entity(AZ::EntityId(id), name);
            AzToolsFramework::Components::TransformComponent* transformComponent = entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
            transformComponent->SetParent(AZ::EntityId(parentId));

            m_editorSliceComponent->AddEntity(entity);
        }

        // create entity containing the EditorOnly component in the editor slice
        void CreateEditorOnlyEntity(const char* name, bool editorOnly)
        {
            AZ::Entity* entity = aznew AZ::Entity(name);
            entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
            entity->CreateComponent<AzToolsFramework::Components::EditorOnlyEntityComponent>();
            m_editorSliceComponent->AddEntity(entity);

            entity->Init();
            EXPECT_EQ(AZ::Entity::State::Init, entity->GetState());
            entity->Activate();
            EXPECT_EQ(AZ::Entity::State::Active, entity->GetState());

            AzToolsFramework::EditorOnlyEntityComponentRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorOnlyEntityComponentRequests::SetIsEditorOnlyEntity, editorOnly);

        }

        // create entity containing the EditorOnly component in the editor slice
        void CreateTestExportRuntimeEntity(const char* name, bool returnPointerToSelf, bool exportHandled)
        {
            AZ::Entity* entity = aznew AZ::Entity(name);
            entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
            entity->CreateComponent<TestExportRuntimeComponent>(returnPointerToSelf, exportHandled);
            m_editorSliceComponent->AddEntity(entity);
        }

        // create entity containing the EditorOnly component in the editor slice
        void CreateTestExportEditorEntity(const char* name, SliceTestExportEditorComponent::ExportComponentType exportType, bool exportHandled)
        {
            AZ::Entity* entity = aznew AZ::Entity(name);
            entity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
            entity->CreateComponent<SliceTestExportEditorComponent>(exportType, exportHandled);
            m_editorSliceComponent->AddEntity(entity);
        }


        // compile m_editorSliceAsset -> m_compiledSliceAsset
        bool CompileSlice(bool expectSuccess = true)
        {
            AzToolsFramework::WorldEditorOnlyEntityHandler worldEditorOnlyEntityHandler;
            AzToolsFramework::EditorOnlyEntityHandlers handlers =
            {
                &worldEditorOnlyEntityHandler
            };
            AzToolsFramework::SliceCompilationResult compileResult = AzToolsFramework::CompileEditorSlice(m_editorSliceAsset, AZ::PlatformTagSet(), *m_app.GetSerializeContext(), handlers);

            EXPECT_EQ(compileResult.IsSuccess(), expectSuccess);
            if (compileResult.IsSuccess())
            {
                m_compiledSliceAsset = AZStd::move(compileResult.GetValue());
                m_compiledSliceComponent = m_compiledSliceAsset.Get()->GetComponent();
                return true;
            }

            return false;
        }

        // check order of entities in compiled slice
        // reference entities by name, since they have different IDs in compiled slice
        bool IsChildAfterParent(const char* childName, const char* parentName)
        {
            AZStd::vector<AZ::Entity*> entities;
            m_compiledSliceComponent->GetEntities(entities);

            AZ::Entity* parent = nullptr;
            for (AZ::Entity* entity : entities)
            {
                const AZStd::string& name = entity->GetName();
                if (name == parentName)
                {
                    parent = entity;
                }

                if (name == childName)
                {
                    return parent != nullptr;
                }
            }

            return false;
        }

        // Locate and return an entity from the compiled slice
        AZ::Entity* GetCompiledEntity(const char* entityName)
        {
            AZStd::vector<AZ::Entity*> entities;
            m_compiledSliceComponent->GetEntities(entities);

            for (AZ::Entity* entity : entities)
            {
                const AZStd::string& name = entity->GetName();
                if (name == entityName)
                {
                    return entity;
                }
            }

            return nullptr;
        }
    };

    TEST_F(SliceCompilerTest, EntitiesInCompiledSlice_SortedParentsBeforeChildren)
    {
        // Hieararchy looks like:
        // A
        // + B
        //   + C
        // D
        // + E
        // + F
        CreateEditorEntity(0xB, "B", 0xA);
        CreateEditorEntity(0xE, "E", 0xD);
        CreateEditorEntity(0xD, "D");
        CreateEditorEntity(0xA, "A");
        CreateEditorEntity(0xF, "F", 0xD);
        CreateEditorEntity(0xC, "C", 0xB);

        if (!CompileSlice())
        {
            return;
        }

        EXPECT_TRUE(IsChildAfterParent("B", "A"));
        EXPECT_TRUE(IsChildAfterParent("C", "B"));
        EXPECT_TRUE(IsChildAfterParent("E", "D"));
        EXPECT_TRUE(IsChildAfterParent("F", "D"));
    }

    TEST_F(SliceCompilerTest, EditorOnlyEntity_OnlyRuntimeEntityExported)
    {
        // Create one entity that's flagged as Editor-Only, and one that's enabled for runtime.
        CreateEditorOnlyEntity("EditorOnly", true);
        CreateEditorOnlyEntity("EditorAndRuntime", false);

        if (!CompileSlice())
        {
            return;
        }

        // Expected result:  Only the runtime entity exists in the exported slice.
        EXPECT_FALSE(GetCompiledEntity("EditorOnly"));
        EXPECT_TRUE(GetCompiledEntity("EditorAndRuntime"));
    }

    TEST_F(SliceCompilerTest, RuntimeExportCallback_RuntimeComponentExportedSuccessfully)
    {
        // Create a component that has a RuntimeExportCallback and successfully exports itself
        CreateTestExportRuntimeEntity("EntityWithRuntimeComponent", true, true);

        if (!CompileSlice())
        {
            return;
        }

        // Expected result:  exported slice contains the component.
        AZ::Entity* entity = GetCompiledEntity("EntityWithRuntimeComponent");
        EXPECT_TRUE(entity);
        EXPECT_TRUE(entity->FindComponent<TestExportRuntimeComponent>());
    }

    TEST_F(SliceCompilerTest, RuntimeExportCallback_RuntimeComponentExportSuppressed)
    {
        // Create a component that has a RuntimeExportCallback and successfully suppresses itself from exporting
        CreateTestExportRuntimeEntity("EntityWithRuntimeComponent", false, true);

        if (!CompileSlice())
        {
            return;
        }

        // Expected result:  exported slice does NOT contain the component.
        AZ::Entity* entity = GetCompiledEntity("EntityWithRuntimeComponent");
        EXPECT_TRUE(entity);
        EXPECT_FALSE(entity->FindComponent<TestExportRuntimeComponent>());
    }

    TEST_F(SliceCompilerTest, RuntimeExportCallback_RuntimeComponentExportUnhandled)
    {
        // Create a component that has a RuntimeExportCallback, returns a pointer to itself, but says it wasn't handled.
        CreateTestExportRuntimeEntity("EntityWithRuntimeComponent", true, false);

        if (!CompileSlice())
        {
            return;
        }

        // Expected result:  exported slice contains the component, because the default behavior is "clone/add" for
        // runtime components.
        AZ::Entity* entity = GetCompiledEntity("EntityWithRuntimeComponent");
        EXPECT_TRUE(entity);
        EXPECT_TRUE(entity->FindComponent<TestExportRuntimeComponent>());
    }

    TEST_F(SliceCompilerTest, RuntimeExportCallback_RuntimeComponentExportSuppressedAndUnhandled)
    {
        // Create a component that has a RuntimeExportCallback and suppresses itself from exporting, but says it wasn't handled
        CreateTestExportRuntimeEntity("EntityWithRuntimeComponent", false, false);

        if (!CompileSlice())
        {
            return;
        }

        // Expected result:  exported slice contains the component, because by saying it wasn't handled, it
        // should fall back on the default behavior of "clone/add" for runtime components.
        AZ::Entity* entity = GetCompiledEntity("EntityWithRuntimeComponent");
        EXPECT_TRUE(entity);
        EXPECT_TRUE(entity->FindComponent<TestExportRuntimeComponent>());
    }




    TEST_F(SliceCompilerTest, RuntimeExportCallback_EditorComponentExportedSuccessfully)
    {
        // Create an editor component that has a RuntimeExportCallback and successfully exports itself
        CreateTestExportEditorEntity("EntityWithEditorComponent", SliceTestExportEditorComponent::ExportComponentType::EXPORT_OTHER_RUNTIME_COMPONENT, true);

        if (!CompileSlice())
        {
            return;
        }

        // Expected result:  exported slice contains the OtherRuntime component, exported from RuntimeExportCallback.
        // (A result of Runtime component means BuildGameEntity() ran instead)
        AZ::Entity* entity = GetCompiledEntity("EntityWithEditorComponent");
        EXPECT_TRUE(entity);
        EXPECT_FALSE(entity->FindComponent<SliceTestExportEditorComponent>());
        EXPECT_FALSE(entity->FindComponent<TestExportRuntimeComponent>());
        EXPECT_TRUE(entity->FindComponent<TestExportOtherRuntimeComponent>());
    }

    TEST_F(SliceCompilerTest, RuntimeExportCallback_EditorComponentExportSuppressed)
    {
        // Create an editor component that has a RuntimeExportCallback and successfully suppresses itself from exporting
        CreateTestExportEditorEntity("EntityWithEditorComponent", SliceTestExportEditorComponent::ExportComponentType::EXPORT_NULL_COMPONENT, true);

        if (!CompileSlice())
        {
            return;
        }

        // Expected result:  exported slice does NOT contain either component.
        AZ::Entity* entity = GetCompiledEntity("EntityWithEditorComponent");
        EXPECT_TRUE(entity);
        EXPECT_FALSE(entity->FindComponent<SliceTestExportEditorComponent>());
        EXPECT_FALSE(entity->FindComponent<TestExportRuntimeComponent>());
        EXPECT_FALSE(entity->FindComponent<TestExportOtherRuntimeComponent>());
    }

    TEST_F(SliceCompilerTest, RuntimeExportCallback_EditorComponentExportUnhandledFallbackToBuildGameEntity)
    {
        // Create an editor component that has a RuntimeExportCallback, returns a pointer to itself, but says it wasn't handled.
        CreateTestExportEditorEntity("EntityWithEditorComponent", SliceTestExportEditorComponent::ExportComponentType::EXPORT_EDITOR_COMPONENT, false);

        if (!CompileSlice())
        {
            return;
        }

        // Expected result:  exported slice contains the runtime component, because the fallback to BuildGameEntity()
        // produced a runtime component.
        AZ::Entity* entity = GetCompiledEntity("EntityWithEditorComponent");
        EXPECT_TRUE(entity);
        EXPECT_FALSE(entity->FindComponent<SliceTestExportEditorComponent>());
        EXPECT_TRUE(entity->FindComponent<TestExportRuntimeComponent>());
        EXPECT_FALSE(entity->FindComponent<TestExportOtherRuntimeComponent>());
    }

    TEST_F(SliceCompilerTest, RuntimeExportCallback_EditorComponentExportSuppressedAndUnhandledFallbackToBuildGameEntity)
    {
        // Create an editor component that has a RuntimeExportCallback and suppresses itself from exporting, but says it wasn't handled
        CreateTestExportEditorEntity("EntityWithEditorComponent", SliceTestExportEditorComponent::ExportComponentType::EXPORT_NULL_COMPONENT, false);

        if (!CompileSlice())
        {
            return;
        }

        // Expected result:  exported slice contains the runtime component, because the fallback to BuildGameEntity()
        // produced a runtime component.
        AZ::Entity* entity = GetCompiledEntity("EntityWithEditorComponent");
        EXPECT_TRUE(entity);
        EXPECT_FALSE(entity->FindComponent<SliceTestExportEditorComponent>());
        EXPECT_TRUE(entity->FindComponent<TestExportRuntimeComponent>());
        EXPECT_FALSE(entity->FindComponent<TestExportOtherRuntimeComponent>());
    }

    TEST_F(SliceCompilerTest, RuntimeExportCallback_EditorComponentFailsToExportItself)
    {
        // Create an editor component that has a RuntimeExportCallback and suppresses itself from exporting, but says it wasn't handled
        CreateTestExportEditorEntity("EntityWithEditorComponent", SliceTestExportEditorComponent::ExportComponentType::EXPORT_EDITOR_COMPONENT, true);

        // We expect the slice compilation to fail, since an editor component is being exported as a game component
        CompileSlice(false);
    }
} // namespace UnitTest

