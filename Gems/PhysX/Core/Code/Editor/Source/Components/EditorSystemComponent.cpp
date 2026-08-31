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

#include <Editor/ColliderComponentMode.h>
#include <Editor/EditorJointConfiguration.h>
#include <Editor/EditorWindow.h>
#include <Editor/PropertyTypes.h>
#include <Editor/Source/ComponentModes/Joints/JointsComponentMode.h>
#include <Editor/Source/Material/PhysXEditorMaterialAsset.h>
#include <Pipeline/PhysicsPrefabProcessor.h>
#include <System/PhysXSystem.h>

namespace PhysX
{
    void EditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        ColliderComponentMode::Reflect(context);
        EditorJointLimitConfig::Reflect(context);
        EditorJointLimitPairConfig::Reflect(context);
        EditorJointLimitLinearPairConfig::Reflect(context);
        EditorJointLimitConeConfig::Reflect(context);
        EditorJointConfig::Reflect(context);
        JointsComponentMode::Reflect(context);

        EditorMaterialAsset::Reflect(context);

        PhysicsPrefabProcessor::Reflect(context);

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
    }

    void EditorSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("AssetDatabaseService"));
        dependent.push_back(AZ_CRC_CE("AssetCatalogService"));
        dependent.push_back(AZ_CRC_CE("PhysicsMaterialService"));
    }

    void EditorSystemComponent::Activate()
    {
        Physics::EditorWorldBus::Handler::BusConnect();

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

        PhysX::Editor::RegisterPropertyTypes();

        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    }

    void EditorSystemComponent::Deactivate()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        Physics::EditorWorldBus::Handler::BusDisconnect();

        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            for (const auto& [worldId, sceneHandle] : m_editorWorldScenes)
            {
                physicsSystem->RemoveScene(sceneHandle);
            }
        }
        m_editorWorldScenes.clear();

        m_materialAssetBuilder.BusDisconnect();

        for (auto& assetHandler : m_assetHandlers)
        {
            if (auto editorMaterialAssetHandler = azrtti_cast<AzFramework::GenericAssetHandler<PhysX::EditorMaterialAsset>*>(assetHandler.get());
                editorMaterialAssetHandler != nullptr)
            {
                editorMaterialAssetHandler->Unregister();
            }
        }
        m_assetHandlers.clear();
    }

    AzPhysics::SceneHandle EditorSystemComponent::GetEditorSceneHandle(AZ::EntityId entityId) const
    {
        AzFramework::EntityContextId worldId = AzFramework::EntityContextId::CreateNull();
        if (entityId.IsValid())
        {
            AzFramework::EntityIdContextQueryBus::EventResult(
                worldId, entityId, &AzFramework::EntityIdContextQueries::GetOwningContextId);
        }
        if (worldId.IsNull())
        {
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                worldId, &AzToolsFramework::EditorEntityContextRequests::GetActiveWorldId);
        }
        if (worldId.IsNull())
        {
            return AzPhysics::InvalidSceneHandle;
        }

        if (auto sceneIt = m_editorWorldScenes.find(worldId); sceneIt != m_editorWorldScenes.end())
        {
            return sceneIt->second;
        }

        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        if (!physicsSystem)
        {
            return AzPhysics::InvalidSceneHandle;
        }

        AzPhysics::SceneConfiguration editorWorldConfiguration = physicsSystem->GetDefaultSceneConfiguration();
        editorWorldConfiguration.m_sceneName =
            AZStd::string::format("%s %s", AzPhysics::EditorPhysicsSceneName, worldId.ToFixedString().c_str());

        const AzPhysics::SceneHandle sceneHandle = physicsSystem->AddScene(editorWorldConfiguration);
        m_editorWorldScenes.emplace(worldId, sceneHandle);
        return sceneHandle;
    }

    void EditorSystemComponent::OnWorldDestroyed(const AzFramework::EntityContextId& worldId)
    {
        auto sceneIt = m_editorWorldScenes.find(worldId);
        if (sceneIt == m_editorWorldScenes.end())
        {
            return;
        }

        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->RemoveScene(sceneIt->second);
        }
        m_editorWorldScenes.erase(sceneIt);
    }

    void EditorSystemComponent::SetEditorScenesEnabled(bool enabled)
    {
        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        if (!physicsSystem)
        {
            return;
        }

        for (const auto& [worldId, sceneHandle] : m_editorWorldScenes)
        {
            if (AzPhysics::Scene* scene = physicsSystem->GetScene(sceneHandle))
            {
                scene->SetEnabled(enabled);
            }
        }
    }

    void EditorSystemComponent::OnActionRegistrationHook()
    {
        ColliderComponentMode::RegisterActions();
        JointsComponentMode::RegisterActions();
    }

    void EditorSystemComponent::OnActionContextModeBindingHook()
    {
        ColliderComponentMode::BindActionsToModes();
        JointsComponentMode::BindActionsToModes();
    }

    void EditorSystemComponent::OnMenuBindingHook()
    {
        ColliderComponentMode::BindActionsToMenus();
        JointsComponentMode::BindActionsToMenus();
    }

    void EditorSystemComponent::OnStartPlayInEditorBegin()
    {
        SetEditorScenesEnabled(false);
    }

    void EditorSystemComponent::OnStopPlayInEditor()
    {
        SetEditorScenesEnabled(true);
    }


    void EditorSystemComponent::NotifyRegisterViews()
    {
        PhysX::Editor::EditorWindow::RegisterViewClass();
    }
}
