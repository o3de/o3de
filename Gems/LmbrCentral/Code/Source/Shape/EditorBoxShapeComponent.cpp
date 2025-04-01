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
#include <AzToolsFramework/ComponentModes/BoxComponentMode.h>
#include <AzToolsFramework/Maths/TransformUtils.h>

#include "BoxShapeComponent.h"
#include "EditorBoxShapeComponent.h"
#include "EditorShapeComponentConverters.h"
#include "ShapeDisplay.h"

namespace LmbrCentral
{
    void EditorBoxShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Deprecate: EditorBoxColliderComponent -> EditorBoxShapeComponent
            serializeContext->ClassDeprecate(
                "EditorBoxColliderComponent",
                AZ::Uuid("{E1707478-4F5F-4C28-A31A-EF42B7BD2A68}"),
                &ClassConverters::DeprecateEditorBoxColliderComponent)
                ;

            serializeContext->Class<EditorBoxShapeComponent, EditorBaseShapeComponent>()
                ->Version(3, &ClassConverters::UpgradeEditorBoxShapeComponent)
                ->Field("BoxShape", &EditorBoxShapeComponent::m_boxShape)
                ->Field("ComponentMode", &EditorBoxShapeComponent::m_componentModeDelegate)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorBoxShapeComponent>(
                    "Box Shape", "The Box Shape component creates a box around the associated entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Box_Shape.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Box_Shape.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/shape/box-shape/")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorBoxShapeComponent::m_boxShape, "Box Shape", "Box Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorBoxShapeComponent::ConfigurationChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorBoxShapeComponent::m_componentModeDelegate, "Component Mode", "Box Shape Component Mode")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;
            }
        }
    }

    void EditorBoxShapeComponent::Init()
    {
        EditorBaseShapeComponent::Init();

        SetShapeComponentConfig(&m_boxShape.ModifyConfiguration());
    }

    void EditorBoxShapeComponent::Activate()
    {
        EditorBaseShapeComponent::Activate();
        m_boxShape.Activate(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        const AZ::EntityComponentIdPair entityComponentIdPair(GetEntityId(), GetId());
        AzToolsFramework::BoxManipulatorRequestBus::Handler::BusConnect(entityComponentIdPair);
        AzToolsFramework::ShapeManipulatorRequestBus::Handler::BusConnect(entityComponentIdPair);

        // ComponentMode
        const bool allowAsymmetricalEditing = true;
        m_componentModeDelegate.ConnectWithSingleComponentMode<EditorBoxShapeComponent, AzToolsFramework::BoxComponentMode>(
            entityComponentIdPair, this, allowAsymmetricalEditing);
    }

    void EditorBoxShapeComponent::Deactivate()
    {
        m_componentModeDelegate.Disconnect();

        AzToolsFramework::ShapeManipulatorRequestBus::Handler::BusDisconnect();
        AzToolsFramework::BoxManipulatorRequestBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        m_boxShape.Deactivate();
        EditorBaseShapeComponent::Deactivate();
    }

    void EditorBoxShapeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        EditorBaseShapeComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("BoxShapeService"));
    }

    void EditorBoxShapeComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void EditorBoxShapeComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DisplayShape(
            debugDisplay, [this]() { return CanDraw(); },
            [this](AzFramework::DebugDisplayRequests& debugDisplay)
            {
                DrawBoxShape(
                    { m_boxShape.GetBoxConfiguration().GetDrawColor(), m_shapeWireColor, m_boxShape.GetBoxConfiguration().IsFilled() },
                    m_boxShape.GetBoxConfiguration(), debugDisplay, m_boxShape.GetCurrentNonUniformScale());
            },
            m_boxShape.GetCurrentTransform());
    }

    void EditorBoxShapeComponent::ConfigurationChanged()
    {
        m_boxShape.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);

        ShapeComponentNotificationsBus::Event(GetEntityId(),
            &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);

        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::Refresh,
            AZ::EntityComponentIdPair(GetEntityId(), GetId()));
    }

    void EditorBoxShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (BoxShapeComponent* boxShapeComponent = gameEntity->CreateComponent<BoxShapeComponent>())
        {
            boxShapeComponent->SetConfiguration(m_boxShape.GetBoxConfiguration());
        }

        if (m_visibleInGameView)
        {
            if (auto component = gameEntity->CreateComponent<BoxShapeDebugDisplayComponent>())
            {
                component->SetConfiguration(m_boxShape.GetBoxConfiguration());
            }
        }
    }

    void EditorBoxShapeComponent::OnTransformChanged(
        const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::Refresh,
            AZ::EntityComponentIdPair(GetEntityId(), GetId()));
    }

    AZ::Vector3 EditorBoxShapeComponent::GetDimensions() const
    {
        return m_boxShape.GetBoxDimensions();
    }

    void EditorBoxShapeComponent::SetDimensions(const AZ::Vector3& dimensions)
    {
        return m_boxShape.SetBoxDimensions(dimensions);
    }

    AZ::Vector3 EditorBoxShapeComponent::GetTranslationOffset() const
    {
        return m_boxShape.GetTranslationOffset();
    }

    void EditorBoxShapeComponent::SetTranslationOffset(const AZ::Vector3& translationOffset)
    {
        m_boxShape.SetTranslationOffset(translationOffset);
    }

    AZ::Transform EditorBoxShapeComponent::GetCurrentLocalTransform() const
    {
        return AZ::Transform::CreateTranslation(m_boxShape.GetTranslationOffset());
    }

    AZ::Transform EditorBoxShapeComponent::GetManipulatorSpace() const
    {
        return GetWorldTM();
    }

    AZ::Quaternion EditorBoxShapeComponent::GetRotationOffset() const
    {
        return AZ::Quaternion::CreateIdentity();
    }
} // namespace LmbrCentral
