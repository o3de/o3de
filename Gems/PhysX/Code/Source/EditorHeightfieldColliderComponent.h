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

#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>

#include <Source/HeightfieldCollider.h>

namespace PhysX
{
    //! Editor PhysX Heightfield Collider Component.
    class EditorHeightfieldColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , protected AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , protected DebugDraw::DisplayCallback
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

        void BlockOnPendingJobs();
    protected:

        // AzToolsFramework::EntitySelectionEvents
        void OnSelected() override;
        void OnDeselected() override;

        // DisplayCallback
        void Display(const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) const;

    private:
        AZ::u32 OnConfigurationChanged();

        DebugDraw::Collider m_colliderDebugDraw; //!< Handles drawing the collider

        AzPhysics::SystemEvents::OnConfigurationChangedEvent::Handler m_physXConfigChangedHandler;

        //! Stores collision layers, whether the collider is a trigger, etc.
        AZStd::shared_ptr<Physics::ColliderConfiguration> m_colliderConfig{ aznew Physics::ColliderConfiguration()  };
        //! Stores all of the cached information for the heightfield shape.
        AZStd::shared_ptr<Physics::HeightfieldShapeConfiguration> m_shapeConfig{ aznew Physics::HeightfieldShapeConfiguration() };
        //! Contains all of the runtime logic for creating / updating / destroying the heightfield collider.
        AZStd::unique_ptr<HeightfieldCollider> m_heightfieldCollider;
    };

} // namespace PhysX
