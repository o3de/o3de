/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <Editor/EditorJointConfiguration.h>
#include <Source/Articulation/ArticulationLinkConfiguration.h>
#include <Source/EditorRigidBodyComponent.h>

namespace PhysX
{
    //! Feature flag for work in progress on PhysX reduced co-ordinate articulations (see https://github.com/o3de/sig-simulation/issues/60).
    constexpr AZStd::string_view ReducedCoordinateArticulationsFlag = "/Amazon/Physics/EnableReducedCoordinateArticulations";

    //! Helper function for checking whether feature flag for in progress PhysX reduced co-ordinate articulations work is enabled.
    //! See https://github.com/o3de/sig-simulation/issues/60 for more details.
    inline bool ReducedCoordinateArticulationsEnabled()
    {
        bool reducedCoordinateArticulationsEnabled = false;

        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(reducedCoordinateArticulationsEnabled, ReducedCoordinateArticulationsFlag);
        }

        return reducedCoordinateArticulationsEnabled;
    }

    class EditorArticulationLinkComponent;

    //! Configuration data for EditorRigidBodyComponent.
    struct EditorArticulationLinkConfiguration
        : public ArticulationLinkConfiguration
    {
        AZ_CLASS_ALLOCATOR(EditorArticulationLinkConfiguration, AZ::SystemAllocator);
        AZ_RTTI(
            EditorArticulationLinkConfiguration,
            "{8FFA0EC2-E850-4562-AB3D-08D157E07B81}",
            ArticulationLinkConfiguration);

        static void Reflect(AZ::ReflectContext* context);
    };

    //! Class for in-editor PhysX Articulation Link Component.
    class EditorArticulationLinkComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , protected AZ::TransformNotificationBus::Handler
        , protected AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(
            EditorArticulationLinkComponent, "{7D23169B-3214-4A32-ABFC-FCCE6E31F2CF}", AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        EditorArticulationLinkComponent() = default;
        explicit EditorArticulationLinkComponent(const EditorArticulationLinkConfiguration& configuration);
        ~EditorArticulationLinkComponent() = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // EditorComponentBase overrides ...
        void Activate() override;
        void Deactivate() override;

        void BuildGameEntity(AZ::Entity* gameEntity) override;

        bool IsRootArticulation() const;

    private:
        // TransformNotificationBus overrides ...
        void OnTransformChanged(const AZ::Transform& localTM, const AZ::Transform& worldTM) override;

        // AzFramework::EntityDebugDisplayEventBus overrides ...
        void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        bool ShowSetupDisplay() const;
        void ShowJointHierarchy(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) const;
        void ShowHingeJoint(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) const;
        void ShowPrismaticJoint(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) const;

        EditorArticulationLinkConfiguration m_config;

        AZ::Transform m_cachedWorldTM;
    };
} // namespace PhysX
