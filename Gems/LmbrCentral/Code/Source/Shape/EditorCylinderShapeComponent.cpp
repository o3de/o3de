/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorCylinderShapeComponent.h"

#include "CylinderShapeComponent.h"
#include "EditorShapeComponentConverters.h"
#include "ShapeDisplay.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/ComponentModes/CylinderComponentMode.h>

namespace LmbrCentral
{
    void EditorCylinderShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Deprecate: EditorCylinderColliderComponent -> EditorCylinderShapeComponent
            serializeContext->ClassDeprecate(
                "EditorCylinderColliderComponent",
                AZ::Uuid("{1C10CEE7-0A5C-4D4A-BBD9-5C3B6C6FE844}"),
                &ClassConverters::DeprecateEditorCylinderColliderComponent);

            serializeContext->Class<EditorCylinderShapeComponent, EditorBaseShapeComponent>()
                ->Version(3, &ClassConverters::UpgradeEditorCylinderShapeComponent)
                ->Field("CylinderShape", &EditorCylinderShapeComponent::m_cylinderShape)
                ->Field("ComponentMode", &EditorCylinderShapeComponent::m_componentModeDelegate);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<EditorCylinderShapeComponent>(
                        "Cylinder Shape", "The Cylinder Shape component creates a cylinder around the associated entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Cylinder_Shape.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Cylinder_Shape.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(
                        AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/shape/cylinder-shape/")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorCylinderShapeComponent::m_cylinderShape,
                        "Cylinder Shape",
                        "Cylinder Shape Configuration")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorCylinderShapeComponent::ConfigurationChanged)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorCylinderShapeComponent::m_componentModeDelegate,
                        "Component Mode",
                        "Cylinder Shape Component Mode")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
            }
        }
    }

    void EditorCylinderShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        EditorBaseShapeComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void EditorCylinderShapeComponent::Init()
    {
        EditorBaseShapeComponent::Init();

        SetShapeComponentConfig(&m_cylinderShape.ModifyConfiguration());
    }

    float EditorCylinderShapeComponent::GetHeight() const
    {
        return m_cylinderShape.GetCylinderConfiguration().m_height;
    }

    void EditorCylinderShapeComponent::SetHeight(float height)
    {
        m_cylinderShape.SetHeight(height);
        ConfigurationChanged();
    }

    float EditorCylinderShapeComponent::GetRadius() const
    {
        return m_cylinderShape.GetCylinderConfiguration().m_radius;
    }

    void EditorCylinderShapeComponent::SetRadius(float radius)
    {
        m_cylinderShape.SetRadius(radius);
        ConfigurationChanged();
    }

    AZ::Vector3 EditorCylinderShapeComponent::GetTranslationOffset() const
    {
        return m_cylinderShape.GetTranslationOffset();
    }

    void EditorCylinderShapeComponent::SetTranslationOffset(const AZ::Vector3& translationOffset)
    {
        m_cylinderShape.SetTranslationOffset(translationOffset);
        ConfigurationChanged();
    }

    AZ::Transform EditorCylinderShapeComponent::GetManipulatorSpace() const
    {
        return GetWorldTM();
    }

    AZ::Quaternion EditorCylinderShapeComponent::GetRotationOffset() const
    {
        return AZ::Quaternion::CreateIdentity();
    }

    void EditorCylinderShapeComponent::Activate()
    {
        EditorBaseShapeComponent::Activate();
        m_cylinderShape.Activate(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        const AZ::EntityComponentIdPair entityComponentIdPair(GetEntityId(), GetId());
        AzToolsFramework::CylinderManipulatorRequestBus::Handler::BusConnect(entityComponentIdPair);
        AzToolsFramework::RadiusManipulatorRequestBus::Handler::BusConnect(entityComponentIdPair);
        AzToolsFramework::ShapeManipulatorRequestBus::Handler::BusConnect(entityComponentIdPair);

        const bool allowAsymmetricalEditing = true;
        m_componentModeDelegate.ConnectWithSingleComponentMode<EditorCylinderShapeComponent, AzToolsFramework::CylinderComponentMode>(
            entityComponentIdPair, this, allowAsymmetricalEditing);
    }

    void EditorCylinderShapeComponent::Deactivate()
    {
        m_componentModeDelegate.Disconnect();

        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::CylinderManipulatorRequestBus::Handler::BusDisconnect();
        AzToolsFramework::RadiusManipulatorRequestBus::Handler::BusDisconnect();
        AzToolsFramework::ShapeManipulatorRequestBus::Handler::BusDisconnect();
        m_cylinderShape.Deactivate();
        EditorBaseShapeComponent::Deactivate();
    }

    void EditorCylinderShapeComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DisplayShape(
            debugDisplay,
            [this]()
            {
                return CanDraw();
            },
            [this](AzFramework::DebugDisplayRequests& debugDisplay)
            {
                DrawCylinderShape(
                    { m_cylinderShape.GetCylinderConfiguration().GetDrawColor(), m_shapeWireColor, m_displayFilled },
                    m_cylinderShape.GetCylinderConfiguration(),
                    debugDisplay);
            },
            m_cylinderShape.GetCurrentTransform());
    }

    void EditorCylinderShapeComponent::ConfigurationChanged()
    {
        m_cylinderShape.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(
            GetEntityId(),
            &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void EditorCylinderShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto component = gameEntity->CreateComponent<CylinderShapeComponent>())
        {
            component->SetConfiguration(m_cylinderShape.GetCylinderConfiguration());
        }

        if (m_visibleInGameView)
        {
            if (auto component = gameEntity->CreateComponent<CylinderShapeDebugDisplayComponent>())
            {
                component->SetConfiguration(m_cylinderShape.GetCylinderConfiguration());
            }
        }
    }
} // namespace LmbrCentral
