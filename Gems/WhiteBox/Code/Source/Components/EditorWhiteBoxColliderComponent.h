/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "WhiteBoxColliderConfiguration.h"
#include "WhiteBoxConvexDecomposition.h"

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
        //! @param voxelComponent non-null to use its greedy voxel collider for stamped cubes.
        //! @param convex cook one convex hull instead of a triangle mesh.
        //! @param keepFraction below one, simplify the triangle mesh to about that share.
        static bool CookToConfiguration(
            const WhiteBoxMesh& whiteBox, Physics::CookedMeshShapeConfiguration& outConfiguration,
            EditorWhiteBoxComponent* voxelComponent = nullptr, bool convex = false, float keepFraction = 1.0f);
        //! Share of triangles to keep: the Resolution for Triangle Mesh (Simplified), otherwise all of them.
        float KeepFraction() const;
        //! One hull for the whole mesh: chosen, or forced because PhysX triggers cannot be triangle meshes.
        bool UseConvex() const;
        //! One hull per convex part (Convex Parts); these go in the hull lists, not the single configurations.
        bool UseConvexParts() const;
        //! Decompose @p whiteBox and cook every part; false when nothing could be cooked.
        //! In the background, unfinished concave shells stand in as one hull each and the collider rebuilds when they land.
        bool CookConvexParts(
            const WhiteBoxMesh& whiteBox, AZStd::vector<Physics::CookedMeshShapeConfiguration>& outParts,
            DecomposeWait wait = DecomposeWait::Background);
        //! Every part's hull, appended into one triangle list for the wireframe.
        void AppendPartsDebugMesh(
            const AZStd::vector<Physics::CookedMeshShapeConfiguration>& parts, AZStd::vector<AZ::Vector3>& vertices,
            AZStd::vector<AZ::u32>& indices) const;
        //! The hull PhysX actually built, as a triangle list for the wireframe; false when it cannot be read back.
        bool ConvexDebugMesh(
            const Physics::CookedMeshShapeConfiguration& configuration, AZStd::vector<AZ::Vector3>& vertices,
            AZStd::vector<AZ::u32>& indices) const;
        //! Re-cook when the trigger flag or collision shape changes in the Inspector.
        AZ::Crc32 OnColliderSettingsChanged();
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
        AZStd::vector<Physics::CookedMeshShapeConfiguration> m_partShapeConfigurations; //!< Cooked Convex Parts of the base mesh.
        AZStd::vector<Physics::CookedMeshShapeConfiguration> m_booleanPartShapeConfigurations; //!< Cooked Convex Parts of the boolean mesh.
        AzPhysics::SimulatedBodyHandle m_rigidBodyHandle = AzPhysics::InvalidSimulatedBodyHandle; //!< Handle to a static rigid body to represent the White Box Mesh at edit time.
        WhiteBoxColliderConfiguration
            m_whiteBoxColliderConfiguration; //!< White Box specific collider configuration information.

        bool m_drawCollider = false; //!< Draw the actual collision mesh (the geometry that is cooked) as a wireframe (opt-in).
        float m_editorBuiltScale = 1.0f; //!< Uniform scale the current edit-time body was built with.
        AZStd::vector<AZ::Vector3> m_debugVertices; //!< Local-space vertices of the currently cooked collision mesh.
        AZStd::vector<AZ::u32> m_debugIndices; //!< Triangle indices into m_debugVertices for the wireframe overlay.
    };
} // namespace WhiteBox
