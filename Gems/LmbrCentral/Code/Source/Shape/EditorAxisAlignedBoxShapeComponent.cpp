/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/Maths/TransformUtils.h>

#include "AxisAlignedBoxShapeComponent.h"
#include "EditorAxisAlignedBoxShapeComponent.h"
#include "EditorAxisAlignedBoxShapeComponentMode.h"
#include "EditorShapeComponentConverters.h"
#include "ShapeDisplay.h"

namespace LmbrCentral
{
    void EditorAxisAlignedBoxShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorAxisAlignedBoxShapeComponentMode::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorAxisAlignedBoxShapeComponent, EditorBaseShapeComponent>()
                ->Version(1)
                ->Field("AxisAlignedBoxShape", &EditorAxisAlignedBoxShapeComponent::m_aaboxShape)
                ->Field("ComponentMode", &EditorAxisAlignedBoxShapeComponent::m_componentModeDelegate)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorAxisAlignedBoxShapeComponent>(
                    "Axis Aligned Box Shape", "The Axis Aligned Box Shape component creates a box around the associated entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/AxisAlignedBoxShape.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/AxisAlignedBoxShape.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/shape/axis-aligned-box-shape/")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAxisAlignedBoxShapeComponent::m_aaboxShape, "Axis Aligned Box Shape", "Axis Aligned Box Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAxisAlignedBoxShapeComponent::ConfigurationChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAxisAlignedBoxShapeComponent::m_componentModeDelegate, "Component Mode", "Axis Aligned Box Shape Component Mode")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
            }
        }
    }

    void EditorAxisAlignedBoxShapeComponent::Init()
    {
        EditorBaseShapeComponent::Init();

        SetShapeComponentConfig(&m_aaboxShape.ModifyConfiguration());
    }

    void EditorAxisAlignedBoxShapeComponent::Activate()
    {
        EditorBaseShapeComponent::Activate();
        m_aaboxShape.Activate(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::BoxManipulatorRequestBus::Handler::BusConnect(
            AZ::EntityComponentIdPair(GetEntityId(), GetId()));
        AzToolsFramework::ShapeManipulatorRequestBus::Handler::BusConnect(
            AZ::EntityComponentIdPair(GetEntityId(), GetId()));

        // ComponentMode
        const bool allowAsymmetricalEditing = true;
        m_componentModeDelegate.ConnectWithSingleComponentMode<
            EditorAxisAlignedBoxShapeComponent, EditorAxisAlignedBoxShapeComponentMode>(
                AZ::EntityComponentIdPair(GetEntityId(), GetId()), this, allowAsymmetricalEditing);
    }

    void EditorAxisAlignedBoxShapeComponent::Deactivate()
    {
        m_componentModeDelegate.Disconnect();

        AzToolsFramework::ShapeManipulatorRequestBus::Handler::BusDisconnect();
        AzToolsFramework::BoxManipulatorRequestBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        m_aaboxShape.Deactivate();
        EditorBaseShapeComponent::Deactivate();
    }

    void EditorAxisAlignedBoxShapeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        EditorBaseShapeComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("BoxShapeService"));
        provided.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
    }

    void EditorAxisAlignedBoxShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        EditorBaseShapeComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void EditorAxisAlignedBoxShapeComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DisplayShape(
            debugDisplay, [this]() { return CanDraw(); },
            [this](AzFramework::DebugDisplayRequests& debugDisplay)
            {
                DrawBoxShape(
                    { m_aaboxShape.GetBoxConfiguration().GetDrawColor(), m_shapeWireColor, m_aaboxShape.GetBoxConfiguration().IsFilled() },
                    m_aaboxShape.GetBoxConfiguration(), debugDisplay);
            },
            m_aaboxShape.GetCurrentTransform());
    }

    void EditorAxisAlignedBoxShapeComponent::ConfigurationChanged()
    {
        m_aaboxShape.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);

        ShapeComponentNotificationsBus::Event(GetEntityId(),
            &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);

        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::Refresh,
            AZ::EntityComponentIdPair(GetEntityId(), GetId()));
    }

    void EditorAxisAlignedBoxShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (AxisAlignedBoxShapeComponent* boxShapeComponent = gameEntity->CreateComponent<AxisAlignedBoxShapeComponent>())
        {
            boxShapeComponent->SetConfiguration(m_aaboxShape.GetBoxConfiguration());
        }

        if (m_visibleInGameView)
        {
            if (auto component = gameEntity->CreateComponent<AxisAlignedBoxShapeDebugDisplayComponent>())
            {
                component->SetConfiguration(m_aaboxShape.GetBoxConfiguration());
            }
        }
    }

    void EditorAxisAlignedBoxShapeComponent::OnTransformChanged(
        const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::Refresh,
            AZ::EntityComponentIdPair(GetEntityId(), GetId()));
    }

    AZ::Vector3 EditorAxisAlignedBoxShapeComponent::GetDimensions() const
    {
        return m_aaboxShape.GetBoxDimensions();
    }

    void EditorAxisAlignedBoxShapeComponent::SetDimensions(const AZ::Vector3& dimensions)
    {
        return m_aaboxShape.SetBoxDimensions(dimensions);
    }

    AZ::Vector3 EditorAxisAlignedBoxShapeComponent::GetTranslationOffset() const
    {
        return m_aaboxShape.GetTranslationOffset();
    }

    void EditorAxisAlignedBoxShapeComponent::SetTranslationOffset(const AZ::Vector3& translationOffset)
    {
        m_aaboxShape.SetTranslationOffset(translationOffset);
    }

    AZ::Transform EditorAxisAlignedBoxShapeComponent::GetCurrentLocalTransform() const
    {
        return AZ::Transform::CreateTranslation(m_aaboxShape.GetTranslationOffset());
    }

    AZ::Transform EditorAxisAlignedBoxShapeComponent::GetManipulatorSpace() const
    {
        AZ::Transform worldTransform = GetWorldTM();
        worldTransform.SetRotation(AZ::Quaternion::CreateIdentity());
        return worldTransform;
    }

    AZ::Quaternion EditorAxisAlignedBoxShapeComponent::GetRotationOffset() const
    {
        return AZ::Quaternion::CreateIdentity();
    }

    AZ::Aabb EditorAxisAlignedBoxShapeComponent::GetLocalBounds() const
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        m_aaboxShape.GetTransformAndLocalBounds(transform, aabb);
        return aabb.GetTransformedAabb(AZ::Transform::CreateFromQuaternion(GetWorldTM().GetRotation().GetInverseFast()));
    }
} // namespace LmbrCentral

