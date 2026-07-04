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
#include "ShineFeatureProcessor.h"
#include "ShineSystemComponent.h"
#include "UiSerialize.h"

#include "UiCanvasFileObject.h"
#include "UiCanvasComponent.h"
#include "ShineDebug.h"
#include "UiElementComponent.h"
#include "UiHierarchyInteractivityToggleComponent.h"
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
#include "ShinePass.h"

namespace Shine
{
    const AZStd::list<AZ::ComponentDescriptor*>* ShineSystemComponent::m_componentDescriptors = nullptr;

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        UiSerialize::ReflectUiTypes(context);
        UiCanvasFileObject::Reflect(context);
        UiNavigationSettings::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ShineSystemComponent, AZ::Component>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("AssetBuilder") }))
                ->Field("CursorImagePath", &ShineSystemComponent::m_cursorImagePathname);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                auto editInfo = ec->Class<ShineSystemComponent>("Shine", "In-game User Interface System");
                editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "UI")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &ShineSystemComponent::m_cursorImagePathname, "CursorImagePath", "The cursor image path.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ShineSystemComponent::BroadcastCursorImagePathname);
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
        
        ShineFeatureProcessor::Reflect(context);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ShineService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ShineService"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // RPISystem is only available at runtime (Editor/Game), not in builders or tests.
        // Use GetDependentServices for soft dependency instead of hard requirement,
        // since this component is also loaded in AssetBuilder where RPI is not present.
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("AssetDatabaseService"));
        dependent.push_back(AZ_CRC_CE("AssetCatalogService"));
        dependent.push_back(AZ_CRC_CE("RPISystem"));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::SetShineComponentDescriptors(const AZStd::list<AZ::ComponentDescriptor*>* descriptors)
    {
        m_componentDescriptors = descriptors;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    ShineSystemComponent::ShineSystemComponent()
    {
        m_cursorImagePathname.SetAssetPath("Textures/Cursor_Default.tif");
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::Init()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::Activate()
    {
        UiSystemBus::Handler::BusConnect();
        UiSystemToolsBus::Handler::BusConnect();
        UiFrameworkBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();

        // register all the component types internal to the Shine module
        // These are registered in the order we want them to appear in the Add Component menu
        RegisterComponentTypeForMenuOrdering(UiCanvasComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiElementComponent::RTTI_Type());
        RegisterComponentTypeForMenuOrdering(UiHierarchyInteractivityToggleComponent::RTTI_Type());
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

#if !defined(SHINE_BUILDER) && !defined(SHINE_TESTS)
        // Add Shine pass — only if the RPI is actually available (not in AssetBuilder context)
        auto* passSystem = AZ::RPI::PassSystemInterface::Get();
        if (passSystem)
        {
            passSystem->AddPassCreator(AZ::Name("ShinePass"), &Shine::ShinePass::Create);
            passSystem->AddPassCreator(AZ::Name("ShineChildPass"), &Shine::ShineChildPass::Create);
            passSystem->AddPassCreator(AZ::Name("RttChildPass"), &Shine::RttChildPass::Create);

            // Setup handler for load pass template mappings
            m_loadTemplatesHandler = AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler([this]() { this->LoadPassTemplateMappings(); });
            AZ::RPI::PassSystemInterface::Get()->ConnectEvent(m_loadTemplatesHandler);

            // Register feature processor
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<ShineFeatureProcessor>();
        }
#endif
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::Deactivate()
    {
#if !defined(SHINE_BUILDER) && !defined(SHINE_TESTS)
        m_loadTemplatesHandler.Disconnect();
        auto* featureProcessorFactory = AZ::RPI::FeatureProcessorFactory::Get();
        if (featureProcessorFactory)
        {
            featureProcessorFactory->UnregisterFeatureProcessor<ShineFeatureProcessor>();
        }
#endif

        UiSystemBus::Handler::BusDisconnect();
        UiSystemToolsBus::Handler::BusDisconnect();
        UiFrameworkBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::RegisterComponentTypeForMenuOrdering(const AZ::Uuid& typeUuid)
    {
        m_componentTypes.push_back(typeUuid);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    const AZStd::vector<AZ::Uuid>* ShineSystemComponent::GetComponentTypesForMenuOrdering()
    {
        return &m_componentTypes;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    const AZStd::list<AZ::ComponentDescriptor*>* ShineSystemComponent::GetShineComponentDescriptors()
    {
        return m_componentDescriptors;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    UiSystemToolsInterface::CanvasAssetHandle* ShineSystemComponent::LoadCanvasFromStream(AZ::IO::GenericStream& stream, const AZ::ObjectStream::FilterDescriptor& filterDesc)
    {
        return UiCanvasFileObject::LoadCanvasFromStream(stream, filterDesc);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::SaveCanvasToStream(UiSystemToolsInterface::CanvasAssetHandle* canvas, AZ::IO::FileIOStream& stream)
    {
        UiCanvasFileObject* canvasFileObject = static_cast<UiCanvasFileObject*>(canvas);
        UiCanvasFileObject::SaveCanvasToStream(stream, canvasFileObject);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::vector<AZ::Entity*>& ShineSystemComponent::GetChildEntities(CanvasAssetHandle* canvas)
    {
        UiCanvasFileObject* canvasFileObject = static_cast<UiCanvasFileObject*>(canvas);
        return canvasFileObject->m_childEntities;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Entity* ShineSystemComponent::GetCanvasEntity(CanvasAssetHandle* canvas)
    {
        UiCanvasFileObject* canvasFileObject = static_cast<UiCanvasFileObject*>(canvas);
        return canvasFileObject->m_canvasEntity;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::ReplaceChildEntities(UiSystemToolsInterface::CanvasAssetHandle* canvas, AZStd::vector<AZ::Entity*> newEntities)
    {
        UiCanvasFileObject* canvasFileObject = static_cast<UiCanvasFileObject*>(canvas);
        canvasFileObject->m_childEntities = AZStd::move(newEntities);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::ReplaceCanvasEntity(UiSystemToolsInterface::CanvasAssetHandle* canvas, AZ::Entity* newCanvasEntity)
    {
        UiCanvasFileObject* canvasFileObject = static_cast<UiCanvasFileObject*>(canvas);
        canvasFileObject->m_canvasEntity = newCanvasEntity;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::DestroyCanvas(CanvasAssetHandle* canvas)
    {
        UiCanvasFileObject* canvasFileObject = static_cast<UiCanvasFileObject*>(canvas);
        delete canvasFileObject;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    bool ShineSystemComponent::HasUiElementComponent(AZ::Entity* entity)
    {
        return entity->FindComponent<UiElementComponent>() != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::AddEditorOnlyEntity(AZ::Entity* editorOnlyEntity, EntityIdSet& editorOnlyEntities)
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
    void ShineSystemComponent::HandleEditorOnlyEntities(const EntityList& exportEntities, const EntityIdSet& editorOnlyEntityIds)
    {
        AZStd::unordered_map<AZ::EntityId, AZStd::vector<AZ::EntityId>> parentToChildren;

        // Build a map of entity Ids to their parent Ids, for faster lookup during processing.
        for (AZ::Entity* exportParentEntity : exportEntities)
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
        for (AZ::Entity* exportParentEntity : exportEntities)
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
    void ShineSystemComponent::OnCrySystemInitialized(ISystem& system, [[maybe_unused]] const SSystemInitParams& startupParams)
    {
#if !defined(AZ_MONOLITHIC_BUILD)
        // When module is linked dynamically, we must set our gEnv pointer.
        // When module is linked statically, we'll share the application's gEnv pointer.
        gEnv = system.GetGlobalEnvironment();
#endif
        m_Shine = AZStd::make_unique<CShine>();
        AZ::Interface<IShine>::Register(m_Shine.get());

        system.GetILevelSystem()->AddListener(this);

        BroadcastCursorImagePathname();

        if (AZ::Interface<IShine>::Get())
        {
            AZ::Interface<IShine>::Get()->PostInit();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::OnCrySystemShutdown(ISystem& system)
    {
        system.GetILevelSystem()->RemoveListener(this);

        if (m_Shine)
        {
            AZ::Interface<IShine>::Unregister(m_Shine.get());
            m_Shine.reset();
        }
    }

    ////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::OnUnloadComplete([[maybe_unused]] const char* levelName)
    {
        // Perform level unload procedures for the Shine UI system
        if (AZ::Interface<IShine>::Get())
        {
            AZ::Interface<IShine>::Get()->OnLevelUnload();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::BroadcastCursorImagePathname()
    {
        UiCursorBus::Broadcast(&UiCursorInterface::SetUiCursor, m_cursorImagePathname.GetAssetPath().c_str());
    }

#if !defined(SHINE_BUILDER) && !defined(SHINE_TESTS)
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ShineSystemComponent::LoadPassTemplateMappings()
    {
        const char* passTemplatesFile = "Passes/ShinePassTemplates.azasset";
        AZ::RPI::PassSystemInterface::Get()->LoadPassTemplateMappings(passTemplatesFile);
    }
#endif
}
