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

#include <AzToolsFramework/ToolsComponents/AzToolsFrameworkConfigurationSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Scene/SceneSystemBus.h>

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
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
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
        // Associate the EditorEntityContext with the default scene.
        AzFramework::EntityContextId editorEntityContextId;
        EditorEntityContextRequestBus::BroadcastResult(editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

        AzFramework::Scene* defaultScene = nullptr;
        AzFramework::SceneSystemRequestBus::BroadcastResult(defaultScene, &AzFramework::SceneSystemRequests::GetScene, "default");

        if (!editorEntityContextId.IsNull() && defaultScene)
        {
            bool success = false;
            AzFramework::SceneSystemRequestBus::BroadcastResult(success, &AzFramework::SceneSystemRequests::SetSceneForEntityContextId, editorEntityContextId, defaultScene);
        }
    }

    void AzToolsFrameworkConfigurationSystemComponent::Deactivate()
    {
    }

    void AzToolsFrameworkConfigurationSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("AzToolsFrameworkConfigurationSystemComponentService", 0xfc4f9667));
    }

    void AzToolsFrameworkConfigurationSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("AzToolsFrameworkConfigurationSystemComponentService", 0xfc4f9667));
    }

    void AzToolsFrameworkConfigurationSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("AzFrameworkConfigurationSystemComponentService", 0xcc49c96e));
        dependent.push_back(AZ_CRC("SceneSystemComponentService", 0xd8975435));
        dependent.push_back(AZ_CRC("EditorEntityContextService", 0x28d93a43));
    }

} // AzToolsFramework

