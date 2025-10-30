/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/AzFrameworkModule.h>

// Component includes
#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzFramework/Asset/CustomAssetTypeComponent.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Components/NonUniformScaleComponent.h>
#include <AzFramework/Components/AzFrameworkConfigurationSystemComponent.h>
#include <AzFramework/Device/DeviceAttributesSystemComponent.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <AzFramework/FileTag/FileTagComponent.h>
#include <AzFramework/Input/Contexts/InputContextComponent.h>
#include <AzFramework/Input/System/InputSystemComponent.h>
#include <AzFramework/PaintBrush/PaintBrushSystemComponent.h>
#include <AzFramework/Quality/QualitySystemComponent.h>
#include <AzFramework/Render/GameIntersectorComponent.h>
#include <AzFramework/Scene/SceneSystemComponent.h>
#include <AzFramework/Script/ScriptComponent.h>
#include <AzFramework/Script/ScriptRemoteDebugging.h>
#include <AzFramework/Spawnable/SpawnableSystemComponent.h>
#include <AzFramework/Physics/Material/PhysicsMaterialSystemComponent.h>
#include <AzFramework/StreamingInstall/StreamingInstall.h>
#include <AzFramework/Visibility/OctreeSystemComponent.h>

AZ_DEFINE_BUDGET(AzFramework);

namespace AzFramework
{
    AzFrameworkModule::AzFrameworkModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            AzFramework::AssetCatalogComponent::CreateDescriptor(),
            AzFramework::CustomAssetTypeComponent::CreateDescriptor(),
            AzFramework::FileTag::ExcludeFileComponent::CreateDescriptor(),
            AzFramework::TransformComponent::CreateDescriptor(),
            AzFramework::NonUniformScaleComponent::CreateDescriptor(),
            AzFramework::GameEntityContextComponent::CreateDescriptor(),
            AzFramework::RenderGeometry::GameIntersectorComponent::CreateDescriptor(),
            AzFramework::CreateScriptDebugAgentFactory(),
            AzFramework::AssetSystem::AssetSystemComponent::CreateDescriptor(),
            AzFramework::InputSystemComponent::CreateDescriptor(),
            AzFramework::InputContextComponent::CreateDescriptor(),
            AzFramework::PaintBrushSystemComponent::CreateDescriptor(),

    #if !defined(AZCORE_EXCLUDE_LUA)
            AzFramework::ScriptComponent::CreateDescriptor(),
    #endif
            AzFramework::SceneSystemComponent::CreateDescriptor(),
            AzFramework::StreamingInstall::StreamingInstallSystemComponent::CreateDescriptor(),
            AzFramework::AzFrameworkConfigurationSystemComponent::CreateDescriptor(),
            AzFramework::QualitySystemComponent::CreateDescriptor(),
            AzFramework::DeviceAttributesSystemComponent::CreateDescriptor(),

            AzFramework::OctreeSystemComponent::CreateDescriptor(),
            AzFramework::SpawnableSystemComponent::CreateDescriptor(),
            Physics::MaterialSystemComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList AzFrameworkModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList
        {
            azrtti_typeid<AzFramework::OctreeSystemComponent>(),
            azrtti_typeid<AzFramework::QualitySystemComponent>(),
            azrtti_typeid<AzFramework::DeviceAttributesSystemComponent>(),
        };
    }
}
