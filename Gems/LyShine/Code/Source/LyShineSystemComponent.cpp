/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include "LyShineFeatureProcessor.h"
#include "LyShineSystemComponent.h"
#include "UiSerialize.h"

#include "UiCanvasFileObject.h"
#include "UiCanvasComponent.h"
#include "LyShineDebug.h"
#include "UiElementComponent.h"
#include "UiTransform2dComponent.h"
#include "UiImageComponent.h"
#include "UiImageSequenceComponent.h"
#include "UiTextComponent.h"
#include "UiButtonComponent.h"
#include "UiMarkupButtonComponent.h"
#include "UiCheckboxComponent.h"
#include "UiDraggableComponent.h"
#include "UiDropTargetComponent.h"
#include "UiDropdownComponent.h"
#include "UiDropdownOptionComponent.h"
#include "UiSliderComponent.h"
#include "UiTextInputComponent.h"
#include "UiScrollBarComponent.h"
#include "UiScrollBoxComponent.h"
#include "UiFaderComponent.h"
#include "UiLayoutFitterComponent.h"
#include "UiMaskComponent.h"
#include "UiLayoutCellComponent.h"
#include "UiLayoutColumnComponent.h"
#include "UiLayoutRowComponent.h"
#include "UiLayoutGridComponent.h"
#include "UiParticleEmitterComponent.h"
#include "UiFlipbookAnimationComponent.h"
#include "UiRadioButtonComponent.h"
#include "UiRadioButtonGroupComponent.h"
#include "UiTooltipComponent.h"
#include "UiTooltipDisplayComponent.h"
#include "UiDynamicLayoutComponent.h"
#include "UiDynamicScrollBoxComponent.h"
#include "UiNavigationSettings.h"
#include "LyShinePass.h"

