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

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Physics/SystemBus.h>
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

        // AztoolsFramework::EditorEvents::Bus::Handler
        void PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags) override;
        void NotifyRegisterViews() override;

        AZ::Data::AssetId GenerateSurfaceTypesLibrary();

        AzPhysics::SceneHandle m_editorWorldSceneHandle = AzPhysics::InvalidSceneHandle;
    };
}
