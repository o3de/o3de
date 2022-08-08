/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorSystemComponent.h"
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>

#include <IEditor.h>

#include <Editor/EditorJointConfiguration.h>
#include <Editor/EditorWindow.h>
#include <Editor/PropertyTypes.h>
#include <Editor/Source/Material/PhysXEditorMaterialAsset.h>
#include <Editor/Source/Material/Conversion/LegacyPhysicsMaterialLibraryConversion.h>
#include <System/PhysXSystem.h>

namespace PhysX
{
    void EditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorJointLimitConfig::Reflect(context);
        EditorJointLimitPairConfig::Reflect(context);
        EditorJointLimitConeConfig::Reflect(context);
        EditorJointConfig::Reflect(context);

        EditorMaterialAsset::Reflect(context);
        ReflectLegacyMaterialClasses(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSystemComponent, AZ::Component>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("AssetBuilder") }))
                ;
        }
    }

    void EditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsEditorService"));
    }

    void EditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysicsEditorService"));
    }

    void EditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("PhysicsService"));
        required.push_back(AZ_CRC_CE("PhysicsMaterialService"));
    }

    void EditorSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("AssetDatabaseService"));
        dependent.push_back(AZ_CRC_CE("AssetCatalogService"));
    }

    void EditorSystemComponent::Activate()
    {
        Physics::EditorWorldBus::Handler::BusConnect();
        AzToolsFramework::EditorContextMenuBus::Handler::BusConnect();

        // Register PhysX Material Asset
        auto* materialAsset = aznew AzFramework::GenericAssetHandler<PhysX::EditorMaterialAsset>("PhysX Material", Physics::MaterialAsset::AssetGroup, EditorMaterialAsset::FileExtension);
        materialAsset->Register();
        m_assetHandlers.emplace_back(materialAsset);

        // Register PhysX Material Asset Builder
        AssetBuilderSDK::AssetBuilderDesc materialAssetBuilderDescriptor;
        materialAssetBuilderDescriptor.m_name = "PhysX Material Asset Builder";
        materialAssetBuilderDescriptor.m_version = 1; // bump this to rebuild all physxmaterial files
        materialAssetBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern(AZStd::string::format("*.%s", EditorMaterialAsset::FileExtension), AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        materialAssetBuilderDescriptor.m_busId = azrtti_typeid<EditorMaterialAssetBuilder>();
        materialAssetBuilderDescriptor.m_createJobFunction = [this](const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
        {
            m_materialAssetBuilder.CreateJobs(request, response);
        };
        materialAssetBuilderDescriptor.m_processJobFunction = [this](const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
        {
            m_materialAssetBuilder.ProcessJob(request, response);
        };
        m_materialAssetBuilder.BusConnect(materialAssetBuilderDescriptor.m_busId);
        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, materialAssetBuilderDescriptor);

        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            AzPhysics::SceneConfiguration editorWorldConfiguration = physicsSystem->GetDefaultSceneConfiguration();
            editorWorldConfiguration.m_sceneName = AzPhysics::EditorPhysicsSceneName;
            m_editorWorldSceneHandle = physicsSystem->AddScene(editorWorldConfiguration);
        }

        PhysX::Editor::RegisterPropertyTypes();

        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
    }

    void EditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        AzToolsFramework::EditorContextMenuBus::Handler::BusDisconnect();
        Physics::EditorWorldBus::Handler::BusDisconnect();

        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->RemoveScene(m_editorWorldSceneHandle);
        }
        m_editorWorldSceneHandle = AzPhysics::InvalidSceneHandle;

        m_materialAssetBuilder.BusDisconnect();

        m_assetHandlers.clear();
    }

    AzPhysics::SceneHandle EditorSystemComponent::GetEditorSceneHandle() const
    {
        return m_editorWorldSceneHandle;
    }

    void EditorSystemComponent::OnStartPlayInEditorBegin()
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            if (AzPhysics::Scene* scene = physicsSystem->GetScene(m_editorWorldSceneHandle))
            {
                scene->SetEnabled(false);
            }
        }
    }

    void EditorSystemComponent::OnStopPlayInEditor()
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            if (AzPhysics::Scene* scene = physicsSystem->GetScene(m_editorWorldSceneHandle))
            {
                scene->SetEnabled(true);
            }
        }
    }

    void EditorSystemComponent::PopulateEditorGlobalContextMenu([[maybe_unused]] QMenu* menu, [[maybe_unused]] const AZ::Vector2& point, [[maybe_unused]] int flags)
    {

    }

    void EditorSystemComponent::NotifyRegisterViews()
    {
        PhysX::Editor::EditorWindow::RegisterViewClass();
    }
}
