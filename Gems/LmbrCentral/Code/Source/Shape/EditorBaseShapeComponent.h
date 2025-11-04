/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Transform.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/API/ComponentEntitySelectionBus.h>
#include <LmbrCentral/Shape/EditorShapeComponentBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <Shape/ShapeDisplay.h>

namespace LmbrCentral
{
    /// Common functionality for Editor Component Shapes.
    class EditorBaseShapeComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public EditorShapeComponentRequestsBus::Handler
        , public AzToolsFramework::EditorComponentSelectionRequestsBus::Handler
        , public AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , public AzFramework::BoundsRequestBus::Handler
        , protected ShapeComponentNotificationsBus::Handler
    {
    public:
        AZ_RTTI(EditorBaseShapeComponent, "{32B9D7E9-6743-427B-BAFD-1C42CFBE4879}", AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::SerializeContext& context);

        EditorBaseShapeComponent() = default;

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // AZ::TransformNotificationBus overrides ...
        void OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/) override {}

        // EditorShapeComponentRequestsBus overrides ...
        void SetShapeColor(const AZ::Color& solidColor) override;
        void SetShapeWireframeColor(const AZ::Color& wireColor) override;
        void SetVisibleInEditor(bool visible) override;
        void SetVisibleInGame(bool visible) override;
        void SetShapeColorIsEditable(bool editable) override;
        bool GetShapeColorIsEditable() override;

        // BoundsRequestBus overrides ...
        AZ::Aabb GetWorldBounds() const override;
        AZ::Aabb GetLocalBounds() const override;

        // ShapeComponentNotificationsBus overrides ...
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        /// Should shape be rendered all the time, even when not selected.
        bool CanDraw() const;

        void SetShapeComponentConfig(ShapeComponentConfig* shapeConfig);

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        // EditorComponentSelectionRequestsBus overrides ...
        AZ::Aabb GetEditorSelectionBoundsViewport(
            const AzFramework::ViewportInfo& viewportInfo) override;
        bool EditorSelectionIntersectRayViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) override;
        bool SupportsEditorRayIntersect() override;
        bool SupportsEditorRayIntersectViewport(const AzFramework::ViewportInfo& viewportInfo) override;

        // EditorComponentSelectionNotificationsBus overrides ... 
        void OnAccentTypeChanged(AzToolsFramework::EntityAccentType accent) override;

        AZ::Color m_shapeColor = AzFramework::ViewportColors::DeselectedColor; // Shaded color used for debug visualizations.
        AZ::Color m_shapeWireColor = AzFramework::ViewportColors::WireColor; // Wireframe color used for debug visualizations.
        AZ::Color m_shapeColorSaved = AzFramework::ViewportColors::DeselectedColor; // When the shape color is set to not be editable, its current value is saved here so it can be restored later.
        bool m_shapeColorIsEditable = true;

        bool m_visibleInEditor = true; // Visible in the editor viewport.
        bool m_visibleInGameView = false; // Visible in Game View.
        bool m_displayFilled = true; // Should shape be displayed filled.

        ShapeComponentConfig* m_shapeConfig = nullptr;

    private:

        void OnShapeColorChanged();
        void OnDisplayFilledChanged();
    };
} // namespace LmbrCentral
