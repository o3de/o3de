/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/EntityBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <PhysX/ColliderComponentBus.h>
#include <PhysX/Debug/PhysXDebugConfiguration.h>
#include <PhysX/Debug/PhysXDebugInterface.h>
#include <Source/RigidBody.h>

namespace PhysX
{
    //! Configuration data for EditorRigidBodyComponent.
    struct EditorRigidBodyConfiguration
        : public AzPhysics::RigidBodyConfiguration
    {
        AZ_CLASS_ALLOCATOR(PhysX::EditorRigidBodyConfiguration, AZ::SystemAllocator);
        AZ_RTTI(PhysX::EditorRigidBodyConfiguration, "{27297024-5A99-4C58-8614-4EF18137CE69}", AzPhysics::RigidBodyConfiguration);

        static void Reflect(AZ::ReflectContext* context);

        // Debug properties.
        bool m_centerOfMassDebugDraw = false;
    };

    //! Class for in-editor PhysX Dynamic Rigid Body Component.
    class EditorRigidBodyComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public AZ::EntityBus::Handler
        , protected AzFramework::EntityDebugDisplayEventBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private Physics::ColliderComponentEventBus::Handler
        , private AzPhysics::SimulatedBodyComponentRequestsBus::Handler
        , public AzFramework::BoundsRequestBus::Handler
        , public AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorRigidBodyComponent, "{F2478E6B-001A-4006-9D7E-DCB5A6B041DD}", AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        EditorRigidBodyComponent();
        explicit EditorRigidBodyComponent(const EditorRigidBodyConfiguration& configuration);
        EditorRigidBodyComponent(const EditorRigidBodyConfiguration& configuration, const RigidBodyConfiguration& physxSpecificConfiguration);
        ~EditorRigidBodyComponent() = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
            provided.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
            provided.push_back(AZ_CRC_CE("PhysicsDynamicRigidBodyService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("TransformService"));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC_CE("NonUniformScaleService"));
        }

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AZ::EntityBus overrides ...
        void OnEntityActivated(const AZ::EntityId& entityId) override;

        // BoundsRequestBus overrides ...
        AZ::Aabb GetWorldBounds() const override;
        AZ::Aabb GetLocalBounds() const override;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        const AzPhysics::RigidBody* GetRigidBody() const;

    private:
        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        // TransformBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // non-uniform scale handling
        void OnNonUniformScaleChanged(const AZ::Vector3& scale);

        // Physics::ColliderComponentEventBus
        void OnColliderChanged() override;

        // AzPhysics::SimulatedBodyComponentRequestsBus::Handler overrides ...
        void EnablePhysics() override;
        void DisablePhysics() override;
        bool IsPhysicsEnabled() const override;
        AZ::Aabb GetAabb() const override;
        AzPhysics::SimulatedBody* GetSimulatedBody() override;
        AzPhysics::SimulatedBodyHandle GetSimulatedBodyHandle() const override;
        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;

        // EditorComponentSelectionRequestsBus overrides ...
        AZ::Aabb GetEditorSelectionBoundsViewport(const AzFramework::ViewportInfo& viewportInfo) override;
        bool EditorSelectionIntersectRayViewport(
            const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) override;
        bool SupportsEditorRayIntersect() override;

        void CreateEditorWorldRigidBody();
        void UpdateDebugDrawSettings(const Debug::DebugDisplayData& data);

        void SetShouldBeRecreated();

        void InitPhysicsTickHandler();
        void PrePhysicsTick();

        void OnConfigurationChanged();

        Debug::DebugDisplayDataChangedEvent::Handler m_debugDisplayDataChangeHandler;

        EditorRigidBodyConfiguration m_config; //!< Generic properties from AzPhysics.
        RigidBodyConfiguration
            m_physxSpecificConfig; //!< Properties specific to PhysX which might not have exact equivalents in other physics engines.
        AzPhysics::SimulatedBodyHandle m_editorRigidBodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
        AzPhysics::SceneHandle m_editorSceneHandle = AzPhysics::InvalidSceneHandle;

        AZ::Color m_centerOfMassDebugColor = AZ::Colors::White;
        float m_centerOfMassDebugSize = 0.1f;
        bool m_shouldBeRecreated = false;

        AzPhysics::SceneEvents::OnSceneSimulationStartHandler m_sceneStartSimHandler;
        AZ::NonUniformScaleChangedEvent::Handler m_nonUniformScaleChangedHandler; //!< Responds to changes in non-uniform scale.
        AzPhysics::SystemEvents::OnDefaultSceneConfigurationChangedEvent::Handler m_sceneConfigChangedHandler; //!< Responds to changes in Scene Config.
    };
} // namespace PhysX