namespace LyShine
{
    const AZStd::list<AZ::ComponentDescriptor*>* LyShineSystemComponent::m_componentDescriptors = nullptr;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        UiSerialize::ReflectUiTypes(context);
        UiCanvasFileObject::Reflect(context);
        UiNavigationSettings::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<LyShineSystemComponent, AZ::Component>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("AssetBuilder") }))
                ->Field("CursorImagePath", &LyShineSystemComponent::m_cursorImagePathname);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                auto editInfo = ec->Class<LyShineSystemComponent>("LyShine", "In-game User Interface System");
                editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "UI")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &LyShineSystemComponent::m_cursorImagePathname, "CursorImagePath", "The cursor image path.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &LyShineSystemComponent::BroadcastCursorImagePathname);
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<UiCanvasManagerBus>("UiCanvasManagerBus")
                ->Event("CreateCanvas", &UiCanvasManagerBus::Events::CreateCanvas)
                ->Event("LoadCanvas", &UiCanvasManagerBus::Events::LoadCanvas)
                ->Event("UnloadCanvas", &UiCanvasManagerBus::Events::UnloadCanvas)
                ->Event("FindLoadedCanvasByPathName", &UiCanvasManagerBus::Events::FindLoadedCanvasByPathName)
            ;

            behaviorContext->EBus<UiCursorBus>("UiCursorBus")
                ->Event("IncrementVisibleCounter", &UiCursorBus::Events::IncrementVisibleCounter)
                ->Event("DecrementVisibleCounter", &UiCursorBus::Events::DecrementVisibleCounter)
                ->Event("IsUiCursorVisible", &UiCursorBus::Events::IsUiCursorVisible)
                ->Event("SetUiCursor", &UiCursorBus::Events::SetUiCursor)
                ->Event("GetUiCursorPosition", &UiCursorBus::Events::GetUiCursorPosition)
                ->Event("SetUiCursorPosition", &UiCursorBus::Events::SetUiCursorPosition)
                ;
        }
        
        LyShineFeatureProcessor::Reflect(context);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("LyShineService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("LyShineService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
#if !defined(LYSHINE_BUILDER) && !defined(LYSHINE_TESTS)
        required.push_back(AZ_CRC_CE("RPISystem"));
#endif
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("AssetDatabaseService"));
        dependent.push_back(AZ_CRC_CE("AssetCatalogService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::SetLyShineComponentDescriptors(const AZStd::list<AZ::ComponentDescriptor*>* descriptors)
    {
        m_componentDescriptors = descriptors;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    LyShineSystemComponent::LyShineSystemComponent()
    {
        m_cursorImagePathname.SetAssetPath("Textures/Cursor_Default.tif");
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::Init()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::Activate()
    {
        UiSystemBus::Handler::BusConnect();
        UiSystemToolsBus::Handler::BusConnect();
        UiFrameworkBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();

        // register all the component types internal to the LyShine module
        // These are registered in the order we want them to appear in the Add Component menu
        RegisterComponentTypeForMenuOrdering(UiCanvasComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiElementComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiTransform2dComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiImageComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiImageSequenceComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiTextComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiButtonComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiMarkupButtonComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiCheckboxComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiRadioButtonComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiRadioButtonGroupComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiSliderComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiTextInputComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiScrollBarComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiScrollBoxComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiDraggableComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiDropTargetComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiDropdownComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiDropdownOptionComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiFaderComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiMaskComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiLayoutColumnComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiLayoutRowComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiLayoutGridComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiLayoutCellComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiLayoutFitterComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiTooltipComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiTooltipDisplayComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiDynamicLayoutComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiDynamicScrollBoxComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiParticleEmitterComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiFlipbookAnimationComponent::RTTI_Type());

#if !defined(LYSHINE_BUILDER) && !defined(LYSHINE_TESTS)
        // Add LyShine pass
        auto* passSystem = AZ::RPI::PassSystemInterface::Get();
        AZ_Assert(passSystem, "Cannot get the pass system.");
        passSystem->AddPassCreator(AZ::Name("LyShinePass"), &LyShine::LyShinePass::Create);
        passSystem->AddPassCreator(AZ::Name("LyShineChildPass"), &LyShine::LyShineChildPass::Create);
        passSystem->AddPassCreator(AZ::Name("RttChildPass"), &LyShine::RttChildPass::Create);

        // Setup handler for load pass template mappings
        m_loadTemplatesHandler = AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler([this]() { this->LoadPassTemplateMappings(); });
        AZ::RPI::PassSystemInterface::Get()->ConnectEvent(m_loadTemplatesHandler);
        
        // Register feature processor
        AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<LyShineFeatureProcessor>();
#endif
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::Deactivate()
    {
#if !defined(LYSHINE_BUILDER) && !defined(LYSHINE_TESTS)
        m_loadTemplatesHandler.Disconnect();        
        AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<LyShineFeatureProcessor>();
#endif

        UiSystemBus::Handler::BusDisconnect();
        UiSystemToolsBus::Handler::BusDisconnect();
        UiFrameworkBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::RegisterComponentTypeForMenuOrdering(const AZ::Uuid& typeUuid)
    {
        m_componentTypes.push_back(typeUuid);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    const AZStd::vector<AZ::Uuid>* LyShineSystemComponent::GetComponentTypesForMenuOrdering()
    {
        return &m_componentTypes;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    const AZStd::list<AZ::ComponentDescriptor*>* LyShineSystemComponent::GetLyShineComponentDescriptors()
    {
        return m_componentDescriptors;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    UiSystemToolsInterface::CanvasAssetHandle* LyShineSystemComponent::LoadCanvasFromStream(AZ::IO::GenericStream& stream, const AZ::ObjectStream::FilterDescriptor& filterDesc)
    {
        return UiCanvasFileObject::LoadCanvasFromStream(stream, filterDesc);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::SaveCanvasToStream(UiSystemToolsInterface::CanvasAssetHandle* canvas, AZ::IO::FileIOStream& stream)
    {
        UiCanvasFileObject* canvasFileObject = static_cast<UiCanvasFileObject*>(canvas);
        UiCanvasFileObject::SaveCanvasToStream(stream, canvasFileObject);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Entity* LyShineSystemComponent::GetRootSliceEntity(CanvasAssetHandle* canvas)
    {
        UiCanvasFileObject* canvasFileObject = static_cast<UiCanvasFileObject*>(canvas);
        return canvasFileObject->m_rootSliceEntity;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Entity* LyShineSystemComponent::GetCanvasEntity(CanvasAssetHandle* canvas)
    {
        UiCanvasFileObject* canvasFileObject = static_cast<UiCanvasFileObject*>(canvas);
        return canvasFileObject->m_canvasEntity;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::SliceComponent* LyShineSystemComponent::GetRootSliceSliceComponent(UiSystemToolsInterface::CanvasAssetHandle* canvas)
    {
        UiCanvasFileObject* canvasFileObject = static_cast<UiCanvasFileObject*>(canvas);
        AZ::Entity* rootSliceEntity = canvasFileObject->m_rootSliceEntity;

        if (rootSliceEntity->GetState() == AZ::Entity::State::Constructed)
        {
            rootSliceEntity->Init();
        }

        AZ::SliceComponent* sliceComponent = rootSliceEntity->FindComponent<AZ::SliceComponent>();
        return sliceComponent;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::ReplaceRootSliceSliceComponent(UiSystemToolsInterface::CanvasAssetHandle* canvas, AZ::SliceComponent* newSliceComponent)
    {
        UiCanvasFileObject* canvasFileObject = static_cast<UiCanvasFileObject*>(canvas);
        AZ::Entity* oldRootSliceEntity = canvasFileObject->m_rootSliceEntity;
        AZ::EntityId idToReuse = oldRootSliceEntity->GetId();
        
        AZ::Entity* newRootSliceEntity = aznew AZ::Entity(idToReuse, AZStd::to_string(static_cast<AZ::u64>(idToReuse)).c_str());
        newRootSliceEntity->AddComponent(newSliceComponent);
        canvasFileObject->m_rootSliceEntity = newRootSliceEntity;

        delete oldRootSliceEntity;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::ReplaceCanvasEntity(UiSystemToolsInterface::CanvasAssetHandle* canvas, AZ::Entity* newCanvasEntity)
    {
        UiCanvasFileObject* canvasFileObject = static_cast<UiCanvasFileObject*>(canvas);
        canvasFileObject->m_canvasEntity = newCanvasEntity;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::DestroyCanvas(CanvasAssetHandle* canvas)
    {
        UiCanvasFileObject* canvasFileObject = static_cast<UiCanvasFileObject*>(canvas);
        delete canvasFileObject->m_canvasEntity;
        delete canvasFileObject->m_rootSliceEntity;
        delete canvasFileObject;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool LyShineSystemComponent::HasUiElementComponent(AZ::Entity* entity)
    {
        return entity->FindComponent<UiElementComponent>() != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::AddEditorOnlyEntity(AZ::Entity* editorOnlyEntity, EntityIdSet& editorOnlyEntities)
    {
        // All descendents of an editor-only entity are considered editor-only also.
        // Iterate through all the descedents of the given entity and add their IDs
        // to the list of editor-only entities.
        AZStd::vector<AZ::Entity*> childEntities = { editorOnlyEntity };
        while (!childEntities.empty())
        {
            AZ::Entity* parentEntity = childEntities.back();
            childEntities.pop_back();
            editorOnlyEntities.insert(parentEntity->GetId());

            UiElementComponent* elementComponent = parentEntity->FindComponent<UiElementComponent>();
            if (elementComponent)
            {
                int numChildren = elementComponent->GetNumChildElements();
                for (int i = 0; i < numChildren; ++i)
                {
                    childEntities.push_back(elementComponent->GetChildElement(i));
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::HandleEditorOnlyEntities(const EntityList& exportSliceEntities, const EntityIdSet& editorOnlyEntityIds)
    {
        AZStd::unordered_map<AZ::EntityId, AZStd::vector<AZ::EntityId>> parentToChildren;

        // Build a map of entity Ids to their parent Ids, for faster lookup during processing.
        for (AZ::Entity* exportParentEntity : exportSliceEntities)
        {
            UiElementComponent* exportParentComponent = exportParentEntity->FindComponent<UiElementComponent>();
            if (!exportParentComponent)
            {
                continue;
            }

            // Map the child entities to the parent ID
            int numChildElements = exportParentComponent->GetNumChildElements();
            for (int exportChildIndex = 0; exportChildIndex < numChildElements; ++exportChildIndex)
            {
                AZ::EntityId childExportEntity = exportParentComponent->GetChildEntityId(exportChildIndex);
                parentToChildren[exportParentEntity->GetId()].push_back(childExportEntity);
            }
        }

        // Remove editor-only entities from parent hierarchy
        for (AZ::Entity* exportParentEntity : exportSliceEntities)
        {
            for (AZ::EntityId exportChildEntity : parentToChildren[exportParentEntity->GetId()])
            {
                const bool childIsEditorOnly = editorOnlyEntityIds.end() != editorOnlyEntityIds.find(exportChildEntity);
                if (childIsEditorOnly)
                {
                    UiElementComponent* exportParentComponent = exportParentEntity->FindComponent<UiElementComponent>();
                    exportParentComponent->RemoveChild(exportChildEntity);
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::OnCrySystemInitialized(ISystem& system, [[maybe_unused]] const SSystemInitParams& startupParams)
    {
#if !defined(AZ_MONOLITHIC_BUILD)
        // When module is linked dynamically, we must set our gEnv pointer.
        // When module is linked statically, we'll share the application's gEnv pointer.
        gEnv = system.GetGlobalEnvironment();
#endif
        m_lyShine = AZStd::make_unique<CLyShine>();
        AZ::Interface<ILyShine>::Register(m_lyShine.get());

        system.GetILevelSystem()->AddListener(this);

        BroadcastCursorImagePathname();

        if (AZ::Interface<ILyShine>::Get())
        {
            AZ::Interface<ILyShine>::Get()->PostInit();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::OnCrySystemShutdown(ISystem& system)
    {
        system.GetILevelSystem()->RemoveListener(this);

        if (m_lyShine)
        {
            AZ::Interface<ILyShine>::Unregister(m_lyShine.get());
            m_lyShine.reset();
        }
    }

    ////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::OnUnloadComplete([[maybe_unused]] const char* levelName)
    {
        // Perform level unload procedures for the LyShine UI system
        if (AZ::Interface<ILyShine>::Get())
        {
            AZ::Interface<ILyShine>::Get()->OnLevelUnload();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::BroadcastCursorImagePathname()
    {
        UiCursorBus::Broadcast(&UiCursorInterface::SetUiCursor, m_cursorImagePathname.GetAssetPath().c_str());
    }

#if !defined(LYSHINE_BUILDER) && !defined(LYSHINE_TESTS)
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void LyShineSystemComponent::LoadPassTemplateMappings()
    {
        const char* passTemplatesFile = "Passes/LyShinePassTemplates.azasset";
        AZ::RPI::PassSystemInterface::Get()->LoadPassTemplateMappings(passTemplatesFile);
    }
#endif
}
