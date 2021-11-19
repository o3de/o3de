/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "LmbrCentralEditor.h"

#include "Ai/EditorNavigationAreaComponent.h"
#include "Ai/EditorNavigationSeedComponent.h"
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

#include "Shape/EditorAxisAlignedBoxShapeComponent.h"
#include "Shape/EditorBoxShapeComponent.h"
#include "Shape/EditorQuadShapeComponent.h"
#include "Shape/EditorSphereShapeComponent.h"
#include "Shape/EditorDiskShapeComponent.h"
#include "Shape/EditorCylinderShapeComponent.h"
#include "Shape/EditorCapsuleShapeComponent.h"
#include "Shape/EditorSplineComponent.h"
#include "Shape/EditorTubeShapeComponent.h"
#include "Shape/EditorPolygonPrismShapeComponent.h"
#include "Editor/EditorCommentComponent.h"

#include "Shape/EditorCompoundShapeComponent.h"

#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>
#include <AzToolsFramework/ToolsComponents/EditorSelectionAccentSystemComponent.h>
#include <Builders/BenchmarkAssetBuilder/BenchmarkAssetBuilderComponent.h>
#include <Builders/LevelBuilder/LevelBuilderComponent.h>
#include <Builders/LuaBuilder/LuaBuilderComponent.h>
#include <Builders/SliceBuilder/SliceBuilderComponent.h>
#include <Builders/TranslationBuilder/TranslationBuilderComponent.h>
#include "Builders/CopyDependencyBuilder/CopyDependencyBuilderComponent.h"
#include <Builders/DependencyBuilder/DependencyBuilderComponent.h>

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
            EditorSplineComponent::CreateDescriptor(),
            EditorPolygonPrismShapeComponent::CreateDescriptor(),
            EditorCommentComponent::CreateDescriptor(),
            EditorNavigationAreaComponent::CreateDescriptor(),
            EditorNavigationSeedComponent::CreateDescriptor(),
            EditorRandomTimedSpawnerComponent::CreateDescriptor(),
            EditorSpawnerComponent::CreateDescriptor(),            
            CopyDependencyBuilder::CopyDependencyBuilderComponent::CreateDescriptor(),
            DependencyBuilder::DependencyBuilderComponent::CreateDescriptor(),
            LevelBuilder::LevelBuilderComponent::CreateDescriptor(),
            SliceBuilder::BuilderPluginComponent::CreateDescriptor(),
            TranslationBuilder::BuilderPluginComponent::CreateDescriptor(),
            LuaBuilder::BuilderPluginComponent::CreateDescriptor(),

            BenchmarkAssetBuilder::BenchmarkAssetBuilderComponent::CreateDescriptor(),
        });

        // This is internal Amazon code, so register it's components for metrics tracking, otherwise the name of the component won't get sent back.
        AZStd::vector<AZ::Uuid> typeIds;
        typeIds.reserve(m_descriptors.size());
        for (AZ::ComponentDescriptor* descriptor : m_descriptors)
        {
            typeIds.emplace_back(descriptor->GetUuid());
        }
        EBUS_EVENT(AzFramework::MetricsPlainTextNameRegistrationBus, RegisterForNameSending, typeIds);
    }

    LmbrCentralEditorModule::~LmbrCentralEditorModule()
    {
    }

    AZ::ComponentTypeList LmbrCentralEditorModule::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList requiredComponents = LmbrCentralModule::GetRequiredSystemComponents();

        requiredComponents.push_back(azrtti_typeid<AzToolsFramework::Components::EditorSelectionAccentSystemComponent>());

        return requiredComponents;
    }
} // namespace LmbrCentral

AZ_DECLARE_MODULE_CLASS(Gem_LmbrCentralEditor, LmbrCentral::LmbrCentralEditorModule)
