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
#include <AzFramework/AzFrameworkModule.h>

// Component includes
#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzFramework/Asset/CustomAssetTypeComponent.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Components/NonUniformScaleComponent.h>
#include <AzFramework/Components/AzFrameworkConfigurationSystemComponent.h>
#include <AzFramework/Driller/RemoteDrillerInterface.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <AzFramework/FileTag/FileTagComponent.h>
#include <AzFramework/Input/System/InputSystemComponent.h>
#include <AzFramework/Network/NetBindingComponent.h>
#include <AzFramework/Network/NetBindingSystemComponent.h>
#include <AzFramework/Render/GameIntersectorComponent.h>
#include <AzFramework/Scene/SceneSystemComponent.h>
#include <AzFramework/Script/ScriptComponent.h>
#include <AzFramework/Script/ScriptRemoteDebugging.h>
#include <AzFramework/StreamingInstall/StreamingInstall.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>
#include <AzFramework/Visibility/OctreeSystemComponent.h>

namespace AzFramework
{
    AzFrameworkModule::AzFrameworkModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            AzFramework::AssetCatalogComponent::CreateDescriptor(),
            AzFramework::CustomAssetTypeComponent::CreateDescriptor(),
            AzFramework::FileTag::ExcludeFileComponent::CreateDescriptor(),
            AzFramework::NetBindingComponent::CreateDescriptor(),
            AzFramework::NetBindingSystemComponent::CreateDescriptor(),
            AzFramework::TransformComponent::CreateDescriptor(),
            AzFramework::NonUniformScaleComponent::CreateDescriptor(),
            AzFramework::GameEntityContextComponent::CreateDescriptor(),
            AzFramework::RenderGeometry::GameIntersectorComponent::CreateDescriptor(),
    #if !defined(_RELEASE)
            AzFramework::TargetManagementComponent::CreateDescriptor(),
    #endif
            AzFramework::CreateScriptDebugAgentFactory(),
            AzFramework::AssetSystem::AssetSystemComponent::CreateDescriptor(),
            AzFramework::InputSystemComponent::CreateDescriptor(),
            AzFramework::DrillerNetworkAgentComponent::CreateDescriptor(),

    #if !defined(AZCORE_EXCLUDE_LUA)
            AzFramework::ScriptComponent::CreateDescriptor(),
    #endif
            AzFramework::SceneSystemComponent::CreateDescriptor(),
            AzFramework::StreamingInstall::StreamingInstallSystemComponent::CreateDescriptor(),
            AzFramework::AzFrameworkConfigurationSystemComponent::CreateDescriptor(),

            AzFramework::OctreeSystemComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList AzFrameworkModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList
        {
            azrtti_typeid<AzFramework::OctreeSystemComponent>(),
        };
    }
}
