/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Entity/EditorEntityActionComponent.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorLayerComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <QColor>

namespace AzToolsFramework
{
    // Used to promote some functions to public so the unit tests can access them.
    class EditorLayerComponentTestHelper :
        public Layers::EditorLayerComponent
    {
    public:
        AZ_EDITOR_COMPONENT(EditorLayerComponentTestHelper, "{E21CAB65-BFDC-4DFC-B550-A8BF7F235BDA}", Layers::EditorLayerComponent);
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorLayerComponentTestHelper, Layers::EditorLayerComponent>()
                    ->Version(1);
            }
        }

        void ClearUnsavedChanges() { m_hasUnsavedChanges = false; }

        void SetSaveAsBinary(bool saveAsBinary) { m_editableLayerProperties.m_saveAsBinary = saveAsBinary; }
        bool GetSaveAsBinary() const { return m_editableLayerProperties.m_saveAsBinary; }

        // Forces a state where a layer is written to a stream with 2 entities with the same ID.
        // This allows for testing that the layer load logic safely removes duplicates.
        Layers::LayerResult CreateLayerStreamWithDuplicateEntity(
            AZStd::vector<AZ::Entity*>& entityList,
            AZ::SliceComponent::SliceReferenceToInstancePtrs& layerInstances,
            AZ::IO::ByteContainerStream<AZStd::vector<char> >& layerStream,
            Layers::EditorLayer& layer)
        {
            Layers::LayerResult layerPrepareResult = PrepareLayerForSaving(layer, entityList, layerInstances);
            if (!layerPrepareResult.IsSuccess())
            {
                return layerPrepareResult;
            }
            // Add the first entity in the list twice.
            layer.m_layerEntities.push_back(layer.m_layerEntities.front());
            return WriteLayerToStream(layer, layerStream);
        }

        // The unit tests are just testing the ability to save and load layers,
        // and not the ability to write this information to disk.
        // These functions allow this testing to occur.
        Layers::LayerResult PopulateLayerWriteToStreamAndGetEntities(
            AZStd::vector<AZ::Entity*>& entityList,
            AZ::SliceComponent::SliceReferenceToInstancePtrs& layerInstances,
            AZ::IO::ByteContainerStream<AZStd::vector<char> >& layerStream,
            Layers::EditorLayer& layer)
        {
            Layers::LayerResult layerPrepareResult = PrepareLayerForSaving(layer, entityList, layerInstances);
            if (!layerPrepareResult.IsSuccess())
            {
                return layerPrepareResult;
            }
            return WriteLayerToStream(layer, layerStream);
        }

        Layers::LayerResult ReadFromLayerStream(
            AZ::IO::ByteContainerStream<AZStd::vector<char> >& layerStream,
            AZ::SliceComponent::SliceAssetToSliceInstancePtrs& sliceInstances,
            AZStd::unordered_map<AZ::EntityId, AZ::Entity*>& uniqueEntities)
        {
            m_loadedLayer = AZ::Utils::LoadObjectFromStream<Layers::EditorLayer>(layerStream);
            EXPECT_NE(m_loadedLayer, nullptr);
            return PopulateFromLoadedLayerData(*m_loadedLayer, sliceInstances, uniqueEntities);
        }
    };

    struct EntityAndLayerComponent
    {
        AZ::Entity* m_entity = nullptr;
        // Not all functions being tested are available on the ebus, some need direct access.
        EditorLayerComponentTestHelper* m_layer = nullptr;
    };

    bool IsEntityInList(const AZStd::vector<AZ::Entity*>& entityList, AZ::Entity* entity)
    {
        return AZStd::find(entityList.begin(), entityList.end(), entity) != entityList.end();
    }

    bool IsEntityInLooseEditorEntities(AZ::Entity* entity)
    {
        AZStd::vector<AZ::Entity*> editorEntities;
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::GetLooseEditorEntities,
            editorEntities);
        return IsEntityInList(editorEntities, entity);
    }

    bool IsInstanceAndReferenceInRootSlice(AZ::SliceComponent* rootSlice, AZ::SliceComponent::SliceInstanceAddress sliceInstance)
    {
        // See if the rootSlice knows about the asset associated with the passed in instance.
        // These checks are round about, and not through FindSlice to make sure the root slice's list of references and instances is used.
        AZ::SliceComponent::SliceReference* sliceReference = rootSlice->GetSlice(sliceInstance.GetReference()->GetSliceAsset());
        if (!sliceReference)
        {
            return false;
        }
        // Checking this way makes sure we're using as much data from the rootSlice as possible.
        // When moving instances in and out of the root slice, the slice references can change.
        return sliceReference->FindInstance(sliceInstance.GetInstance()->GetId()) != nullptr;
    }

    bool IsLooseEntityInRootSlice(const AZ::SliceComponent* rootSlice, const AZ::EntityId layerEntityId)
    {
        const EntityList& looseEntities(rootSlice->GetNewEntities());

        for (const auto& entity : looseEntities)
        {
            if (entity->GetId() == layerEntityId)
            {
                return true;
            }
        }
        return false;
    }

    AZ::Entity* CreateEditorReadyEntity(const char* entityName)
    {
        AZ::EntityId createdEntityId;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            createdEntityId,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::CreateNewEditorEntity,
            entityName);
        EXPECT_TRUE(createdEntityId.IsValid());

        AZ::Entity* createdEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(createdEntity, &AZ::ComponentApplicationRequests::FindEntity, createdEntityId);
        EXPECT_NE(createdEntity, nullptr);

        EXPECT_EQ(createdEntity->GetState(), AZ::Entity::State::Active);
        createdEntity->Deactivate();
        EXPECT_EQ(createdEntity->GetState(), AZ::Entity::State::Init);

        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::AddRequiredComponents,
            *createdEntity);

        createdEntity->Activate();
        EXPECT_EQ(createdEntity->GetState(), AZ::Entity::State::Active);
        return createdEntity;
    }

    EntityAndLayerComponent CreateEntityWithLayer(const char* entityName)
    {
        EntityAndLayerComponent result;
        result.m_entity = CreateEditorReadyEntity(entityName);

        result.m_layer = aznew EditorLayerComponentTestHelper();
        AZStd::vector<AZ::Component*> newComponents;
        newComponents.push_back(result.m_layer);
        Components::EditorEntityActionComponent::AddExistingComponentsOutcome componentAddResult(
            AZ::Failure(AZStd::string("No listener on AddExistingComponentsToEntity bus.")));
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
            componentAddResult,
            &AzToolsFramework::EntityCompositionRequests::AddExistingComponentsToEntityById,
            result.m_entity->GetId(),
            newComponents);

        EXPECT_TRUE(componentAddResult.IsSuccess());

        // Make sure everything is setup correctly.
        EXPECT_NE(result.m_entity, nullptr);
        EXPECT_NE(result.m_layer, nullptr);
        EXPECT_EQ(result.m_entity->GetState(), AZ::Entity::State::Active);
        return result;
    }

    // Filter used to test component copyability. 
    // Ensure filter passes everything to ensure layer component copyability is unaffected by filter results.
    bool DummyComponentFilter([[maybe_unused]] const AZ::SerializeContext::ClassData& classData)
    {
        return true;
    }

    class SliceToolsTestApplication : public UnitTest::ToolsTestApplication
    {
    public:
        explicit SliceToolsTestApplication(AZStd::string applicationName)
            : ToolsTestApplication(applicationName)
        {
        }

    protected:
        bool IsPrefabSystemEnabled() const override
        {
            return false;
        }
    };


    class EditorLayerComponentTest
        : public UnitTest::AllocatorsTestFixture
        , public UnitTest::TraceBusRedirector
    {
    protected:
        void SetUp() override
        {
            m_app.Start(m_descriptor);

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            AZ::SerializeContext* context = m_app.GetSerializeContext();
            m_editorLayerComponentTestHelperDescriptor = EditorLayerComponentTestHelper::CreateDescriptor();
            m_editorLayerComponentTestHelperDescriptor->Reflect(context);

            m_layerEntity = CreateEntityWithLayer(m_entityName);
            AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            if (m_layerEntity.m_entity)
            {
                m_layerEntity.m_layer->CleanupLoadedLayer();
            }
            m_editorLayerComponentTestHelperDescriptor->ReleaseDescriptor();

            
            m_app.Stop();
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }

        // A few tests save a layer and want to check the state after saving.
        // A separate unit test actually validates all of the behavior in this function.
        Layers::LayerResult SaveMainLayer(Layers::EditorLayer& layerOutput)
        {
            AZStd::vector<char> entitySaveBuffer;
            AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
            return SaveMainLayer(layerOutput, entitySaveStream);
        }

        Layers::LayerResult SaveMainLayer(
            Layers::EditorLayer& layerOutput,
            AZ::IO::ByteContainerStream<AZStd::vector<char> > &streamOutput,
            AZStd::vector<AZ::Entity*>& layerEntities,
            AZ::SliceComponent::SliceReferenceToInstancePtrs& instancesInLayers)
        {
            return m_layerEntity.m_layer->PopulateLayerWriteToStreamAndGetEntities(
                layerEntities,
                instancesInLayers,
                streamOutput,
                layerOutput);
        }

        Layers::LayerResult SaveMainLayer(
            Layers::EditorLayer& layerOutput,
            AZ::IO::ByteContainerStream<AZStd::vector<char> > &streamOutput)
        {
            AZStd::vector<AZ::Entity*> layerEntities;
            AZ::SliceComponent::SliceReferenceToInstancePtrs instancesInLayers;
            return SaveMainLayer(layerOutput, streamOutput, layerEntities, instancesInLayers);
        }

        AZ::SliceComponent::SliceInstanceAddress CreateSliceInstanceFromSlice(
            AZ::SliceComponent* rootSlice,
            const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset)
        {
            EXPECT_NE(rootSlice, nullptr);
            AZ::SliceComponent::SliceInstanceAddress instantiatedSlice = rootSlice->AddSlice(sliceAsset);
            AZ::Entity* instanceEntity = GetEntityFromSliceInstance(instantiatedSlice);
            instanceEntity->Init();
            EXPECT_EQ(instanceEntity->GetState(), AZ::Entity::State::Init);
            instanceEntity->Activate();
            EXPECT_EQ(instanceEntity->GetState(), AZ::Entity::State::Active);
            return instantiatedSlice;
        }

        // Creates a slice instance, used to test serialization with layers.
        // Note that DeleteSliceInstance must be called.
        AZ::SliceComponent::SliceInstanceAddress CreateSliceInstance()
        {
            AZ::Data::Asset<AZ::SliceAsset> sliceAsset;
            AZ::Entity* sliceEntity = aznew AZ::Entity("SliceEntity");
            AZ::SliceComponent* sliceComponent = sliceEntity->CreateComponent<AZ::SliceComponent>();
            sliceComponent->SetSerializeContext(m_app.GetSerializeContext());

            AZ::Entity* entityInSlice = aznew AZ::Entity("EntityInSlice");
            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                &AzToolsFramework::EditorEntityContextRequestBus::Events::AddRequiredComponents,
                *entityInSlice);

            EntityList entitiesToAddToSlice;
            entitiesToAddToSlice.push_back(entityInSlice);
            sliceComponent->AddEntities(entitiesToAddToSlice);
            AZStd::string sliceAssetFile("Temp/GeneratedSlices/LayerTestSlice.slice");

            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetId,
                &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                sliceAssetFile.c_str(),
                azrtti_typeid<AZ::SliceAsset>(),
                true);
            sliceAsset.Create(assetId, false);
            sliceAsset.Get()->SetData(sliceEntity, sliceComponent, false);
            sliceComponent->Instantiate();

            AZ::SliceComponent* rootSlice;
            AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
                rootSlice, &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
            EXPECT_NE(rootSlice, nullptr);

            return CreateSliceInstanceFromSlice(rootSlice, sliceAsset);
        }

        AZ::Entity* GetEntityFromSliceInstance(AZ::SliceComponent::SliceInstanceAddress instantiatedSlice)
        {
            EXPECT_TRUE(instantiatedSlice.IsValid());
            EXPECT_NE(instantiatedSlice.GetInstance(), nullptr);
            EXPECT_NE(instantiatedSlice.GetInstance()->GetInstantiated(), nullptr);
            EXPECT_EQ(instantiatedSlice.GetInstance()->GetInstantiated()->m_entities.size(), 1);
            return instantiatedSlice.GetInstance()->GetInstantiated()->m_entities[0];
        }

        void DeleteSliceInstance(AZ::SliceComponent::SliceInstanceAddress instance)
        {
            AZ::SliceComponent* rootSlice;
            AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
                rootSlice, &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
            EXPECT_NE(rootSlice, nullptr);
            rootSlice->RemoveSliceInstance(instance);
        }

        const char* m_entityName = "LayerEntityName";
        SliceToolsTestApplication m_app{ "EditorLayerComponentTest" };
        EntityAndLayerComponent m_layerEntity;
        AZ::ComponentApplication::Descriptor m_descriptor;
        AZ::ComponentDescriptor* m_editorLayerComponentTestHelperDescriptor = nullptr;
    };

    TEST_F(EditorLayerComponentTest, LayerTests_EntityCreatedWithLayer_HasLayerReturnsTrue)
    {
        bool isLayerEntity = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            isLayerEntity,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
        EXPECT_TRUE(isLayerEntity);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_GetLayerColor_ReturnsCorrectColor)
    {
        // Layers serialize color as an AZ::Color because it works with our serialization system,
        // but GetColor returns a QColor because it works with our UI system to render with the color.
        // Alpha is not tested because layers don't use alpha.
        AZ::Color setLayerColor(aznumeric_cast<uint8_t>(255), aznumeric_cast<uint8_t>(128), aznumeric_cast<uint8_t>(64), aznumeric_cast<uint8_t>(255));
        m_layerEntity.m_layer->SetLayerColor(setLayerColor);

        // Set the get color to specifically not be the same as the set, so we know if the ebus
        // was connected and worked.
        QColor getLayerColor(0, 0, 0);
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            getLayerColor,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::GetLayerColor);
        EXPECT_EQ(getLayerColor.red(), setLayerColor.GetR8());
        EXPECT_EQ(getLayerColor.green(), setLayerColor.GetG8());
        EXPECT_EQ(getLayerColor.blue(), setLayerColor.GetB8());
    }

    TEST_F(EditorLayerComponentTest, LayerTests_SingleLayer_LayerNameIsValid)
    {
        bool isLayerNameValid = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            isLayerNameValid,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::IsLayerNameValid);
        EXPECT_TRUE(isLayerNameValid);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_TwoLayersUniqueNames_LayerNameIsValid)
    {
        CreateEntityWithLayer("UniqueLayerName");
        bool isLayerNameValid = true;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            isLayerNameValid,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::IsLayerNameValid);
        EXPECT_TRUE(isLayerNameValid);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_TwoLayersConflictingNames_LayerNameIsNotValid)
    {
        CreateEntityWithLayer(m_entityName);
        bool isLayerNameValid = true;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            isLayerNameValid,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::IsLayerNameValid);
        EXPECT_FALSE(isLayerNameValid);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_TwoLayersConflictingNames_LayerNameConflictingMappingShowsConflict)
    {
        EntityAndLayerComponent secondLayer = CreateEntityWithLayer(m_entityName);
        AZStd::unordered_set<AZ::EntityId> layerEntities;
        layerEntities.insert(m_layerEntity.m_entity->GetId());
        layerEntities.insert(secondLayer.m_entity->GetId());

        AZStd::unordered_map<AZStd::string, int> nameConflictMapping;
        for (AZ::EntityId layerEntityId : layerEntities)
        {
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
                layerEntityId,
                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::UpdateLayerNameConflictMapping,
                nameConflictMapping);
        }
        EXPECT_EQ(nameConflictMapping[m_entityName], 2);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_TwoLayersSameNameDifferentCase_LayerNameConflictingMappingShowsConflict)
    {
        AZStd::string upperCaseName = m_entityName;
        AZStd::to_upper(upperCaseName.begin(), upperCaseName.end());
        // Verify that the casing actually changed. This catches if someone adjusts the
        // default layer name in these layer tests to be all upper case.
        EXPECT_NE(upperCaseName.compare(m_entityName), 0);
        EntityAndLayerComponent secondLayer = CreateEntityWithLayer(upperCaseName.c_str());
        AZStd::unordered_set<AZ::EntityId> layerEntities;
        layerEntities.insert(m_layerEntity.m_entity->GetId());
        layerEntities.insert(secondLayer.m_entity->GetId());

        AZStd::unordered_map<AZStd::string, int> nameConflictMapping;
        for (AZ::EntityId layerEntityId : layerEntities)
        {
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
                layerEntityId,
                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::UpdateLayerNameConflictMapping,
                nameConflictMapping);
        }
        EXPECT_EQ(nameConflictMapping[m_entityName], 2);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_TwoLayersUniqueNames_NameConflictMappingHasNoConflicts)
    {
        EntityAndLayerComponent secondLayer = CreateEntityWithLayer("UniqueLayerName");
        AZStd::unordered_set<AZ::EntityId> layerEntities;
        layerEntities.insert(m_layerEntity.m_entity->GetId());
        layerEntities.insert(secondLayer.m_entity->GetId());

        AZStd::unordered_map<AZStd::string, int> nameConflictMapping;
        for (AZ::EntityId layerEntityId : layerEntities)
        {
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
                layerEntityId,
                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::UpdateLayerNameConflictMapping,
                nameConflictMapping);
        }
        EXPECT_EQ(nameConflictMapping.find(m_entityName), nameConflictMapping.end());
    }

    // The design of this is expected to change, but this unit test validates current behavior.
    // Eventually we would like to decouple the layer name from the entity name.
    TEST_F(EditorLayerComponentTest, LayerTests_LayerWithNoParent_FileNameIsEntityName)
    {
        AZ::Outcome<AZStd::string, AZStd::string> layerFileNameResult(
            AZ::Failure(AZStd::string("No ebus listener available for this layer.")));
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            layerFileNameResult,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::GetLayerBaseFileName);
        EXPECT_TRUE(layerFileNameResult.IsSuccess());
        EXPECT_EQ(layerFileNameResult.GetValue().compare(m_entityName), 0);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_LayerWithParent_FileNameIsParentDotEntityName)
    {
        const AZStd::string parentName = "AParentLayer";
        EntityAndLayerComponent parentLayer = CreateEntityWithLayer(parentName.c_str());
        
        AZ::TransformBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            parentLayer.m_entity->GetId());

        AZ::Outcome<AZStd::string, AZStd::string> layerFileNameResult(
            AZ::Failure(AZStd::string("No ebus listener available for this layer.")));
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            layerFileNameResult,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::GetLayerBaseFileName);
        EXPECT_TRUE(layerFileNameResult.IsSuccess());

        AZStd::string expectedParentName = parentName + "." + m_entityName;

        EXPECT_EQ(layerFileNameResult.GetValue().compare(expectedParentName), 0);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_SetVisibility_GetVisibilityReturnsCorrectValue)
    {
        bool currentVisibility = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            currentVisibility,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::AreLayerChildrenVisible);
        EXPECT_EQ(currentVisibility, true);

        bool flippedVisibility = !currentVisibility;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::SetLayerChildrenVisibility,
            flippedVisibility);

        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            currentVisibility,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::AreLayerChildrenVisible);

        EXPECT_EQ(flippedVisibility, currentVisibility);

        // Change the visibility again to make sure setting it to both values works.
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::SetLayerChildrenVisibility,
            !flippedVisibility);

        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            currentVisibility,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::AreLayerChildrenVisible);

        EXPECT_EQ(!flippedVisibility, currentVisibility);

    }

    TEST_F(EditorLayerComponentTest, LayerTests_NewLayer_HasUnsavedChanges)
    {
        bool hasUnsavedChanges = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            hasUnsavedChanges,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasUnsavedChanges);
        EXPECT_TRUE(hasUnsavedChanges);
    }
    
    TEST_F(EditorLayerComponentTest, LayerTests_SavedLayer_DoesNotHaveUnsavedChanges)
    {
        m_layerEntity.m_layer->ClearUnsavedChanges();
        bool hasUnsavedChanges = true;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            hasUnsavedChanges,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasUnsavedChanges);
        EXPECT_FALSE(hasUnsavedChanges);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_AddedEntityToLayer_HasUnsavedChanges)
    {
        m_layerEntity.m_layer->ClearUnsavedChanges();

        AZ::Entity* childEntity = CreateEditorReadyEntity("ChildEntity");
        // An undo batch needs to begin before the entity can be registered as dirty
        AzToolsFramework::ScopedUndoBatch undoBatch("Reparent Entity");
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        bool hasUnsavedChanges = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            hasUnsavedChanges,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasUnsavedChanges);
        EXPECT_TRUE(hasUnsavedChanges);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_ModifiedEntityInLayer_HasUnsavedChanges)
    {
        AZ::Entity* childEntity = CreateEditorReadyEntity("ChildEntity");
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());
        
        m_layerEntity.m_layer->ClearUnsavedChanges();

        // Change the scale of the child entity so it registers as an unsaved change on the layer.
        // An undo batch needs to begin before the entity can be registered as dirty
        AzToolsFramework::ScopedUndoBatch undoBatch("Scale Entity");
        float scale = 0.0f;
        AZ::TransformBus::EventResult(
            scale,
            childEntity->GetId(),
            &AZ::TransformBus::Events::GetLocalUniformScale);
        scale += 1.0f;
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetLocalUniformScale,
            scale);

        bool hasUnsavedChanges = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            hasUnsavedChanges,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasUnsavedChanges);
        EXPECT_TRUE(hasUnsavedChanges);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_CanParentChangeToNonLayer_ReturnsFalse)
    {
        AZ::Entity* childEntity = CreateEditorReadyEntity("NonLayerEntity");
        bool canParentChange = true;
        AZ::TransformNotificationBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AZ::TransformNotificationBus::Events::CanParentChange,
            canParentChange,
            AZ::EntityId(),
            childEntity->GetId());
        EXPECT_FALSE(canParentChange);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_CanParentChangeToLayer_ReturnsTrue)
    {
        EntityAndLayerComponent secondLayer = CreateEntityWithLayer("UniqueLayerName");
        // Most tests around ebuses, the value is initialized to the opposite of the expected result.
        // This particular message is an aggregate message that goes out to multiple listeners,
        // and any of them can decline the parent change. This means that the initial
        // value needs to start true.
        bool canParentChange = true;
        AZ::TransformNotificationBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AZ::TransformNotificationBus::Events::CanParentChange,
            canParentChange,
            AZ::EntityId(),
            secondLayer.m_entity->GetId());
        EXPECT_TRUE(canParentChange);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_CanParentChangeFromLayerToInvalid_ReturnsTrue)
    {
        EntityAndLayerComponent secondLayer = CreateEntityWithLayer("UniqueLayerName");
        AZ::TransformBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            secondLayer.m_entity->GetId());

        bool canParentChange = true;
        AZ::TransformNotificationBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AZ::TransformNotificationBus::Events::CanParentChange,
            canParentChange,
            secondLayer.m_entity->GetId(),
            AZ::EntityId());
        EXPECT_TRUE(canParentChange);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_AttemptToSetParentToNonLayer_ParentDoesNotChange)
    {
        AZ::Entity* nonLayerEntity = CreateEditorReadyEntity("NonLayerEntity");
        AZ::TransformBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            nonLayerEntity->GetId());

        AZ::EntityId parentId = nonLayerEntity->GetId();
        AZ::TransformBus::EventResult(
            parentId,
            m_layerEntity.m_entity->GetId(),
            &AZ::TransformBus::Events::GetParentId);
        EXPECT_EQ(parentId, AZ::EntityId());
    }

    TEST_F(EditorLayerComponentTest, LayerTests_VisibleLayerVisibleChild_BothVisible)
    {
        AZ::Entity* childEntity = CreateEditorReadyEntity("NonLayerEntity");
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        bool layerChildrenVisibility = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            layerChildrenVisibility,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::AreLayerChildrenVisible);

        bool isChildVisible = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isChildVisible,
            childEntity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);

        bool isChildVisibilityFlagSet = false;
        AzToolsFramework::EditorVisibilityRequestBus::EventResult(
            isChildVisibilityFlagSet,
            childEntity->GetId(),
            &AzToolsFramework::EditorVisibilityRequestBus::Events::GetVisibilityFlag);

        EXPECT_TRUE(layerChildrenVisibility);
        EXPECT_TRUE(isChildVisible);
        EXPECT_TRUE(isChildVisibilityFlagSet);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_InvisibleLayerVisibleChild_BothInvisibleChildPreservesVisibility)
    {
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::SetLayerChildrenVisibility,
            false);

        // This is necessary to prevent a warning in the undo system.
        // This unit test had modified an entity to make it dirty, so it needs to be marked as such.
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity,
            m_layerEntity.m_entity->GetId());

        AZ::Entity* childEntity = CreateEditorReadyEntity("NonLayerEntity");
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        bool layerChildrenVisibility = true;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            layerChildrenVisibility,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::AreLayerChildrenVisible);

        bool isChildVisible = true;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isChildVisible,
            childEntity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);

        bool isChildVisibilityFlagSet = false;
        AzToolsFramework::EditorVisibilityRequestBus::EventResult(
            isChildVisibilityFlagSet,
            childEntity->GetId(),
            &AzToolsFramework::EditorVisibilityRequestBus::Events::GetVisibilityFlag);

        EXPECT_FALSE(layerChildrenVisibility);
        EXPECT_FALSE(isChildVisible);
        EXPECT_TRUE(isChildVisibilityFlagSet);
    }

    // Layer visibility toggle only works one way: Invisible layers make all children invisible, but
    // visible layers do not force children to be visible.
    TEST_F(EditorLayerComponentTest, LayerTests_VisibleLayerInvisibleChild_ChildIsInvisible)
    {
        AZ::Entity* childEntity = CreateEditorReadyEntity("NonLayerEntity");
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        AzToolsFramework::EditorVisibilityRequestBus::Event(
            childEntity->GetId(),
            &AzToolsFramework::EditorVisibilityRequestBus::Events::SetVisibilityFlag,
            false);

        bool layerChildrenVisibility = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            layerChildrenVisibility,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::AreLayerChildrenVisible);

        bool isChildVisible = true;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isChildVisible,
            childEntity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);

        bool isChildVisibilityFlagSet = true;
        AzToolsFramework::EditorVisibilityRequestBus::EventResult(
            isChildVisibilityFlagSet,
            childEntity->GetId(),
            &AzToolsFramework::EditorVisibilityRequestBus::Events::GetVisibilityFlag);

        EXPECT_TRUE(layerChildrenVisibility);
        EXPECT_FALSE(isChildVisible);
        EXPECT_FALSE(isChildVisibilityFlagSet);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_InvisibleLayerInvisibleChild_BothInvisibleChildPreservesVisibility)
    {
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::SetLayerChildrenVisibility,
            false);
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity,
            m_layerEntity.m_entity->GetId());

        AZ::Entity* childEntity = CreateEditorReadyEntity("NonLayerEntity");
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        AzToolsFramework::EditorVisibilityRequestBus::Event(
            childEntity->GetId(),
            &AzToolsFramework::EditorVisibilityRequestBus::Events::SetVisibilityFlag,
            false);

        bool layerChildrenVisibility = true;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            layerChildrenVisibility,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::AreLayerChildrenVisible);

        bool isChildVisible = true;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isChildVisible,
            childEntity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);

        bool isChildVisibilityFlagSet = true;
        AzToolsFramework::EditorVisibilityRequestBus::EventResult(
            isChildVisibilityFlagSet,
            childEntity->GetId(),
            &AzToolsFramework::EditorVisibilityRequestBus::Events::GetVisibilityFlag);

        EXPECT_FALSE(layerChildrenVisibility);
        EXPECT_FALSE(isChildVisible);
        EXPECT_FALSE(isChildVisibilityFlagSet);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_InvisLayerVisLayerVisChild_AllInvisiblePreservingVisibility)
    {
        const AZStd::string parentName = "AParentLayer";
        EntityAndLayerComponent parentLayer = CreateEntityWithLayer(parentName.c_str());

        AZ::TransformBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            parentLayer.m_entity->GetId());

        AZ::Entity* childEntity = CreateEditorReadyEntity("NonLayerEntity");
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            parentLayer.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::SetLayerChildrenVisibility,
            false);
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity,
            parentLayer.m_entity->GetId());

        bool parentLayerChildrenVisible = true;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            parentLayerChildrenVisible,
            parentLayer.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::AreLayerChildrenVisible);

        bool layerChildrenVisibility = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            layerChildrenVisibility,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::AreLayerChildrenVisible);

        bool isChildVisible = true;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isChildVisible,
            childEntity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);

        bool isChildVisibilityFlagSet = false;
        AzToolsFramework::EditorVisibilityRequestBus::EventResult(
            isChildVisibilityFlagSet,
            childEntity->GetId(),
            &AzToolsFramework::EditorVisibilityRequestBus::Events::GetVisibilityFlag);

        EXPECT_FALSE(parentLayerChildrenVisible);
        EXPECT_TRUE(layerChildrenVisibility);
        EXPECT_FALSE(isChildVisible);
        EXPECT_TRUE(isChildVisibilityFlagSet);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_UnlockedLayerUnlockedChild_BothUnlocked)
    {
        AZ::Entity* childEntity = CreateEditorReadyEntity("NonLayerEntity");
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        bool isLayerLocked = true;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isLayerLocked,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsJustThisEntityLocked);
        bool isChildLockFlagSet = true;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isChildLockFlagSet,
            childEntity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsJustThisEntityLocked);
        bool isChildLockedViaHierarchy = true;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isChildLockedViaHierarchy,
            childEntity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsLocked);

        EXPECT_FALSE(isLayerLocked);
        EXPECT_FALSE(isChildLockFlagSet);
        EXPECT_FALSE(isChildLockedViaHierarchy);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_LockedLayerUnlockedChild_ChildLockedAndPreservesLockState)
    {
        AZ::Entity* childEntity = CreateEditorReadyEntity("NonLayerEntity");
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        AzToolsFramework::EditorLockComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::EditorLockComponentRequests::SetLocked,
            true);

        bool isLayerLocked = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isLayerLocked,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsJustThisEntityLocked);
        bool isChildLockFlagSet = true;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isChildLockFlagSet,
            childEntity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsJustThisEntityLocked);
        bool isChildLockedViaHierarchy = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isChildLockedViaHierarchy,
            childEntity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsLocked);

        EXPECT_TRUE(isLayerLocked);
        EXPECT_FALSE(isChildLockFlagSet);
        EXPECT_TRUE(isChildLockedViaHierarchy);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_LockedLayerLockedChild_AllLocked)
    {
        AZ::Entity* childEntity = CreateEditorReadyEntity("NonLayerEntity");
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        AzToolsFramework::EditorLockComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::EditorLockComponentRequests::SetLocked,
            true);

        AzToolsFramework::EditorLockComponentRequestBus::Event(
            childEntity->GetId(),
            &AzToolsFramework::EditorLockComponentRequests::SetLocked,
            true);

        bool isLayerLocked = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isLayerLocked,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsJustThisEntityLocked);
        bool isChildLockFlagSet = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isChildLockFlagSet,
            childEntity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsJustThisEntityLocked);
        bool isChildLockedViaHierarchy = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isChildLockedViaHierarchy,
            childEntity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsLocked);

        EXPECT_TRUE(isLayerLocked);
        EXPECT_TRUE(isChildLockFlagSet);
        EXPECT_TRUE(isChildLockedViaHierarchy);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_LockLayerUnlockLayerUnlockChild_AllLockedPreservingVisibility)
    {
        const AZStd::string parentName = "AParentLayer";
        EntityAndLayerComponent parentLayer = CreateEntityWithLayer(parentName.c_str());

        AZ::TransformBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            parentLayer.m_entity->GetId());

        AZ::Entity* childEntity = CreateEditorReadyEntity("NonLayerEntity");
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        AzToolsFramework::EditorLockComponentRequestBus::Event(
            parentLayer.m_entity->GetId(),
            &AzToolsFramework::EditorLockComponentRequests::SetLocked,
            true);

        bool isParentLocked = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isParentLocked,
            parentLayer.m_entity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsJustThisEntityLocked);

        bool isLayerLockFlagSet = true;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isLayerLockFlagSet,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsJustThisEntityLocked);
        bool isLayerLockedViaHierarchy = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isLayerLockedViaHierarchy,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsLocked);
        bool isChildLockFlagSet = true;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isChildLockFlagSet,
            childEntity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsJustThisEntityLocked);
        bool isChildLockedViaHierarchy = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            isChildLockedViaHierarchy,
            childEntity->GetId(),
            &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsLocked);

        EXPECT_TRUE(isParentLocked);
        EXPECT_TRUE(isLayerLockedViaHierarchy);
        EXPECT_FALSE(isLayerLockFlagSet);
        EXPECT_TRUE(isLayerLockedViaHierarchy);
        EXPECT_FALSE(isChildLockFlagSet);
        EXPECT_TRUE(isChildLockedViaHierarchy);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_SaveEmptyLayer_SavesWithoutError)
    {
        // SaveMainLayer is not called here because this test is testing behavior
        // that SaveMainLayer assumes will work.
        AZStd::vector<AZ::Entity*> layerEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs instancesInLayers;

        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        Layers::EditorLayer layer;
        Layers::LayerResult layerResult = m_layerEntity.m_layer->PopulateLayerWriteToStreamAndGetEntities(
            layerEntities,
            instancesInLayers,
            entitySaveStream,
            layer);
        EXPECT_TRUE(layerResult.IsSuccess());
        EXPECT_EQ(layer.m_layerEntities.size(), 0);
        EXPECT_EQ(layer.m_sliceAssetsToSliceInstances.size(), 0);
    }

    // To minimize the need to modify the level file, the color should save to the layer
    // and not the level. This is tested by verifying the color is set to zero on the
    // layer component after the save, and the color is set correctly in the EditorLayer object.
    TEST_F(EditorLayerComponentTest, LayerTests_SaveColorModified_ColorSavedCorrectly)
    {
        AZ::Color setLayerColor(aznumeric_cast<uint8_t>(64), aznumeric_cast<uint8_t>(255), aznumeric_cast<uint8_t>(128), aznumeric_cast<uint8_t>(255));
        m_layerEntity.m_layer->SetLayerColor(setLayerColor);

        Layers::EditorLayer layer;
        Layers::LayerResult saveResult = SaveMainLayer(layer);
        EXPECT_TRUE(saveResult.IsSuccess());

        QColor getLayerColor(2, 4, 8);
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            getLayerColor,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::GetLayerColor);
        EXPECT_EQ(getLayerColor.red(), 255);
        EXPECT_EQ(getLayerColor.green(), 255);
        EXPECT_EQ(getLayerColor.blue(), 255);

        EXPECT_EQ(layer.m_layerProperties.m_color.GetR8(), setLayerColor.GetR8());
        EXPECT_EQ(layer.m_layerProperties.m_color.GetG8(), setLayerColor.GetG8());
        EXPECT_EQ(layer.m_layerProperties.m_color.GetB8(), setLayerColor.GetB8());
    }

    TEST_F(EditorLayerComponentTest, LayerTests_ColorModifiedRestoreLayerAfterSave_ColorRestoresCorrectly)
    {
        AZ::Color setLayerColor(aznumeric_cast<uint8_t>(10), aznumeric_cast<uint8_t>(30), aznumeric_cast<uint8_t>(20), aznumeric_cast<uint8_t>(255));
        m_layerEntity.m_layer->SetLayerColor(setLayerColor);

        Layers::EditorLayer layer;
        Layers::LayerResult saveResult = SaveMainLayer(layer);
        EXPECT_TRUE(saveResult.IsSuccess());

        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::RestoreEditorData);

        QColor getLayerColor(2, 4, 8);
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            getLayerColor,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::GetLayerColor);
        EXPECT_EQ(getLayerColor.red(), setLayerColor.GetR8());
        EXPECT_EQ(getLayerColor.green(), setLayerColor.GetG8());
        EXPECT_EQ(getLayerColor.blue(), setLayerColor.GetB8());
    }

    TEST_F(EditorLayerComponentTest, LayerTests_SaveFormatModifiedRestoreLayerAfterSave_SaveFormatRestoresCorrectly)
    {
        // Set the save format to binary, as the default is xml.
        AzToolsFramework::Layers::LayerProperties::SaveFormat setSaveFormat = AzToolsFramework::Layers::LayerProperties::SaveFormat::Binary;
        m_layerEntity.m_layer->SetSaveFormat(setSaveFormat);

        Layers::EditorLayer layer;
        Layers::LayerResult saveResult = SaveMainLayer(layer);
        EXPECT_TRUE(saveResult.IsSuccess());

        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::RestoreEditorData);

        // Check the save format is still binary after a restoring the layer.
        AzToolsFramework::Layers::LayerProperties::SaveFormat getSaveFormat = AzToolsFramework::Layers::LayerProperties::SaveFormat::Xml;
        bool saveFormatIsBinary = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            saveFormatIsBinary,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::IsSaveFormatBinary);

        if (saveFormatIsBinary)
        {
            getSaveFormat = AzToolsFramework::Layers::LayerProperties::SaveFormat::Binary;
        }
        EXPECT_EQ(getSaveFormat, setSaveFormat);


        // Now change the save format back to xml and redo the test.
        setSaveFormat = AzToolsFramework::Layers::LayerProperties::SaveFormat::Xml;
        m_layerEntity.m_layer->SetSaveFormat(setSaveFormat);

        saveResult = SaveMainLayer(layer);
        EXPECT_TRUE(saveResult.IsSuccess());

        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::RestoreEditorData);

        getSaveFormat = AzToolsFramework::Layers::LayerProperties::SaveFormat::Xml;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            saveFormatIsBinary,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::IsSaveFormatBinary);

        if (saveFormatIsBinary)
        {
            getSaveFormat = AzToolsFramework::Layers::LayerProperties::SaveFormat::Binary;
        }
        EXPECT_EQ(getSaveFormat, setSaveFormat);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_SaveAndLoadColorModified_ColorLoadsCorrectly)
    {
        AZ::Color savedLayerColor(aznumeric_cast<uint8_t>(6), aznumeric_cast<uint8_t>(7), aznumeric_cast<uint8_t>(8), aznumeric_cast<uint8_t>(255));
        m_layerEntity.m_layer->SetLayerColor(savedLayerColor);

        Layers::EditorLayer layer;
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        Layers::LayerResult saveResult = SaveMainLayer(layer, entitySaveStream);
        EXPECT_TRUE(saveResult.IsSuccess());

        // After saving the layer, restore the editor data and set the color to something else.
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::RestoreEditorData);
        AZ::Color unsavedLayerColor(aznumeric_cast<uint8_t>(20), aznumeric_cast<uint8_t>(30), aznumeric_cast<uint8_t>(40), aznumeric_cast<uint8_t>(255));
        m_layerEntity.m_layer->SetLayerColor(unsavedLayerColor);
        
        AZStd::unordered_map<AZ::EntityId, AZ::Entity*> uniqueEntities;
        AZ::SliceComponent::SliceAssetToSliceInstancePtrs sliceInstances;
        Layers::LayerResult readResult = m_layerEntity.m_layer->ReadFromLayerStream(entitySaveStream, sliceInstances, uniqueEntities);
        EXPECT_TRUE(readResult.IsSuccess());

        QColor getLayerColor(2, 4, 8);
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            getLayerColor,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::GetLayerColor);
        EXPECT_FLOAT_EQ(aznumeric_cast<float>(getLayerColor.redF()), savedLayerColor.GetR());
        EXPECT_FLOAT_EQ(aznumeric_cast<float>(getLayerColor.greenF()), savedLayerColor.GetG());
        EXPECT_FLOAT_EQ(aznumeric_cast<float>(getLayerColor.blueF()), savedLayerColor.GetB());
    }

    TEST_F(EditorLayerComponentTest, LayerTests_SaveLayerVisibilityModified_VisibilitySavesCorrectly)
    {
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::SetLayerChildrenVisibility,
            false);

        Layers::EditorLayer layer;
        Layers::LayerResult saveResult = SaveMainLayer(layer);
        EXPECT_TRUE(saveResult.IsSuccess());

        bool visibilityAfterSave = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            visibilityAfterSave,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::AreLayerChildrenVisible);

        EXPECT_TRUE(visibilityAfterSave);
        EXPECT_FALSE(layer.m_layerProperties.m_isLayerVisible);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_RestoreLayerAfterSave_VisibilityRestoresCorrectly)
    {
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::SetLayerChildrenVisibility,
            false);

        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity,
            m_layerEntity.m_entity->GetId());

        Layers::EditorLayer layer;
        Layers::LayerResult saveResult = SaveMainLayer(layer);
        EXPECT_TRUE(saveResult.IsSuccess());

        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::RestoreEditorData);

        bool visibilityAfterSave = true;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            visibilityAfterSave,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::AreLayerChildrenVisible);

        EXPECT_FALSE(visibilityAfterSave);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_SaveAndLoadVisibilityModified_VisibilityLoadsCorrectly)
    {
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::SetLayerChildrenVisibility,
            false);

        Layers::EditorLayer layer;
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        Layers::LayerResult saveResult = SaveMainLayer(layer, entitySaveStream);
        EXPECT_TRUE(saveResult.IsSuccess());

        // After saving the layer, restore the editor data and set the visibility to true, the value that wasn't saved.
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::RestoreEditorData);

        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::SetLayerChildrenVisibility,
            true);

        AZStd::unordered_map<AZ::EntityId, AZ::Entity*> uniqueEntities;
        AZ::SliceComponent::SliceAssetToSliceInstancePtrs sliceInstances;
        Layers::LayerResult readResult = m_layerEntity.m_layer->ReadFromLayerStream(entitySaveStream, sliceInstances, uniqueEntities);
        EXPECT_TRUE(readResult.IsSuccess());

        bool visibilityAfterLoad = true;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            visibilityAfterLoad,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::AreLayerChildrenVisible);
        EXPECT_FALSE(visibilityAfterLoad);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_SaveLayerBinaryModified_BinarySavesCorrectly)
    {
        m_layerEntity.m_layer->SetSaveAsBinary(true);

        Layers::EditorLayer layer;
        Layers::LayerResult saveResult = SaveMainLayer(layer);
        EXPECT_TRUE(saveResult.IsSuccess());

        EXPECT_FALSE(m_layerEntity.m_layer->GetSaveAsBinary());
        EXPECT_TRUE(layer.m_layerProperties.m_saveAsBinary);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_RestoreLayerAfterSave_BinaryRestoresCorrectly)
    {
        m_layerEntity.m_layer->SetSaveAsBinary(true);

        Layers::EditorLayer layer;
        Layers::LayerResult saveResult = SaveMainLayer(layer);
        EXPECT_TRUE(saveResult.IsSuccess());

        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::RestoreEditorData);

        EXPECT_TRUE(m_layerEntity.m_layer->GetSaveAsBinary());
    }
    
    TEST_F(EditorLayerComponentTest, LayerTests_SaveAndLoadBinarySaveTypeModified_BinarySaveTypeLoadsCorrectly)
    {
        m_layerEntity.m_layer->SetSaveAsBinary(true);

        Layers::EditorLayer layer;
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        Layers::LayerResult saveResult = SaveMainLayer(layer, entitySaveStream);
        EXPECT_TRUE(saveResult.IsSuccess());

        // After saving the layer, restore the editor data and set the save as binary to false, the value that wasn't saved.
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::RestoreEditorData);

        m_layerEntity.m_layer->SetSaveAsBinary(false);

        AZStd::unordered_map<AZ::EntityId, AZ::Entity*> uniqueEntities;
        AZ::SliceComponent::SliceAssetToSliceInstancePtrs sliceInstances;
        Layers::LayerResult readResult = m_layerEntity.m_layer->ReadFromLayerStream(entitySaveStream, sliceInstances, uniqueEntities);
        EXPECT_TRUE(readResult.IsSuccess());

        EXPECT_TRUE(m_layerEntity.m_layer->GetSaveAsBinary());
    }

    TEST_F(EditorLayerComponentTest, LayerTests_SaveAndLoadEntityInLayer_EntityLoadsCorrectly)
    {
        AZ::Entity* childEntity = CreateEditorReadyEntity("ChildEntity");
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        Layers::EditorLayer layer;
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        AZStd::vector<AZ::Entity*> savedEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs savedInstances;
        Layers::LayerResult saveResult = SaveMainLayer(layer, entitySaveStream, savedEntities, savedInstances);
        EXPECT_TRUE(saveResult.IsSuccess());

        EXPECT_EQ(savedEntities.size(), 1);
        EXPECT_NE(savedEntities[0], nullptr);
        EXPECT_EQ(savedEntities[0]->GetId(), childEntity->GetId());

        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::RestoreEditorData);

        AZStd::unordered_map<AZ::EntityId, AZ::Entity*> uniqueEntities;
        AZ::SliceComponent::SliceAssetToSliceInstancePtrs loadedInstances;
        Layers::LayerResult readResult = m_layerEntity.m_layer->ReadFromLayerStream(entitySaveStream, loadedInstances, uniqueEntities);
        EXPECT_TRUE(readResult.IsSuccess());

        EXPECT_EQ(uniqueEntities.size(), 1);
        EXPECT_NE(uniqueEntities.begin(), uniqueEntities.end());
        EXPECT_EQ(uniqueEntities.begin()->first, childEntity->GetId());
        EXPECT_NE(uniqueEntities.begin()->second, nullptr);
        EXPECT_EQ(uniqueEntities.begin()->second->GetId(), childEntity->GetId());
    }

    TEST_F(EditorLayerComponentTest, LayerTests_RootSliceEntityEraseRestore_EntitiesRemovedAndRestoredCorrectly)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        AZ::Entity* childEntity = CreateEditorReadyEntity("ChildEntity");
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        Layers::EditorLayer layer;
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        AZStd::vector<AZ::Entity*> savedEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs savedInstances;
        Layers::LayerResult saveResult = SaveMainLayer(layer, entitySaveStream, savedEntities, savedInstances);
        EXPECT_TRUE(saveResult.IsSuccess());

        EXPECT_EQ(savedEntities.size(), 1);
        EXPECT_NE(savedEntities[0], nullptr);
        EXPECT_EQ(savedEntities[0]->GetId(), childEntity->GetId());

        AZ::SliceComponent* rootSlice;
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
            rootSlice, &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
        EXPECT_NE(rootSlice, nullptr);

        // First, verify that childEntity is in the loose entity list.
        // Size of 2 is the layer and the child entity.
        EXPECT_TRUE(IsEntityInLooseEditorEntities(childEntity));

        // After erasing the saved entities, verify childEntity is no longer a loose entity.
        rootSlice->EraseEntities(savedEntities);
        EXPECT_FALSE(IsEntityInLooseEditorEntities(childEntity));

        rootSlice->ReplaceEntities(savedEntities);
        EXPECT_TRUE(IsEntityInLooseEditorEntities(childEntity));
    }

    TEST_F(EditorLayerComponentTest, LayerTests_NestedLayersDoNotSaveLayersInLayers_LayersLoadCorrectly)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        const AZStd::string childLayerName = "ChildLayerName";
        EntityAndLayerComponent childLayer = CreateEntityWithLayer(childLayerName.c_str());

        AZ::TransformBus::Event(
            childLayer.m_entity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        Layers::EditorLayer layer;
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        AZStd::vector<AZ::Entity*> savedEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs savedInstances;
        Layers::LayerResult saveResult = SaveMainLayer(layer, entitySaveStream, savedEntities, savedInstances);
        EXPECT_TRUE(saveResult.IsSuccess());

        EXPECT_EQ(savedEntities.size(), 0);

        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::RestoreEditorData);

        AZStd::unordered_map<AZ::EntityId, AZ::Entity*> uniqueEntities;
        AZ::SliceComponent::SliceAssetToSliceInstancePtrs loadedInstances;
        Layers::LayerResult readResult = m_layerEntity.m_layer->ReadFromLayerStream(entitySaveStream, loadedInstances, uniqueEntities);
        EXPECT_TRUE(readResult.IsSuccess());

        EXPECT_EQ(uniqueEntities.size(), 0);
    }

    // With a hierarchy of: LayerEntityName -> ChildLayer -> ChildEntity, when the parent and child layer are saved,
    // then loaded, the ChildEntity should be saved in the ChildLayer and not the LayerEntityName, and the ChildLayer should not
    // be saved to the LayerEntityName.
    TEST_F(EditorLayerComponentTest, LayerTests_SaveAndLoadLayerLayerEntityHierarchy_LayersAndEntityLoadCorrectly)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        const AZStd::string childLayerName = "ChildLayer";
        EntityAndLayerComponent childLayerEntityAndComponent = CreateEntityWithLayer(childLayerName.c_str());
        AZ::EntityId childLayerEntityId = childLayerEntityAndComponent.m_entity->GetId();

        AZ::TransformBus::Event(
            childLayerEntityId,
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        AZ::Entity* childEntity = CreateEditorReadyEntity("ChildEntity");
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            childLayerEntityId);

        Layers::EditorLayer layer;
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        AZStd::vector<AZ::Entity*> savedEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs savedInstances;
        Layers::LayerResult saveResult = SaveMainLayer(layer, entitySaveStream, savedEntities, savedInstances);
        EXPECT_TRUE(saveResult.IsSuccess());
        EXPECT_EQ(savedEntities.size(), 0);

        Layers::EditorLayer childLayer;
        AZStd::vector<char> childLayerEntitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > childLayerEntitySaveStream(&childLayerEntitySaveBuffer);
        AZStd::vector<AZ::Entity*> childLayerSavedEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs childLayerSavedInstances;
        Layers::LayerResult childSaveResult = childLayerEntityAndComponent.m_layer->PopulateLayerWriteToStreamAndGetEntities(
            childLayerSavedEntities,
            childLayerSavedInstances,
            childLayerEntitySaveStream,
            childLayer);
        EXPECT_TRUE(childSaveResult.IsSuccess());
        EXPECT_EQ(childLayerSavedEntities.size(), 1);
        EXPECT_NE(childLayerSavedEntities[0], nullptr);
        EXPECT_EQ(childLayerSavedEntities[0]->GetId(), childEntity->GetId());

        // Restore the cached editor data for both the parent and child layers.
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::RestoreEditorData);
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            childLayerEntityId,
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::RestoreEditorData);

        // Verify that the outermost layer loads correctly and has no entities in it.
        AZStd::unordered_map<AZ::EntityId, AZ::Entity*> uniqueEntities;
        AZ::SliceComponent::SliceAssetToSliceInstancePtrs loadedInstances;
        Layers::LayerResult readResult = m_layerEntity.m_layer->ReadFromLayerStream(entitySaveStream, loadedInstances, uniqueEntities);
        EXPECT_TRUE(readResult.IsSuccess());
        EXPECT_EQ(uniqueEntities.size(), 0);

        // Verify the inner layer loads correctly and has the entity in it we are looking for.
        AZStd::unordered_map<AZ::EntityId, AZ::Entity*> uniqueChildEntities;
        AZ::SliceComponent::SliceAssetToSliceInstancePtrs loadedChildInstances;
        Layers::LayerResult childReadResult =
            childLayerEntityAndComponent.m_layer->ReadFromLayerStream(childLayerEntitySaveStream, loadedChildInstances, uniqueChildEntities);
        EXPECT_TRUE(childReadResult.IsSuccess());

        EXPECT_EQ(uniqueChildEntities.size(), 1);
        EXPECT_NE(uniqueChildEntities.begin(), uniqueChildEntities.end());
        EXPECT_EQ(uniqueChildEntities.begin()->first, childEntity->GetId());
        EXPECT_NE(uniqueChildEntities.begin()->second, nullptr);
        EXPECT_EQ(uniqueChildEntities.begin()->second->GetId(), childEntity->GetId());

        childLayerEntityAndComponent.m_layer->CleanupLoadedLayer();
    }

    TEST_F(EditorLayerComponentTest, LayerTests_SliceInstanceAddedToLayer_LayerHasUnsavedChanges)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        AZ::SliceComponent::SliceInstanceAddress instantiatedSlice = CreateSliceInstance();
        AZ::Entity* childEntity = GetEntityFromSliceInstance(instantiatedSlice);
        m_layerEntity.m_layer->ClearUnsavedChanges();

        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        bool hasUnsavedChanges = false;
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
            hasUnsavedChanges,
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasUnsavedChanges);
        EXPECT_TRUE(hasUnsavedChanges);

        DeleteSliceInstance(instantiatedSlice);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_SaveAndLoadInstanceInLayer_InstanceLoadsCorrectly)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        AZ::SliceComponent::SliceInstanceAddress instantiatedSlice = CreateSliceInstance();
        AZ::Entity* childEntity = GetEntityFromSliceInstance(instantiatedSlice);
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        Layers::EditorLayer layer;
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        AZStd::vector<AZ::Entity*> savedEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs savedInstances;
        Layers::LayerResult saveResult = SaveMainLayer(layer, entitySaveStream, savedEntities, savedInstances);
        EXPECT_TRUE(saveResult.IsSuccess());

        EXPECT_EQ(savedEntities.size(), 0);
        EXPECT_EQ(savedInstances.size(), 1);
        AZ::SliceComponent::SliceReferenceToInstancePtrs::iterator refToInstanceIterator =
            savedInstances.find(instantiatedSlice.GetReference());
        EXPECT_NE(refToInstanceIterator, savedInstances.end());
        EXPECT_NE(refToInstanceIterator->second.find(instantiatedSlice.GetInstance()), refToInstanceIterator->second.end());

        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::RestoreEditorData);

        AZStd::unordered_map<AZ::EntityId, AZ::Entity*> uniqueEntities;
        AZ::SliceComponent::SliceAssetToSliceInstancePtrs loadedInstances;
        Layers::LayerResult readResult = m_layerEntity.m_layer->ReadFromLayerStream(entitySaveStream, loadedInstances, uniqueEntities);
        EXPECT_TRUE(readResult.IsSuccess());

        EXPECT_EQ(uniqueEntities.size(), 0);
        EXPECT_EQ(loadedInstances.size(), 1);
        EXPECT_NE(loadedInstances.find(instantiatedSlice.GetReference()->GetSliceAsset()), loadedInstances.end());

        DeleteSliceInstance(instantiatedSlice);
    }

    // Temporarily disabled until fix provided in mainline - instantiatedSlice is no longer valid to access after RemoveAndCacheInstances
    /*
    TEST_F(EditorLayerComponentTest, LayerTests_SliceInstanceOnlyInLayerRootSliceRemoveRestore_InstanceRemovedAndRestoredCorrectly)
    {
        AZ::SliceComponent::SliceInstanceAddress instantiatedSlice = CreateSliceInstance();
        AZ::Entity* childEntity = GetEntityFromSliceInstance(instantiatedSlice);
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        Layers::EditorLayer layer;
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        AZStd::vector<AZ::Entity*> savedEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs savedInstances;
        Layers::LayerResult saveResult = SaveMainLayer(layer, entitySaveStream, savedEntities, savedInstances);
        EXPECT_TRUE(saveResult.IsSuccess());

        EXPECT_EQ(savedEntities.size(), 0);
        EXPECT_EQ(savedInstances.size(), 1);
        AZ::SliceComponent::SliceReferenceToInstancePtrs::iterator refToInstanceIterator =
            savedInstances.find(instantiatedSlice.GetReference());
        EXPECT_NE(refToInstanceIterator, savedInstances.end());
        EXPECT_NE(refToInstanceIterator->second.find(instantiatedSlice.GetInstance()), refToInstanceIterator->second.end());

        AZ::SliceComponent* rootSlice;
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
                rootSlice, &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
        EXPECT_NE(rootSlice, nullptr);
        
        // First, verify the the instance is in the root slice.
        EXPECT_TRUE(IsInstanceAndReferenceInRootSlice(rootSlice, instantiatedSlice));

        // After removing the instances, verify the instance and reference is no longer in the root slice.
        rootSlice->RemoveAndCacheInstances(savedInstances);
        EXPECT_FALSE(IsInstanceAndReferenceInRootSlice(rootSlice, instantiatedSlice));

        rootSlice->RestoreCachedInstances();
        EXPECT_TRUE(IsInstanceAndReferenceInRootSlice(rootSlice, instantiatedSlice));

        DeleteSliceInstance(instantiatedSlice);
    }

    // Tests that the slice reference remains in the root scene if there is an instance of the slice in the layer and in the root scene.
    TEST_F(EditorLayerComponentTest, LayerTests_MultipleSliceInstanceInLayerRootSliceRemoveRestore_InstanceRemovedAndRestoredCorrectly)
    {
        AZ::SliceComponent::SliceInstanceAddress instantiatedSlice = CreateSliceInstance();
        // Put the first instance in the layer
        AZ::Entity* childEntity = GetEntityFromSliceInstance(instantiatedSlice);
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        AZ::SliceComponent* rootSlice;
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
                rootSlice, &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
        EXPECT_NE(rootSlice, nullptr);

        // Create a second slice instance that is not in the layer.
        AZ::SliceComponent::SliceInstanceAddress secondSliceInstance = CreateSliceInstanceFromSlice(
            rootSlice,
            instantiatedSlice.GetReference()->GetSliceAsset());

        Layers::EditorLayer layer;
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        AZStd::vector<AZ::Entity*> savedEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs savedInstances;
        Layers::LayerResult saveResult = SaveMainLayer(layer, entitySaveStream, savedEntities, savedInstances);
        EXPECT_TRUE(saveResult.IsSuccess());

        EXPECT_EQ(savedEntities.size(), 0);
        EXPECT_EQ(savedInstances.size(), 1);
        AZ::SliceComponent::SliceReferenceToInstancePtrs::iterator refToInstanceIterator =
            savedInstances.find(instantiatedSlice.GetReference());
        EXPECT_NE(refToInstanceIterator, savedInstances.end());
        EXPECT_NE(refToInstanceIterator->second.find(instantiatedSlice.GetInstance()), refToInstanceIterator->second.end());
        // Make sure the second slice instance is not saved.
        EXPECT_EQ(refToInstanceIterator->second.find(secondSliceInstance.GetInstance()), refToInstanceIterator->second.end());

        // First, verify the the instances are in the root slice.
        EXPECT_TRUE(IsInstanceAndReferenceInRootSlice(rootSlice, instantiatedSlice));
        EXPECT_TRUE(IsInstanceAndReferenceInRootSlice(rootSlice, secondSliceInstance));

        // After removing the instances, verify the first instance is no longer in the root slice, but the reference and second instance are.
        rootSlice->RemoveAndCacheInstances(savedInstances);
        EXPECT_FALSE(IsInstanceAndReferenceInRootSlice(rootSlice, instantiatedSlice));

        AZ::SliceComponent::SliceReference* sliceReference = rootSlice->GetSlice(instantiatedSlice.GetReference()->GetSliceAsset());
        EXPECT_NE(sliceReference, nullptr);
        EXPECT_TRUE(IsInstanceAndReferenceInRootSlice(rootSlice, secondSliceInstance));

        rootSlice->RestoreCachedInstances();
        EXPECT_TRUE(IsInstanceAndReferenceInRootSlice(rootSlice, instantiatedSlice));
        EXPECT_TRUE(IsInstanceAndReferenceInRootSlice(rootSlice, secondSliceInstance));

        DeleteSliceInstance(instantiatedSlice);
        DeleteSliceInstance(secondSliceInstance);
    }
    */
    TEST_F(EditorLayerComponentTest, LayerTests_DuplicateEntitiesInSceneAndLayer_DuplicateEntityIsDeleted)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        AZ::Entity* childEntity = CreateEditorReadyEntity("ChildEntity");
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        Layers::EditorLayer layer;
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        AZStd::vector<AZ::Entity*> savedEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs savedInstances;
        Layers::LayerResult saveResult = SaveMainLayer(layer, entitySaveStream, savedEntities, savedInstances);
        EXPECT_TRUE(saveResult.IsSuccess());

        EXPECT_EQ(savedEntities.size(), 1);
        EXPECT_NE(savedEntities[0], nullptr);
        EXPECT_EQ(savedEntities[0]->GetId(), childEntity->GetId());

        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::RestoreEditorData);

        AZStd::unordered_map<AZ::EntityId, AZ::Entity*> uniqueEntities;

        // ReadFromLayerStream will delete the older entities, so create an entity here that can be safely deleted.
        AZ::Entity* duplicateToBeDeletedEntity = aznew AZ::Entity(childEntity->GetId(), "GoingToBeDeleted");
        uniqueEntities[childEntity->GetId()] = duplicateToBeDeletedEntity;

        AZ::SliceComponent::SliceAssetToSliceInstancePtrs loadedInstances;
        Layers::LayerResult readResult = m_layerEntity.m_layer->ReadFromLayerStream(entitySaveStream, loadedInstances, uniqueEntities);
        EXPECT_TRUE(readResult.IsSuccess());

        EXPECT_EQ(uniqueEntities.size(), 1);
        EXPECT_NE(uniqueEntities.begin(), uniqueEntities.end());
        EXPECT_EQ(uniqueEntities.begin()->first, childEntity->GetId());
        EXPECT_NE(uniqueEntities.begin()->second, nullptr);
        EXPECT_EQ(uniqueEntities.begin()->second->GetId(), childEntity->GetId());
        EXPECT_NE(uniqueEntities.begin()->second, duplicateToBeDeletedEntity);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_DuplicateEntitiesInLayer_DuplicateEntityIsDeleted)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        AZ::Entity* childEntity = CreateEditorReadyEntity("ChildEntity");
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            m_layerEntity.m_entity->GetId());

        Layers::EditorLayer layer;
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        AZStd::vector<AZ::Entity*> savedEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs savedInstances;
        Layers::LayerResult saveResult = 
            m_layerEntity.m_layer->CreateLayerStreamWithDuplicateEntity(
                savedEntities,
                savedInstances,
                entitySaveStream,
                layer);

        EXPECT_TRUE(saveResult.IsSuccess());

        EXPECT_EQ(savedEntities.size(), 1);
        EXPECT_NE(savedEntities[0], nullptr);
        EXPECT_EQ(savedEntities[0]->GetId(), childEntity->GetId());
        EXPECT_EQ(layer.m_layerEntities.size(), 2);
        // Make sure the saved data has a duplicate entity in it.
        EXPECT_EQ(layer.m_layerEntities[0]->GetId(), layer.m_layerEntities[1]->GetId());

        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::RestoreEditorData);

        AZStd::unordered_map<AZ::EntityId, AZ::Entity*> uniqueEntities;        
        AZ::SliceComponent::SliceAssetToSliceInstancePtrs loadedInstances;
        Layers::LayerResult readResult = m_layerEntity.m_layer->ReadFromLayerStream(entitySaveStream, loadedInstances, uniqueEntities);
        EXPECT_TRUE(readResult.IsSuccess());
        
        // Verify there is only one entity loaded from the layer.
        EXPECT_EQ(uniqueEntities.size(), 1);
        EXPECT_NE(uniqueEntities.begin(), uniqueEntities.end());
        EXPECT_EQ(uniqueEntities.begin()->first, childEntity->GetId());
        EXPECT_NE(uniqueEntities.begin()->second, nullptr);
        EXPECT_EQ(uniqueEntities.begin()->second->GetId(), childEntity->GetId());
    }

    TEST_F(EditorLayerComponentTest, LayerTests_RestoreNullLayer_FailsToRestore)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        AZ::EntityId invalidParentId;
        Layers::LayerResult recoveryResult = AzToolsFramework::Layers::EditorLayerComponent::RecoverEditorLayer(nullptr, "RecoveredLayerName", invalidParentId);
        EXPECT_FALSE(recoveryResult.IsSuccess());
    }

    TEST_F(EditorLayerComponentTest, LayerTests_RestoreEmptyLayer_RestoresCorrectly)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        AZ::EntityId layerEntityId = m_layerEntity.m_entity->GetId();
        // Check that the layer is in the scene.
        AZ::SliceComponent* rootSlice;
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
            rootSlice, &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
        EXPECT_NE(rootSlice, nullptr);
        const EntityList& looseEntities(rootSlice->GetNewEntities());
        EXPECT_EQ(looseEntities.size(), 1);
        EXPECT_EQ(looseEntities[0]->GetId(), layerEntityId);

        // Next, save that layer to a stream.
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        AZStd::vector<AZ::Entity*> savedEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs savedInstances;
        Layers::EditorLayer layer;
        Layers::LayerResult saveResult = SaveMainLayer(layer, entitySaveStream, savedEntities, savedInstances);
        EXPECT_TRUE(saveResult.IsSuccess());

        // Delete the layer from the scene.
        bool layerDeleted = false;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            layerDeleted,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity,
            m_layerEntity.m_entity->GetId());
        EXPECT_TRUE(layerDeleted);
        m_layerEntity.m_entity = nullptr;
        // Verify that the layer is gone.
        EXPECT_EQ(looseEntities.size(), 0);

        // Load the layer object from the stream it was saved to.
        AZStd::shared_ptr<Layers::EditorLayer> loadedFromStream(AZ::Utils::LoadObjectFromStream<Layers::EditorLayer>(entitySaveStream));
        EXPECT_NE(loadedFromStream, nullptr);

        // Attempt to recover the empty layer, which should succeed.
        AZ::EntityId invalidParentId;
        Layers::LayerResult recoveryResult = AzToolsFramework::Layers::EditorLayerComponent::RecoverEditorLayer(loadedFromStream, "RecoveredLayerName", invalidParentId);
        EXPECT_TRUE(recoveryResult.IsSuccess());

        // Verify the layer was restored.
        EXPECT_EQ(looseEntities.size(), 1);
        EXPECT_EQ(looseEntities[0]->GetId(), layerEntityId);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_RestoreLayerButLayerStillInScene_FailsToRestore)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        AZ::EntityId layerEntityId = m_layerEntity.m_entity->GetId();
        // Check that the layer is in the scene.
        AZ::SliceComponent* rootSlice;
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
            rootSlice, &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
        EXPECT_NE(rootSlice, nullptr);
        const EntityList& looseEntities(rootSlice->GetNewEntities());
        EXPECT_EQ(looseEntities.size(), 1);
        EXPECT_EQ(looseEntities[0]->GetId(), layerEntityId);

        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity,
            layerEntityId);

        // Next, save that layer to a stream.
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        AZStd::vector<AZ::Entity*> savedEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs savedInstances;
        Layers::EditorLayer layer;
        Layers::LayerResult saveResult = SaveMainLayer(layer, entitySaveStream, savedEntities, savedInstances);
        EXPECT_TRUE(saveResult.IsSuccess());

        // Load the layer object from the stream it was saved to.
        AZStd::shared_ptr<Layers::EditorLayer> loadedFromStream(AZ::Utils::LoadObjectFromStream<Layers::EditorLayer>(entitySaveStream));
        EXPECT_NE(loadedFromStream, nullptr);

        // Attempt to recover the layer, which should fail because it's still in the scene.
        AZ::EntityId invalidParentId;
        Layers::LayerResult recoveryResult = AzToolsFramework::Layers::EditorLayerComponent::RecoverEditorLayer(loadedFromStream, "RecoveredLayerName", invalidParentId);
        EXPECT_FALSE(recoveryResult.IsSuccess());
    }

    TEST_F(EditorLayerComponentTest, LayerTests_RestoreLayerWithEntityChild_RestoresCorrectly)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        const AZ::EntityId layerEntityId = m_layerEntity.m_entity->GetId();

        AZ::Entity* childEntity = CreateEditorReadyEntity("ChildEntity");
        const AZ::EntityId childEntityId = childEntity->GetId();
        AZ::TransformBus::Event(
            childEntityId,
            &AZ::TransformBus::Events::SetParent,
            layerEntityId);

        // Check that the layer and the child entity are in the scene.
        AZ::SliceComponent* rootSlice;
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
            rootSlice, &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
        EXPECT_NE(rootSlice, nullptr);
        const EntityList& looseEntities(rootSlice->GetNewEntities());
        EXPECT_EQ(looseEntities.size(), 2);
        bool foundLayer = false;
        bool foundChild = false;
        for (const auto& entity : looseEntities)
        {
            if (entity->GetId() == layerEntityId)
            {
                foundLayer = true;
            }
            else if (entity->GetId() == childEntityId)
            {
                foundChild = true;
            }
        }
        EXPECT_TRUE(foundLayer);
        EXPECT_TRUE(foundChild);

        // Next, save that layer to a stream.
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        AZStd::vector<AZ::Entity*> savedEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs savedInstances;
        Layers::EditorLayer layer;
        Layers::LayerResult saveResult = SaveMainLayer(layer, entitySaveStream, savedEntities, savedInstances);
        EXPECT_TRUE(saveResult.IsSuccess());

        // Delete the child from the scene.
        bool childDeleted = false;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            childDeleted,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity,
            childEntityId);
        EXPECT_TRUE(childDeleted);
        childEntity = nullptr;

        // Delete the layer from the scene.
        bool layerDeleted = false;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            layerDeleted,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity,
            layerEntityId);
        EXPECT_TRUE(layerDeleted);
        m_layerEntity.m_entity = nullptr;

        // Verify that the layer is gone.
        EXPECT_EQ(looseEntities.size(), 0);

        // Load the layer object from the stream it was saved to.
        AZStd::shared_ptr<Layers::EditorLayer> loadedFromStream(AZ::Utils::LoadObjectFromStream<Layers::EditorLayer>(entitySaveStream));
        EXPECT_NE(loadedFromStream, nullptr);

        // Recover the layer.
        AZ::EntityId invalidParentId;
        Layers::LayerResult recoveryResult = AzToolsFramework::Layers::EditorLayerComponent::RecoverEditorLayer(loadedFromStream, "RecoveredLayerName", invalidParentId);
        EXPECT_TRUE(recoveryResult.IsSuccess());

        // Verify the layer and child were restored.
        EXPECT_EQ(looseEntities.size(), 2);
        foundLayer = false;
        foundChild = false;
        for (const auto& entity : looseEntities)
        {
            if (entity->GetId() == layerEntityId)
            {
                foundLayer = true;
            }
            else if (entity->GetId() == childEntityId)
            {
                foundChild = true;
            }
        }
        EXPECT_TRUE(foundLayer);
        EXPECT_TRUE(foundChild);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_RestoreLayerWithEntityChildStillInScene_FailsToRestore)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        const AZ::EntityId layerEntityId = m_layerEntity.m_entity->GetId();

        AZ::Entity* childEntity = CreateEditorReadyEntity("ChildEntity");
        const AZ::EntityId childEntityId = childEntity->GetId();
        AZ::TransformBus::Event(
            childEntityId,
            &AZ::TransformBus::Events::SetParent,
            layerEntityId);

        // Check that the layer and the child entity are in the scene.
        AZ::SliceComponent* rootSlice;
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
            rootSlice, &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
        EXPECT_NE(rootSlice, nullptr);
        const EntityList& looseEntities(rootSlice->GetNewEntities());
        EXPECT_EQ(looseEntities.size(), 2);
        bool foundLayer = false;
        bool foundChild = false;
        for (const auto& entity : looseEntities)
        {
            if (entity->GetId() == layerEntityId)
            {
                foundLayer = true;
            }
            else if (entity->GetId() == childEntityId)
            {
                foundChild = true;
            }
        }
        EXPECT_TRUE(foundLayer);
        EXPECT_TRUE(foundChild);

        // Next, save that layer to a stream.
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        AZStd::vector<AZ::Entity*> savedEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs savedInstances;
        Layers::EditorLayer layer;
        Layers::LayerResult saveResult = SaveMainLayer(layer, entitySaveStream, savedEntities, savedInstances);
        EXPECT_TRUE(saveResult.IsSuccess());

        // Set the child's parent to invalid, so it sticks around after the layer is deleted
        AZ::TransformBus::Event(
            childEntityId,
            &AZ::TransformBus::Events::SetParent,
            AZ::EntityId());

        // Delete the layer from the scene.
        bool layerDeleted = false;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            layerDeleted,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity,
            layerEntityId);
        EXPECT_TRUE(layerDeleted);
        m_layerEntity.m_entity = nullptr;

        // Verify that the layer is gone, but the child is still in the scene.
        EXPECT_EQ(looseEntities.size(), 1);
        EXPECT_EQ(looseEntities[0]->GetId(), childEntityId);

        // Load the layer object from the stream it was saved to.
        AZStd::shared_ptr<Layers::EditorLayer> loadedFromStream(AZ::Utils::LoadObjectFromStream<Layers::EditorLayer>(entitySaveStream));
        EXPECT_NE(loadedFromStream, nullptr);

        // Attempt to recover the layer, which should fail because the child is still in the scene.
        AZ::EntityId invalidParentId;
        Layers::LayerResult recoveryResult = AzToolsFramework::Layers::EditorLayerComponent::RecoverEditorLayer(loadedFromStream, "RecoveredLayerName", invalidParentId);
        EXPECT_FALSE(recoveryResult.IsSuccess());

        // Verify the layer was not restored.
        EXPECT_EQ(looseEntities.size(), 1);
        EXPECT_EQ(looseEntities[0]->GetId(), childEntityId);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_RestoreLayerWithSliceInstance_RestoresCorrectly)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        const AZ::EntityId layerEntityId = m_layerEntity.m_entity->GetId();
        // First, set up a layer with a slice instance in it.
        AZ::SliceComponent::SliceInstanceAddress instantiatedSlice = CreateSliceInstance();
        AZ::Data::Asset<AZ::SliceAsset> loadedSliceAsset = instantiatedSlice.first->GetSliceAsset();
        AZ::Entity* childEntity = GetEntityFromSliceInstance(instantiatedSlice);
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            layerEntityId);

        // Check that the layer is in the scene.
        AZ::SliceComponent* rootSlice;
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
            rootSlice, &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
        EXPECT_NE(rootSlice, nullptr);
        const EntityList& looseEntities(rootSlice->GetNewEntities());
        EXPECT_EQ(looseEntities.size(), 1);
        EXPECT_EQ(looseEntities[0]->GetId(), layerEntityId);

        // Next, save that layer to a stream.
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        AZStd::vector<AZ::Entity*> savedEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs savedInstances;
        Layers::EditorLayer layer;
        Layers::LayerResult saveResult = SaveMainLayer(layer, entitySaveStream, savedEntities, savedInstances);
        EXPECT_TRUE(saveResult.IsSuccess());

        // Clear out the scene, delete the instance and the layer.
        DeleteSliceInstance(instantiatedSlice);

        bool layerDeleted = false;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            layerDeleted,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity,
            layerEntityId);
        EXPECT_TRUE(layerDeleted);
        m_layerEntity.m_entity = nullptr;

        // Verify the slice instance and layer have been removed from the scene.
        AZStd::list<AZ::SliceComponent::SliceReference>& sliceList = rootSlice->GetSlices();
        EXPECT_EQ(sliceList.size(), 0);
        // Verify the layer is gone.
        EXPECT_EQ(looseEntities.size(), 0);

        // Load the layer object from the stream it was saved to.
        AZStd::shared_ptr<Layers::EditorLayer> loadedFromStream(AZ::Utils::LoadObjectFromStream<Layers::EditorLayer>(entitySaveStream));
        EXPECT_NE(loadedFromStream, nullptr);

        // Recover the layer. It has no parent, so use an invalid ID.
        AZ::EntityId invalidParentId;
        Layers::LayerResult recoveryResult = AzToolsFramework::Layers::EditorLayerComponent::RecoverEditorLayer(loadedFromStream, "RecoveredLayerName", invalidParentId);
        EXPECT_TRUE(recoveryResult.IsSuccess());

        // Verify the slice instance is now in the scene.
        EXPECT_EQ(sliceList.size(), 1);
        EXPECT_EQ(sliceList.front().GetSliceAsset(), loadedSliceAsset);
        AZStd::unordered_set<AZ::SliceComponent::SliceInstance>& sliceInstances = sliceList.front().GetInstances();
        EXPECT_EQ(sliceInstances.size(), 1);

        // Verify the layer is now in the scene.
        EXPECT_EQ(looseEntities.size(), 1);
        EXPECT_EQ(looseEntities[0]->GetId(), layerEntityId);

        // Clean up the slice instance.

        // There's only one instance, so no need to loop to clear it out.
        // Convert the instance from an iterator to a pointer, to remove it.
        rootSlice->RemoveSliceInstance(&(*sliceInstances.begin()));
    }

    TEST_F(EditorLayerComponentTest, LayerTests_RestoreLayerWithSliceInstanceStillInScene_FailsToRestore)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        const AZ::EntityId layerEntityId = m_layerEntity.m_entity->GetId();
        // First, set up a layer with a slice instance in it.
        AZ::SliceComponent::SliceInstanceAddress instantiatedSlice = CreateSliceInstance();
        AZ::Data::Asset<AZ::SliceAsset> loadedSliceAsset = instantiatedSlice.first->GetSliceAsset();
        AZ::Entity* childEntity = GetEntityFromSliceInstance(instantiatedSlice);
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            layerEntityId);

        // Check that the layer is in the scene.
        AZ::SliceComponent* rootSlice;
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
            rootSlice, &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
        EXPECT_NE(rootSlice, nullptr);
        const EntityList& looseEntities(rootSlice->GetNewEntities());
        EXPECT_EQ(looseEntities.size(), 1);
        EXPECT_EQ(looseEntities[0]->GetId(), layerEntityId);

        // Next, save that layer to a stream.
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        AZStd::vector<AZ::Entity*> savedEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs savedInstances;
        Layers::EditorLayer layer;
        Layers::LayerResult saveResult = SaveMainLayer(layer, entitySaveStream, savedEntities, savedInstances);
        EXPECT_TRUE(saveResult.IsSuccess());

        // Move the slice instance out of the layer, so the entity ID remains active and conflicts with the layer.
        AZ::TransformBus::Event(
            childEntity->GetId(),
            &AZ::TransformBus::Events::SetParent,
            AZ::EntityId());

        bool layerDeleted = false;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            layerDeleted,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity,
            layerEntityId);
        EXPECT_TRUE(layerDeleted);
        m_layerEntity.m_entity = nullptr;

        // Verify the layer is gone.
        EXPECT_EQ(looseEntities.size(), 0);

        // Load the layer object from the stream it was saved to.
        AZStd::shared_ptr<Layers::EditorLayer> loadedFromStream(AZ::Utils::LoadObjectFromStream<Layers::EditorLayer>(entitySaveStream));
        EXPECT_NE(loadedFromStream, nullptr);

        // Attempt to recover the layer. It has no parent, so use an invalid ID.
        AZ::EntityId invalidParentId;
        Layers::LayerResult recoveryResult = AzToolsFramework::Layers::EditorLayerComponent::RecoverEditorLayer(loadedFromStream, "RecoveredLayerName", invalidParentId);
        // Verify it failed to recover.
        EXPECT_FALSE(recoveryResult.IsSuccess());

        // Verify the slice instance is now in the scene.
        AZStd::list<AZ::SliceComponent::SliceReference>& sliceList = rootSlice->GetSlices();
        EXPECT_EQ(sliceList.size(), 1);
        EXPECT_EQ(sliceList.front().GetSliceAsset(), loadedSliceAsset);
        AZStd::unordered_set<AZ::SliceComponent::SliceInstance>& sliceInstances = sliceList.front().GetInstances();
        EXPECT_EQ(sliceInstances.size(), 1);

        // Verify the layer is not in the scene. If it failed, it shouldn't create the layer.
        EXPECT_EQ(looseEntities.size(), 0);

        // Clean up the slice instance.
        DeleteSliceInstance(instantiatedSlice);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_AttemptToCopyLayerComponent_IsNotCopyable)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        AZ::Entity::ComponentArrayType components;
        components.push_back(m_layerEntity.m_layer);

        bool isCopyable = EntityPropertyEditor::AreComponentsCopyable(components, DummyComponentFilter);
       
        EXPECT_FALSE(isCopyable);
    }

    TEST_F(EditorLayerComponentTest, LayerTests_CheckOverwriteFlag_IsSetCorrectly)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        // Check layer created with correct value
        EXPECT_FALSE(m_layerEntity.m_layer->GetOverwriteFlag());
        //check direct call works correctly
        m_layerEntity.m_layer->SetOverwriteFlag(true);
        EXPECT_TRUE(m_layerEntity.m_layer->GetOverwriteFlag());
        // check bus works correctly
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            m_layerEntity.m_entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::SetOverwriteFlag, true);
        EXPECT_TRUE(m_layerEntity.m_layer->GetOverwriteFlag());
    }

    TEST_F(EditorLayerComponentTest, LayerTests_UndoRedoRestoreLayerWithChildren_AllRestoredEntitiesCorrect)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        const AZ::EntityId layerEntityId = m_layerEntity.m_entity->GetId();

        AZ::SliceComponent* rootSlice;
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
            rootSlice,
            &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
        EXPECT_NE(rootSlice, nullptr);

        // Set up a an entity and a slice instance as children of the layer.
        AZ::Entity* childEntity = CreateEditorReadyEntity("ChildEntity");
        const AZ::EntityId childEntityId = childEntity->GetId();
        AZ::TransformBus::Event(
            childEntityId,
            &AZ::TransformBus::Events::SetParent,
            layerEntityId);

        AZ::SliceComponent::SliceInstanceAddress instantiatedSlice = CreateSliceInstance();
        AZ::Data::Asset<AZ::SliceAsset> loadedSliceAsset = instantiatedSlice.first->GetSliceAsset();
        AZ::Entity* sliceEntity = GetEntityFromSliceInstance(instantiatedSlice);
        const AZ::EntityId sliceEntityId = sliceEntity->GetId();
        AZ::TransformBus::Event(
            sliceEntityId,
            &AZ::TransformBus::Events::SetParent,
            layerEntityId);

        const EntityList& looseEntities(rootSlice->GetNewEntities());
        EXPECT_EQ(looseEntities.size(), 2);

        // Check that the layers, the entity, and the slice are in the scene.
        EXPECT_TRUE(IsLooseEntityInRootSlice(rootSlice, layerEntityId));
        EXPECT_TRUE(IsLooseEntityInRootSlice(rootSlice, childEntityId));
        EXPECT_TRUE(IsInstanceAndReferenceInRootSlice(rootSlice, instantiatedSlice));

        // Next, save the layer to a stream.
        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        AZStd::vector<AZ::Entity*> savedEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs savedInstances;
        Layers::EditorLayer layer;

        Layers::LayerResult saveResult = m_layerEntity.m_layer->PopulateLayerWriteToStreamAndGetEntities(
            savedEntities,
            savedInstances,
            entitySaveStream,
            layer);

        EXPECT_TRUE(saveResult.IsSuccess());

        // Delete all the objects.
        bool entityDeleted = false;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            entityDeleted,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity,
            childEntityId);
        EXPECT_TRUE(entityDeleted);
        childEntity = nullptr;

        DeleteSliceInstance(instantiatedSlice);
        
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            entityDeleted,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity,
            layerEntityId);
        EXPECT_TRUE(entityDeleted);
        m_layerEntity.m_entity = nullptr;

        // Check that everything is gone.
        EXPECT_EQ(looseEntities.size(), 0);
        EXPECT_FALSE(IsInstanceAndReferenceInRootSlice(rootSlice, instantiatedSlice));
        
        // Load the layer object from the stream it was saved to.
        AZStd::shared_ptr<Layers::EditorLayer> loadedFromStream(AZ::Utils::LoadObjectFromStream<Layers::EditorLayer>(entitySaveStream));
        EXPECT_NE(loadedFromStream, nullptr);

        // Attempt to recover the layer. It has no parent, so use an invalid ID.
        AZ::EntityId invalidParentId;
        Layers::LayerResult recoveryResult = AzToolsFramework::Layers::EditorLayerComponent::RecoverEditorLayer(loadedFromStream, "RecoveredLayerName", invalidParentId);
        // Verify it recovered successfully.
        EXPECT_TRUE(recoveryResult.IsSuccess());
        
        // Verify the slice instance is now in the scene.
        AZStd::list<AZ::SliceComponent::SliceReference>& sliceList = rootSlice->GetSlices();
        EXPECT_EQ(sliceList.size(), 1);
        EXPECT_EQ(sliceList.front().GetSliceAsset(), loadedSliceAsset);
        AZStd::unordered_set<AZ::SliceComponent::SliceInstance>& sliceInstances = sliceList.front().GetInstances();
        EXPECT_EQ(sliceInstances.size(), 1);

        // Verify the loose entities are restored.
        EXPECT_EQ(looseEntities.size(), 2);

        // Check the hierarchy is correct.
        AZ::EntityId childParentId;
        AZ::TransformBus::EventResult(
            childParentId,
            childEntityId,
            &AZ::TransformBus::Events::GetParentId);
        EXPECT_EQ(childParentId, layerEntityId);

        AZ::EntityId sliceParentId;
        AZ::TransformBus::EventResult(
            sliceParentId,
            sliceEntityId,
            &AZ::TransformBus::Events::GetParentId);
        EXPECT_EQ(sliceParentId, layerEntityId);

        // Undo.
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::UndoPressed);
        
        // Check everything's gone again.
        EXPECT_EQ(looseEntities.size(), 0);
        EXPECT_EQ(sliceList.size(), 0);
        
        // Redo.
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::RedoPressed);
        
        // Check everything's back and the hierarchy is correct.
        EXPECT_EQ(looseEntities.size(), 2);
        EXPECT_TRUE(IsLooseEntityInRootSlice(rootSlice, layerEntityId));
        EXPECT_TRUE(IsLooseEntityInRootSlice(rootSlice, childEntityId));
        EXPECT_EQ(sliceList.size(), 1);

        AZ::EntityId childParentIdAfterRedo;
        AZ::TransformBus::EventResult(
            childParentIdAfterRedo,
            childEntityId,
            &AZ::TransformBus::Events::GetParentId);
        EXPECT_EQ(childParentIdAfterRedo, layerEntityId);

        AZ::EntityId sliceParentIdAfterRedo;
        AZ::TransformBus::EventResult(
            sliceParentIdAfterRedo,
            sliceEntityId,
            &AZ::TransformBus::Events::GetParentId);
        EXPECT_EQ(sliceParentIdAfterRedo, layerEntityId);

        rootSlice->RemoveAllEntities();
    }
}
