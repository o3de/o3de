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

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <PhysX/ComponentTypeIds.h>

#include <Source/ForceRegion.h>
#include <Source/ForceRegionForces.h>

namespace PhysX
{
    /// Editor PhysX Force Region Component.
    class EditorForceRegionComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorForceRegionComponent, EditorForceRegionComponentTypeId, AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        EditorForceRegionComponent() = default;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    protected:
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // EntityDebugDisplayEventBus
        void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo
            , AzFramework::DebugDisplayRequests& debugDisplayRequests) override;

    private:
        struct EditorForceProxy
        {
        public:
            AZ_CLASS_ALLOCATOR(EditorForceProxy, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(EditorForceProxy, "{26BB8392-6FE4-472E-B5D4-50BA952F1A39}");

            static void Reflect(AZ::ReflectContext* context);

            enum class ForceType : AZ::u8
            {
                WorldSpace,
                LocalSpace,
                Point,
                SplineFollow,
                SimpleDrag,
                LinearDamping
            };

            void Activate(AZ::EntityId entityId);
            void Deactivate();

            AZ::Vector3 CalculateForce(const EntityParams& entity, const RegionParams& region) const;
            bool IsWorldSpaceForce() const;
            bool IsLocalSpaceForce() const;
            bool IsPointForce() const;
            bool IsSplineFollowForce() const;
            bool IsSimpleDragForce() const;
            bool IsLinearDampingForce() const;

            BaseForce& GetCurrentBaseForce();
            const BaseForce& GetCurrentBaseForce() const;

            ForceType m_type = ForceType::WorldSpace;
            ForceWorldSpace m_forceWorldSpace;
            ForceLocalSpace m_forceLocalSpace;
            ForcePoint m_forcePoint;
            ForceSplineFollow m_forceSplineFollow;
            ForceSimpleDrag m_forceSimpleDrag;
            ForceLinearDamping m_forceLinearDamping;
        };

        void DrawForceArrows(const AZStd::vector<AZ::Vector3>& arrowPositions
            , AzFramework::DebugDisplayRequests& debugDisplayRequests);

        /// Checks if this force region has a spline follow force.
        bool HasSplineFollowForce() const;

        /// Callback invoked when there are changes to the forces in this force region.
        void OnForcesChanged() const;

        bool m_visibleInEditor = true; ///< Visible in the editor viewport even if force region entity is unselected.
        bool m_debugForces = false; ///< Draw debug lines (arrows) for forces in game.
        AZStd::vector<EditorForceProxy> m_forces; ///< Forces (editor version) in force region.
    };
}
