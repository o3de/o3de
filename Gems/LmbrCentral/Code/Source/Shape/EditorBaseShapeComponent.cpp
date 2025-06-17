/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorBaseShapeComponent.h"

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace LmbrCentral
{
    void EditorBaseShapeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ShapeService"));
    }

    void EditorBaseShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ShapeService"));
    }

    void EditorBaseShapeComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void EditorBaseShapeComponent::Reflect(AZ::SerializeContext& context)
    {
        context.Class<EditorBaseShapeComponent, EditorComponentBase>()
            ->Version(2)
            ->Field("Visible", &EditorBaseShapeComponent::m_visibleInEditor)
            ->Field("GameView", &EditorBaseShapeComponent::m_visibleInGameView)
            ->Field("DisplayFilled", &EditorBaseShapeComponent::m_displayFilled)
            ->Field("ShapeColor", &EditorBaseShapeComponent::m_shapeColor);

        if (auto editContext = context.GetEditContext())
        {
            editContext->Class<EditorBaseShapeComponent>("EditorBaseShapeComponent", "Editor base shape component")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ->DataElement(
                    AZ::Edit::UIHandlers::CheckBox, &EditorBaseShapeComponent::m_visibleInEditor, "Visible",
                    "Always display this shape in the editor viewport")
                ->DataElement(
                    AZ::Edit::UIHandlers::CheckBox, &EditorBaseShapeComponent::m_visibleInGameView, "Game View",
                    "Display the shape while in Game View")
                ->DataElement(
                    AZ::Edit::UIHandlers::CheckBox, &EditorBaseShapeComponent::m_displayFilled, "Filled",
                    "Display the shape as either filled or wireframe") // hidden before selection is resolved
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorBaseShapeComponent::OnDisplayFilledChanged)
                ->DataElement(
                    AZ::Edit::UIHandlers::Default, &EditorBaseShapeComponent::m_shapeColor, "Shape Color",
                    "The color to use when rendering the faces of the shape object")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorBaseShapeComponent::OnShapeColorChanged)
                ->Attribute(AZ::Edit::Attributes::Visibility, &EditorBaseShapeComponent::GetShapeColorIsEditable);
        }
    }

    void EditorBaseShapeComponent::Activate()
    {
        EditorComponentBase::Activate();
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        EditorShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusConnect(GetEntityId());
        ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        AzFramework::BoundsRequestBus::Handler::BusConnect(GetEntityId());
    }

    void EditorBaseShapeComponent::Deactivate()
    {
        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionNotificationsBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
        EditorShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        EditorComponentBase::Deactivate();
    }

    void EditorBaseShapeComponent::SetVisibleInEditor(bool visible)
    {
        m_visibleInEditor = visible;
    }

    void EditorBaseShapeComponent::SetVisibleInGame(bool visible)
    {
        m_visibleInGameView = visible;
    }

    void EditorBaseShapeComponent::SetShapeColor(const AZ::Color& shapeColor)
    {
        m_shapeColor = shapeColor;
        InvalidatePropertyDisplay(AzToolsFramework::Refresh_Values);
    }

    void EditorBaseShapeComponent::SetShapeWireframeColor(const AZ::Color& wireColor)
    {
        m_shapeWireColor = wireColor;
    }

    void EditorBaseShapeComponent::SetShapeColorIsEditable(bool editable)
    {
        if (m_shapeColorIsEditable != editable)
        {
            m_shapeColorIsEditable = editable;

            if (editable)
            {
                // Restore the color to the value from when it was previously editable.
                m_shapeColor = m_shapeColorSaved;
            }
            else
            {
                // Save the current color so it can be restored if editable is turned back on later.
                m_shapeColorSaved = m_shapeColor;
            }

            // This changes the visibility of a property so a request to refresh the entire tree must be sent.
            InvalidatePropertyDisplay(AzToolsFramework::Refresh_EntireTree);
        }
    }

    bool EditorBaseShapeComponent::GetShapeColorIsEditable()
    {
        return m_shapeColorIsEditable;
    }

    bool EditorBaseShapeComponent::CanDraw() const
    {
        return IsSelected() || m_visibleInEditor;
    }

    void EditorBaseShapeComponent::SetShapeComponentConfig(ShapeComponentConfig* shapeConfig)
    {
        m_shapeConfig = shapeConfig;
    }

    AZ::Aabb EditorBaseShapeComponent::GetEditorSelectionBoundsViewport([[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo)
    {
        return GetWorldBounds();
    }

    bool EditorBaseShapeComponent::EditorSelectionIntersectRayViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src, const AZ::Vector3& dir, float& distance)
    {
        // if we are not drawing this or it is wireframe, do not allow selection
        if (!CanDraw() || !m_displayFilled)
        {
            return false;
        }

        // don't intersect with shapes when the camera is inside them
        bool isInside = false;
        ShapeComponentRequestsBus::EventResult(isInside, GetEntityId(), &ShapeComponentRequests::IsPointInside, src);
        if (isInside)
        {
            return false;
        }

        bool rayHit = false;
        ShapeComponentRequestsBus::EventResult(rayHit, GetEntityId(), &ShapeComponentRequests::IntersectRay, src, dir, distance);
        return rayHit;
    }

    bool EditorBaseShapeComponent::SupportsEditorRayIntersect()
    {
        return AzToolsFramework::HelpersVisible();
    }

    bool EditorBaseShapeComponent::SupportsEditorRayIntersectViewport(const AzFramework::ViewportInfo& viewportInfo)
    {
        bool helpersVisible = false;
        AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            helpersVisible, viewportInfo.m_viewportId,
            &AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::Events::HelpersVisible);
        return helpersVisible;
    }

    void EditorBaseShapeComponent::OnAccentTypeChanged(AzToolsFramework::EntityAccentType accent)
    {
        if (accent == AzToolsFramework::EntityAccentType::Hover || IsSelected())
        {
            SetShapeWireframeColor(AzFramework::ViewportColors::HoverColor);
        }
        else
        {
            SetShapeWireframeColor(AzFramework::ViewportColors::WireColor);
        }
    }

    void EditorBaseShapeComponent::OnShapeColorChanged()
    {
        AZ::Color drawColor(m_shapeColor);
        drawColor.SetA(AzFramework::ViewportColors::DeselectedColor.GetA());

        if (m_shapeConfig)
        {
            m_shapeConfig->SetDrawColor(drawColor);
        }
    }

    void EditorBaseShapeComponent::OnDisplayFilledChanged()
    {
        if (m_shapeConfig)
        {
            m_shapeConfig->SetIsFilled(m_displayFilled);
        }
    }

    AZ::Aabb EditorBaseShapeComponent::GetWorldBounds() const
    {
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        ShapeComponentRequestsBus::EventResult(aabb, GetEntityId(), &ShapeComponentRequests::GetEncompassingAabb);
        return aabb;
    }

    AZ::Aabb EditorBaseShapeComponent::GetLocalBounds() const
    {
        AZ::Transform unused;
        AZ::Aabb resultBounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::Event(
            GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetTransformAndLocalBounds, unused, resultBounds);
        return resultBounds;
    }

    void EditorBaseShapeComponent::OnShapeChanged(const ShapeChangeReasons changeReason)
    {
        if (changeReason == ShapeChangeReasons::ShapeChanged)
        {
            AZ::Interface<AzFramework::IEntityBoundsUnion>::Get()->RefreshEntityLocalBoundsUnion(GetEntityId());
        }
    }
} // namespace LmbrCentral
