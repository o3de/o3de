/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "SplineComponent.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <LmbrCentral/Shape/SplineComponentBus.h>

namespace LmbrCentral
{
    /// Editor representation of SplineComponent.
    class EditorSplineComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , private AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private SplineComponentRequestBus::Handler
        , private AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler
        , private AZ::VariableVerticesRequestBus<AZ::Vector3>::Handler
        , public AzFramework::BoundsRequestBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorSplineComponent, "{5B29D788-4885-4D56-BD9B-C0C45BE08EC1}", EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        EditorSplineComponent() = default;

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // EditorComponentBase overrides ...
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // BoundsRequestBus overrides ...
        AZ::Aabb GetWorldBounds() const override;
        AZ::Aabb GetLocalBounds() const override;

    private:
        AZ_DISABLE_COPY_MOVE(EditorSplineComponent)

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        // EditorComponentSelectionRequestsBus overrides ...
        AZ::Aabb GetEditorSelectionBoundsViewport(
            const AzFramework::ViewportInfo& viewportInfo) override;
        bool EditorSelectionIntersectRayViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) override;
        bool SupportsEditorRayIntersect() override;
        bool SupportsEditorRayIntersectViewport(const AzFramework::ViewportInfo& viewportInfo) override;

        // EditorComponentSelectionNotificationsBus overrides ...
        void OnAccentTypeChanged(AzToolsFramework::EntityAccentType accent) override { m_accentType = accent; }

        // TransformNotificationBus overrides ...
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // SplineComponentRequestBus overrides ...
        AZ::SplinePtr GetSpline() override;
        void ChangeSplineType(SplineType splineType) override;
        void SetClosed(bool closed) override;

        // SplineComponentRequestBus/VertexContainerInterface overrides ...
        bool GetVertex(size_t index, AZ::Vector3& vertex) const override;
        void AddVertex(const AZ::Vector3& vertex) override;
        bool UpdateVertex(size_t index, const AZ::Vector3& vertex) override;
        bool InsertVertex(size_t index, const AZ::Vector3& vertex) override;
        bool RemoveVertex(size_t index) override;
        void SetVertices(const AZStd::vector<AZ::Vector3>& vertices) override;
        void ClearVertices() override;
        size_t Size() const override;
        bool Empty() const override;

        // EntityDebugDisplayEventBus overrides ...
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        void OnChangeSplineType();
        void SplineChanged() const;

        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate; ///< Responsible for detecting ComponentMode activation 
                                                       ///< and creating a concrete ComponentMode.

        SplineCommon m_splineCommon; ///< Stores common spline functionality and properties.

        AZ::Transform m_cachedUniformScaleTransform; ///< Stores the current transform of the component with uniform scale.

        AzToolsFramework::EntityAccentType m_accentType = AzToolsFramework::EntityAccentType::None; ///< State of the entity selection in the viewport.
        bool m_visibleInEditor = true; ///< Visible in the editor viewport.
    };
} // namespace LmbrCentral
