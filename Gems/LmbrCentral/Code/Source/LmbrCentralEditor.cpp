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

#include "LmbrCentralEditor.h"

#include "Ai/EditorNavigationAreaComponent.h"
#include "Ai/EditorNavigationSeedComponent.h"
#include "Animation/EditorAttachmentComponent.h"
#include "Audio/EditorAudioAreaEnvironmentComponent.h"
#include "Audio/EditorAudioEnvironmentComponent.h"
#include "Audio/EditorAudioListenerComponent.h"
#include "Audio/EditorAudioMultiPositionComponent.h"
#include "Audio/EditorAudioPreloadComponent.h"
#include "Audio/EditorAudioRtpcComponent.h"
#include "Audio/EditorAudioSwitchComponent.h"
#include "Audio/EditorAudioTriggerComponent.h"
#include "Rendering/EditorDecalComponent.h"
#include "Rendering/EditorLensFlareComponent.h"
#include "Rendering/EditorLightComponent.h"
#include "Rendering/EditorPointLightComponent.h"
#include "Rendering/EditorAreaLightComponent.h"
#include "Rendering/EditorProjectorLightComponent.h"
#include "Rendering/EditorEnvProbeComponent.h"
#include "Rendering/EditorHighQualityShadowComponent.h"
#include "Rendering/EditorMeshComponent.h"
#include "Rendering/EditorSkinnedMeshComponent.h"
#include "Rendering/EditorFogVolumeComponent.h"
#include "Rendering/EditorGeomCacheComponent.h"
#include "Scripting/EditorLookAtComponent.h"
#include "Scripting/EditorRandomTimedSpawnerComponent.h"
#include "Scripting/EditorSpawnerComponent.h"
#include "Scripting/EditorTagComponent.h"

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
#include <Builders/MaterialBuilder/MaterialBuilderComponent.h>
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
            EditorAttachmentComponent::CreateDescriptor(),
            EditorAudioAreaEnvironmentComponent::CreateDescriptor(),
            EditorAudioEnvironmentComponent::CreateDescriptor(),
            EditorAudioListenerComponent::CreateDescriptor(),
            EditorAudioMultiPositionComponent::CreateDescriptor(),
            EditorAudioPreloadComponent::CreateDescriptor(),
            EditorAudioRtpcComponent::CreateDescriptor(),
            EditorAudioSwitchComponent::CreateDescriptor(),
            EditorAudioTriggerComponent::CreateDescriptor(),
            EditorDecalComponent::CreateDescriptor(),
            EditorLensFlareComponent::CreateDescriptor(),
            EditorLightComponent::CreateDescriptor(),
            EditorPointLightComponent::CreateDescriptor(),
            EditorAreaLightComponent::CreateDescriptor(),
            EditorProjectorLightComponent::CreateDescriptor(),
            EditorEnvProbeComponent::CreateDescriptor(),
            EditorHighQualityShadowComponent::CreateDescriptor(),
            EditorMeshComponent::CreateDescriptor(),
            EditorSkinnedMeshComponent::CreateDescriptor(),
            EditorTagComponent::CreateDescriptor(),
            EditorSphereShapeComponent::CreateDescriptor(),
            EditorDiskShapeComponent::CreateDescriptor(),
            EditorTubeShapeComponent::CreateDescriptor(),
            EditorBoxShapeComponent::CreateDescriptor(),
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
            EditorFogVolumeComponent::CreateDescriptor(),
            EditorRandomTimedSpawnerComponent::CreateDescriptor(),
            EditorGeometryCacheComponent::CreateDescriptor(),
            EditorSpawnerComponent::CreateDescriptor(),            
            CopyDependencyBuilder::CopyDependencyBuilderComponent::CreateDescriptor(),
            DependencyBuilder::DependencyBuilderComponent::CreateDescriptor(),
            LevelBuilder::LevelBuilderComponent::CreateDescriptor(),
            MaterialBuilder::BuilderPluginComponent::CreateDescriptor(),
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

        EditorMeshBus::Handler::BusConnect();
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

    bool LmbrCentralEditorModule::AddMeshComponentWithAssetId(const AZ::EntityId& targetEntity, const AZ::Uuid& meshAssetId)
    {
        return AddMeshComponentWithMesh(targetEntity, meshAssetId);
    }
} // namespace LmbrCentral

AZ_DECLARE_MODULE_CLASS(Gem_LmbrCentralEditor, LmbrCentral::LmbrCentralEditorModule)
