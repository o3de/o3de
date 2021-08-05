/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzToolsFramework/Editor/EditorContextMenuBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace AzPhysics
{
    struct TriggerEvent;
}

namespace PhysX
{
    ///
    /// System component for PhysX editor
    ///
    class EditorSystemComponent
        : public AZ::Component
        , public Physics::EditorWorldBus::Handler
        , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private AzToolsFramework::EditorContextMenuBus::Handler
    {
    public:
        AZ_COMPONENT(EditorSystemComponent, "{560F08DC-94F5-4D29-9AD4-CDFB3B57C654}");
        static void Reflect(AZ::ReflectContext* context);

        EditorSystemComponent() = default;

        // Physics::EditorWorldBus
        AzPhysics::SceneHandle GetEditorSceneHandle() const override;

    private:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("PhysXEditorService", 0x0a61cda5));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("PhysXService", 0x75beae2d));
        }

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AzToolsFramework::EditorEntityContextNotificationBus
        void OnStartPlayInEditorBegin() override;
        void OnStopPlayInEditor() override;

        // AztoolsFramework::EditorContextMenuBus::Handler
        void PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags) override;

        // AztoolsFramework::EditorEvents::Bus::Handler
        void NotifyRegisterViews() override;

        AZStd::optional<AZ::Data::Asset<AZ::Data::AssetData>> RetrieveDefaultMaterialLibrary();

        AzPhysics::SystemEvents::OnMaterialLibraryLoadErrorEvent::Handler m_onMaterialLibraryLoadErrorEventHandler;
        AzPhysics::SceneHandle m_editorWorldSceneHandle = AzPhysics::InvalidSceneHandle;
    };
}
