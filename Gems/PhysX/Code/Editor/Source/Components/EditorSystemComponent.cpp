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
#include <System/PhysXSystem.h>

namespace PhysX
{
    void EditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorJointLimitConfig::Reflect(context);
        EditorJointLimitPairConfig::Reflect(context);
        EditorJointLimitConeConfig::Reflect(context);
        EditorJointConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSystemComponent, AZ::Component>()
                ->Version(1);
        }
    }

    void EditorSystemComponent::Activate()
    {
        Physics::EditorWorldBus::Handler::BusConnect();
        AzToolsFramework::EditorContextMenuBus::Handler::BusConnect();

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
