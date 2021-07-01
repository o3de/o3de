/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "WhiteBoxColliderConfiguration.h"

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <WhiteBox/EditorWhiteBoxColliderBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace AzPhysics
{
    class SceneInterface;
}

namespace WhiteBox
{
    //! Generates physics from white box mesh.
    class EditorWhiteBoxColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private EditorWhiteBoxColliderRequestBus::Handler
        , private AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(
            EditorWhiteBoxColliderComponent, "{4EF53472-6ED4-4740-B956-F6AE5B4A4BB1}", EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        EditorWhiteBoxColliderComponent() = default;
        EditorWhiteBoxColliderComponent(const EditorWhiteBoxColliderComponent&) = delete;
        EditorWhiteBoxColliderComponent& operator=(const EditorWhiteBoxColliderComponent&) = delete;

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        // AZ::Component ...
        void Activate() override;
        void Deactivate() override;

        // EditorComponentBase ...
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // TransformBus ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // EditorWhiteBoxColliderRequestBus ...
        void CreatePhysics(const WhiteBoxMesh& whiteBox) override;
        void DestroyPhysics() override;

        void ConvertToPhysicsMesh(const WhiteBoxMesh& whiteBox);

        AzPhysics::SceneInterface* m_sceneInterface = nullptr;
        AzPhysics::SceneHandle m_editorSceneHandle = AzPhysics::InvalidSceneHandle;

        Physics::ColliderConfiguration
            m_physicsColliderConfiguration; //!< General physics collider configuration information.
        Physics::CookedMeshShapeConfiguration m_meshShapeConfiguration; //!< The physics representation of the mesh.
        AzPhysics::SimulatedBodyHandle m_rigidBodyHandle = AzPhysics::InvalidSimulatedBodyHandle; //!< Handle to a static rigid body to represent the White Box Mesh at edit time.
        WhiteBoxColliderConfiguration
            m_whiteBoxColliderConfiguration; //!< White Box specific collider configuration information.
    };
} // namespace WhiteBox
