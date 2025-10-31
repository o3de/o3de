/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <Editor/Source/Material/PhysXEditorMaterialAssetBuilder.h>
#include <EditorPhysXJointInterface.h>

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
        , public AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(EditorSystemComponent, "{560F08DC-94F5-4D29-9AD4-CDFB3B57C654}");
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        EditorSystemComponent() = default;
        EditorSystemComponent(const EditorSystemComponent&) = delete;
        EditorSystemComponent& operator=(const EditorSystemComponent&) = delete;

        // AZ::Component overrides...
        void Activate() override;
        void Deactivate() override;

        // Physics::EditorWorldBus overrides...
        AzPhysics::SceneHandle GetEditorSceneHandle() const override;

        // ActionManagerRegistrationNotificationBus overrides ...
        void OnActionRegistrationHook() override;
        void OnActionContextModeBindingHook() override;
        void OnMenuBindingHook() override;

    private:
        // AzToolsFramework::EditorEntityContextNotificationBus overrides...
        void OnStartPlayInEditorBegin() override;
        void OnStopPlayInEditor() override;

        // AztoolsFramework::EditorEvents overrides...
        void NotifyRegisterViews() override;

        AzPhysics::SceneHandle m_editorWorldSceneHandle = AzPhysics::InvalidSceneHandle;

        // Assets related data
        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;

        // Asset builder for PhysX material asset
        EditorMaterialAssetBuilder m_materialAssetBuilder;

        PhysXEditorJointHelpersInterface m_editorJointHelpersInterface;
    };
}
