/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "LmbrCentralEditor.h"

#include "Audio/EditorAudioAreaEnvironmentComponent.h"
#include "Audio/EditorAudioEnvironmentComponent.h"
#include "Audio/EditorAudioListenerComponent.h"
#include "Audio/EditorAudioMultiPositionComponent.h"
#include "Audio/EditorAudioPreloadComponent.h"
#include "Audio/EditorAudioRtpcComponent.h"
#include "Audio/EditorAudioSwitchComponent.h"
#include "Audio/EditorAudioTriggerComponent.h"
#include "Scripting/EditorLookAtComponent.h"
#include "Scripting/EditorRandomTimedSpawnerComponent.h"
#include "Scripting/EditorSpawnerComponent.h"
#include "Scripting/EditorTagComponent.h"

#include "Editor/EditorCommentComponent.h"
#include "Shape/EditorAxisAlignedBoxShapeComponent.h"
#include "Shape/EditorAxisAlignedBoxShapeComponentMode.h"
#include "Shape/EditorBoxShapeComponent.h"
#include "Shape/EditorCapsuleShapeComponent.h"
#include "Shape/EditorCylinderShapeComponent.h"
#include "Shape/EditorDiskShapeComponent.h"
#include "Shape/EditorPolygonPrismShapeComponent.h"
#include "Shape/EditorQuadShapeComponent.h"
#include "Shape/EditorReferenceShapeComponent.h"
#include "Shape/EditorSphereShapeComponent.h"
#include "Shape/EditorSplineComponent.h"
#include "Shape/EditorSplineComponentMode.h"
#include "Shape/EditorTubeShapeComponent.h"
#include "Shape/EditorTubeShapeComponentMode.h"

#include "Shape/EditorCompoundShapeComponent.h"

#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>
#include <AzToolsFramework/ToolsComponents/EditorSelectionAccentSystemComponent.h>
#include <AzToolsFramework/ComponentModes/BoxComponentMode.h>
#include <Builders/BenchmarkAssetBuilder/BenchmarkAssetBuilderComponent.h>
#include <Builders/LevelBuilder/LevelBuilderComponent.h>
#include <Builders/LuaBuilder/LuaBuilderComponent.h>
#include <Builders/SliceBuilder/SliceBuilderComponent.h>
#include <Builders/TranslationBuilder/TranslationBuilderComponent.h>
#include "Builders/CopyDependencyBuilder/CopyDependencyBuilderComponent.h"

namespace LmbrCentral
{
    LmbrCentralEditorModule::LmbrCentralEditorModule()
        : LmbrCentralModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            EditorAudioAreaEnvironmentComponent::CreateDescriptor(),
            EditorAudioEnvironmentComponent::CreateDescriptor(),
            EditorAudioListenerComponent::CreateDescriptor(),
            EditorAudioMultiPositionComponent::CreateDescriptor(),
            EditorAudioPreloadComponent::CreateDescriptor(),
            EditorAudioRtpcComponent::CreateDescriptor(),
            EditorAudioSwitchComponent::CreateDescriptor(),
            EditorAudioTriggerComponent::CreateDescriptor(),
            EditorTagComponent::CreateDescriptor(),
            EditorSphereShapeComponent::CreateDescriptor(),
            EditorDiskShapeComponent::CreateDescriptor(),
            EditorTubeShapeComponent::CreateDescriptor(),
            EditorBoxShapeComponent::CreateDescriptor(),
            EditorAxisAlignedBoxShapeComponent::CreateDescriptor(),
            EditorQuadShapeComponent::CreateDescriptor(),
            EditorLookAtComponent::CreateDescriptor(),
            EditorCylinderShapeComponent::CreateDescriptor(),
            EditorCapsuleShapeComponent::CreateDescriptor(),
            EditorCompoundShapeComponent::CreateDescriptor(),
            EditorReferenceShapeComponent::CreateDescriptor(),
            EditorSplineComponent::CreateDescriptor(),
            EditorPolygonPrismShapeComponent::CreateDescriptor(),
            EditorCommentComponent::CreateDescriptor(),
            EditorRandomTimedSpawnerComponent::CreateDescriptor(),
            EditorSpawnerComponent::CreateDescriptor(),            
            CopyDependencyBuilder::CopyDependencyBuilderComponent::CreateDescriptor(),
            LevelBuilder::LevelBuilderComponent::CreateDescriptor(),
            SliceBuilder::BuilderPluginComponent::CreateDescriptor(),
            TranslationBuilder::BuilderPluginComponent::CreateDescriptor(),
            LuaBuilder::BuilderPluginComponent::CreateDescriptor(),

            BenchmarkAssetBuilder::BenchmarkAssetBuilderComponent::CreateDescriptor(),
        });

        AZStd::vector<AZ::Uuid> typeIds;
        typeIds.reserve(m_descriptors.size());
        for (AZ::ComponentDescriptor* descriptor : m_descriptors)
        {
            typeIds.emplace_back(descriptor->GetUuid());
        }
        AzFramework::MetricsPlainTextNameRegistrationBus::Broadcast(
            &AzFramework::MetricsPlainTextNameRegistrationBus::Events::RegisterForNameSending, typeIds);

        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    }

    LmbrCentralEditorModule::~LmbrCentralEditorModule()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
    }

    AZ::ComponentTypeList LmbrCentralEditorModule::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList requiredComponents = LmbrCentralModule::GetRequiredSystemComponents();

        requiredComponents.push_back(azrtti_typeid<AzToolsFramework::Components::EditorSelectionAccentSystemComponent>());

        return requiredComponents;
    }

    void LmbrCentralEditorModule::OnActionRegistrationHook()
    {
        EditorSplineComponentMode::RegisterActions();
        EditorTubeShapeComponentMode::RegisterActions();
        AzToolsFramework::BoxComponentMode::RegisterActions();
    }

    void LmbrCentralEditorModule::OnActionContextModeBindingHook()
    {
        EditorSplineComponentMode::BindActionsToModes();
        EditorTubeShapeComponentMode::BindActionsToModes();
        AzToolsFramework::BoxComponentMode::BindActionsToModes();
        EditorAxisAlignedBoxShapeComponentMode::BindActionsToModes();
    }

    void LmbrCentralEditorModule::OnMenuBindingHook()
    {
        EditorSplineComponentMode::BindActionsToMenus();
        EditorTubeShapeComponentMode::BindActionsToMenus();
        AzToolsFramework::BoxComponentMode::BindActionsToMenus();
    }

    void LmbrCentralEditorModule::OnPostActionManagerRegistrationHook()
    {
        AzToolsFramework::EditorVertexSelectionActionManagement::DisableComponentModeEndOnVertexSelection();
    }

} // namespace LmbrCentral

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), LmbrCentral::LmbrCentralEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_LmbrCentral_Editor, LmbrCentral::LmbrCentralEditorModule)
#endif
