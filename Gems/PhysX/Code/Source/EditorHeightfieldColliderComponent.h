/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Editor/DebugDraw.h>

#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>
#include <AzFramework/Physics/HeightfieldProviderBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>

#include <PhysX/ColliderShapeBus.h>

namespace PhysX
{
    //! Editor PhysX Heightfield Collider Component.
    class EditorHeightfieldColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , protected AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , protected DebugDraw::DisplayCallback
        , protected AzPhysics::SimulatedBodyComponentRequestsBus::Handler
        , protected PhysX::ColliderShapeRequestBus::Handler
        , protected Physics::HeightfieldProviderNotificationBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(
            EditorHeightfieldColliderComponent,
            "{C388C3DB-8D2E-4D26-96D3-198EDC799B77}",
            AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        EditorHeightfieldColliderComponent();
        ~EditorHeightfieldColliderComponent();

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    protected:

        // AzToolsFramework::EntitySelectionEvents
        void OnSelected() override;
        void OnDeselected() override;

        // DisplayCallback
        void Display(AzFramework::DebugDisplayRequests& debugDisplay) const;

        // ColliderShapeRequestBus
        AZ::Aabb GetColliderShapeAabb() override;
        bool IsTrigger() override
        {
            // PhysX Heightfields don't support triggers.
            return false;
        }

        // AzPhysics::SimulatedBodyComponentRequestsBus::Handler overrides ...
        void EnablePhysics() override;
        void DisablePhysics() override;
        bool IsPhysicsEnabled() const override;
        AZ::Aabb GetAabb() const override;
        AzPhysics::SimulatedBody* GetSimulatedBody() override;
        AzPhysics::SimulatedBodyHandle GetSimulatedBodyHandle() const override;
        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;

        // Physics::HeightfieldProviderNotificationBus
        void OnHeightfieldDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion) override;

    private:
        AZ::u32 OnConfigurationChanged();

        void ClearHeightfield();
        void InitHeightfieldShapeConfiguration();
        void InitStaticRigidBody();
        void RefreshHeightfield();

        DebugDraw::Collider m_colliderDebugDraw; //!< Handles drawing the collider
        AzPhysics::SceneInterface* m_sceneInterface{ nullptr };

        AzPhysics::SystemEvents::OnConfigurationChangedEvent::Handler m_physXConfigChangedHandler;
        AzPhysics::SystemEvents::OnMaterialLibraryChangedEvent::Handler m_onMaterialLibraryChangedEventHandler;

        Physics::ColliderConfiguration m_colliderConfig; //!< Stores collision layers, whether the collider is a trigger, etc.
        AZStd::shared_ptr<Physics::HeightfieldShapeConfiguration> m_shapeConfig{ new Physics::HeightfieldShapeConfiguration() };

        AzPhysics::SimulatedBodyHandle m_staticRigidBodyHandle =
            AzPhysics::InvalidSimulatedBodyHandle; //!< Handle to the body in the editor physics scene if there is no rigid body component.
        AzPhysics::SceneHandle m_attachedSceneHandle = AzPhysics::InvalidSceneHandle;
    };

} // namespace PhysX
