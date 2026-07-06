/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "WhiteBoxColliderConfiguration.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
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
    class EditorWhiteBoxComponent;

    //! Generates physics from white box mesh.
    class EditorWhiteBoxColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private EditorWhiteBoxColliderRequestBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
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

        // EntityDebugDisplayEventBus ...
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        // EditorWhiteBoxColliderRequestBus ...
        void CreatePhysics(const WhiteBoxMesh& whiteBox) override;
        void DestroyPhysics() override;

        void ConvertToPhysicsMesh(const WhiteBoxMesh& whiteBox);
        //! Cook @p whiteBox into @p outConfiguration without touching the edit-time body.
        //! Used to pre-bake the base and boolean collider variants for the game entity.
        //! @param voxelComponent when non-null, a greedy-meshed collider is requested from it
        //! (used for the base voxel-stamped mesh); pass null to always cook the plain per-face
        //! triangulation (e.g. the boolean mesh, or a mesh with no stamped cubes).
        //! @return true if cooking succeeded.
        static bool CookToConfiguration(
            const WhiteBoxMesh& whiteBox, Physics::CookedMeshShapeConfiguration& outConfiguration,
            EditorWhiteBoxComponent* voxelComponent = nullptr);
        //! Cook the base and boolean variants at edit time, so BuildGameEntity only hands them over.
        void CookColliderVariants();

        AzPhysics::SceneInterface* m_sceneInterface = nullptr;
        AzPhysics::SceneHandle m_editorSceneHandle = AzPhysics::InvalidSceneHandle;

        Physics::ColliderConfiguration
            m_physicsColliderConfiguration; //!< General physics collider configuration information.
        Physics::CookedMeshShapeConfiguration m_meshShapeConfiguration; //!< Cooked base mesh.
        Physics::CookedMeshShapeConfiguration
            m_booleanMeshShapeConfiguration; //!< Cooked boolean-evaluated mesh (empty if no boolean source).
        bool m_hasBooleanMesh = false; //!< Whether a boolean-evaluated cooked mesh is available.
        AzPhysics::SimulatedBodyHandle m_rigidBodyHandle = AzPhysics::InvalidSimulatedBodyHandle; //!< Handle to a static rigid body to represent the White Box Mesh at edit time.
        WhiteBoxColliderConfiguration
            m_whiteBoxColliderConfiguration; //!< White Box specific collider configuration information.

        bool m_drawCollider = false; //!< Draw the actual collision mesh (the geometry that is cooked) as a wireframe (opt-in).
        float m_editorBuiltScale = 1.0f; //!< Uniform scale the current edit-time body was built with.
        AZStd::vector<AZ::Vector3> m_debugVertices; //!< Local-space vertices of the currently cooked collision mesh.
        AZStd::vector<AZ::u32> m_debugIndices; //!< Triangle indices into m_debugVertices for the wireframe overlay.
    };
} // namespace WhiteBox
