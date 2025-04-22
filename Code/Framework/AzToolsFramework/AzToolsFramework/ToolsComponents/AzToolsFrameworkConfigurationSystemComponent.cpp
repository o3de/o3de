/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ToolsComponents/AzToolsFrameworkConfigurationSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemInterface.h>

#include <AzToolsFramework/Editor/EditorSettingsAPIBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>

namespace AzToolsFramework
{
    void AzToolsFrameworkConfigurationSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AzToolsFrameworkConfigurationSystemComponent, AZ::Component>();

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AzToolsFrameworkConfigurationSystemComponent>(
                    "AzToolsFramework Configuration Component", "System component responsible for configuring AzToolsFramework")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<EditorSettingsAPIBus>("EditorSettingsAPIBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "editor")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("BuildSettingsList", &EditorSettingsAPIRequests::BuildSettingsList)
                ->Event("GetValue", &EditorSettingsAPIRequests::GetValue)
                ->Event("SetValue", &EditorSettingsAPIRequests::SetValue)
                ;
        }
    }

    void AzToolsFrameworkConfigurationSystemComponent::Activate()
    {
        // Create the editor specific child scene to the main scene and add the editor entity context to it.
        AzFramework::EntityContextId editorEntityContextId;
        EditorEntityContextRequestBus::BroadcastResult(editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

        if (!editorEntityContextId.IsNull())
        {
            auto sceneSystem = AzFramework::SceneSystemInterface::Get();
            AZ_Assert(sceneSystem, "Scene system not available to create the editor scene.");
            AZStd::shared_ptr<AzFramework::Scene> mainScene = sceneSystem->GetScene(AzFramework::Scene::MainSceneName);
            if (mainScene)
            {
                AZ::Outcome<AZStd::shared_ptr<AzFramework::Scene>, AZStd::string> editorScene =
                    sceneSystem->CreateSceneWithParent(AzFramework::Scene::EditorMainSceneName, mainScene);
                if (editorScene.IsSuccess())
                {
                    AzFramework::EntityContext* editorEntityContext = nullptr;
                    EditorEntityContextRequestBus::BroadcastResult(
                        editorEntityContext, &EditorEntityContextRequests::GetEditorEntityContextInstance);
                    if (editorEntityContext != nullptr)
                    {
                        [[maybe_unused]] bool contextAdded =
                            editorScene.GetValue()->SetSubsystem<AzFramework::EntityContext::SceneStorageType&>(editorEntityContext);
                        AZ_Assert(contextAdded, "Unable to add editor entity context to scene.");
                    }
                }
                else
                {
                    AZ_Assert(false, "Failed to create editor scene because: %s", editorScene.GetError().c_str());
                }
            }
            else
            {
                AZ_Assert(false, "No main scene to parent the editor scene under.");
            }
        }
    }

    void AzToolsFrameworkConfigurationSystemComponent::Deactivate()
    {
        [[maybe_unused]] bool success = AzFramework::SceneSystemInterface::Get()->RemoveScene(AzFramework::Scene::EditorMainSceneName);
        AZ_Assert(success, "Unable to remove the main editor scene.");
    }

    void AzToolsFrameworkConfigurationSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AzToolsFrameworkConfigurationSystemComponentService"));
    }

    void AzToolsFrameworkConfigurationSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AzToolsFrameworkConfigurationSystemComponentService"));
    }

    void AzToolsFrameworkConfigurationSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("AzFrameworkConfigurationSystemComponentService"));
        dependent.push_back(AZ_CRC_CE("SceneSystemComponentService"));
        dependent.push_back(AZ_CRC_CE("EditorEntityContextService"));
    }

} // AzToolsFramework

