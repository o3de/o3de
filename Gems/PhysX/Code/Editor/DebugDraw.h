/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Shape.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <PhysX/MeshAsset.h>
#include <PhysX/Debug/PhysXDebugConfiguration.h>
#include <PhysX/Debug/PhysXDebugInterface.h>

namespace AZ
{
    class ReflectContext;
}

namespace physx
{
    class PxBase;
}

namespace PhysX
{
    namespace DebugDraw
    {
        //! Open the PhysX Settings Window on the Global Settings tab.
        void OpenPhysXSettingsWindow();

        //! Determine if the global debug draw preference is set to the specified state.
        //! @param requiredState The collider debug state to check against the global state.
        //! @return True if global collider debug state matches the input requiredState.
        bool IsGlobalColliderDebugCheck(Debug::DebugDisplayData::GlobalCollisionDebugState requiredState);

        class DisplayCallback
        {
        public:
            virtual void Display(const AzFramework::ViewportInfo& viewportInfo,
                AzFramework::DebugDisplayRequests& debugDisplay) const = 0;
        protected:
            ~DisplayCallback() = default;
        };

        class Collider
            : protected AzFramework::EntityDebugDisplayEventBus::Handler
            , protected AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Handler
            , protected AzToolsFramework::EntitySelectionEvents::Bus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(Collider, AZ::SystemAllocator);
            AZ_RTTI(Collider, "{7DE9CA01-DF1E-4D72-BBF4-76C9136BE6A2}");
            static void Reflect(AZ::ReflectContext* context);

            Collider();

            void Connect(AZ::EntityId entityId);
            void SetDisplayCallback(const DisplayCallback* callback);
            void Disconnect();

            bool HasCachedGeometry() const;
            void ClearCachedGeometry();

            void SetDisplayFlag(bool enable);
            bool IsDisplayFlagEnabled() const;

            void BuildMeshes(const Physics::ShapeConfiguration& shapeConfig, AZ::u32 geomIndex) const;

            struct ElementDebugInfo
            {
                // Note: this doesn't use member initializer because of a bug in Mac OS compiler
                ElementDebugInfo()
                    : m_materialSlotIndex(0)
                    , m_numTriangles(0)
                {}

                int m_materialSlotIndex;
                AZ::u32 m_numTriangles;
            };

            AZ::Color CalcDebugColor(const Physics::ColliderConfiguration& colliderConfig
                , const ElementDebugInfo& elementToDebugInfo = ElementDebugInfo()) const;

            AZ::Color CalcDebugColorWarning(const AZ::Color& baseColor, AZ::u32 triangleCount) const;

            void DrawSphere(AzFramework::DebugDisplayRequests& debugDisplay,
                const Physics::ColliderConfiguration& colliderConfig,
                const Physics::SphereShapeConfiguration& sphereShapeConfig,
                const AZ::Vector3& colliderScale = AZ::Vector3::CreateOne()) const;

            void DrawBox(
                AzFramework::DebugDisplayRequests& debugDisplay,
                const Physics::ColliderConfiguration& colliderConfig,
                const Physics::BoxShapeConfiguration& boxShapeConfig,
                const AZ::Vector3& colliderScale = AZ::Vector3::CreateOne()) const;

            void DrawCapsule(AzFramework::DebugDisplayRequests& debugDisplay,
                const Physics::ColliderConfiguration& colliderConfig,
                const Physics::CapsuleShapeConfiguration& capsuleShapeConfig,
                const AZ::Vector3& colliderScale = AZ::Vector3::CreateOne()) const;

            void DrawMesh(AzFramework::DebugDisplayRequests& debugDisplay,
                const Physics::ColliderConfiguration& colliderConfig,
                const Physics::CookedMeshShapeConfiguration& assetConfig,
                const AZ::Vector3& meshScale,
                AZ::u32 geomIndex) const;

            void DrawHeightfield(
                AzFramework::DebugDisplayRequests& debugDisplay,
                const AZ::Vector3& aabbCenterLocalBody,
                float drawDistance,
                const AZStd::shared_ptr<const Physics::Shape>& shape) const;

            void DrawPolygonPrism(
                AzFramework::DebugDisplayRequests& debugDisplay,
                const Physics::ColliderConfiguration& colliderConfig, const AZStd::vector<AZ::Vector3>& points) const;

            AZ::Transform GetColliderLocalTransform(const Physics::ColliderConfiguration& colliderConfig,
                const AZ::Vector3& colliderScale = AZ::Vector3::CreateOne()) const;

            AZ::u32 GetNumShapes() const;
            const AZStd::vector<AZ::Vector3>& GetVerts(AZ::u32 geomIndex) const;
            const AZStd::vector<AZ::Vector3>& GetPoints(AZ::u32 geomIndex) const;
            const AZStd::vector<AZ::u32>& GetIndices(AZ::u32 geomIndex) const;

        protected:
            // AzFramework::EntityDebugDisplayEventBus overrides ...
            void DisplayEntityViewport(
                const AzFramework::ViewportInfo& viewportInfo,
                AzFramework::DebugDisplayRequests& debugDisplay) override;

            // AzToolsFramework::ViewportInteraction::ViewportSettingsNotificationBus::Handler overrides ...
            void OnDrawHelpersChanged(bool enabled) override;

            // AzToolsFramework::EntitySelectionEvents::Bus::Handler overrides ...
            void OnSelected() override;
            void OnDeselected() override;

            void RefreshTreeHelper();

            // Internal mesh drawing subroutines
            void DrawTriangleMesh(
                AzFramework::DebugDisplayRequests& debugDisplay, const Physics::ColliderConfiguration& colliderConfig, AZ::u32 geomIndex,
                const AZ::Vector3& meshScale = AZ::Vector3::CreateOne()) const;

            void DrawConvexMesh(
                AzFramework::DebugDisplayRequests& debugDisplay, const Physics::ColliderConfiguration& colliderConfig, AZ::u32 geomIndex,
                const AZ::Vector3& meshScale = AZ::Vector3::CreateOne()) const;

            void BuildTriangleMesh(physx::PxBase* meshData, AZ::u32 geomIndex) const;

            void BuildConvexMesh(physx::PxBase* meshData, AZ::u32 geomIndex) const;

            AZStd::string GetEntityName() const;

            bool m_locallyEnabled = true; //!< Local setting to enable displaying the collider in editor view.
            AZ::EntityId m_entityId;
            const DisplayCallback* m_displayCallback = nullptr;

            struct GeometryData
            {
                AZStd::unordered_map<int, AZStd::vector<AZ::u32>> m_triangleIndexesByMaterialSlot;
                AZStd::vector<AZ::Vector3> m_verts;
                AZStd::vector<AZ::Vector3> m_points;
                AZStd::vector<AZ::u32> m_indices;
            };

            mutable AZStd::vector<GeometryData> m_geometry;

            PhysX::Debug::DebugDisplayDataChangedEvent::Handler m_debugDisplayDataChangedEvent;
        };
    } // namespace DebugDraw
} // namespace PhysX
