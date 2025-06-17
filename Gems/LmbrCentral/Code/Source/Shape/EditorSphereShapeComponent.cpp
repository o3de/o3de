/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Shape/EditorSphereShapeComponent.h>

#include <Shape/SphereShapeComponent.h>
#include <AzCore/Serialization/EditContext.h>
#include <Shape/EditorShapeComponentConverters.h>
#include <Shape/ShapeDisplay.h>
#include <AzToolsFramework/ComponentModes/SphereComponentMode.h>

namespace LmbrCentral
{
    void EditorSphereShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Note: this must be called by the first EditorShapeComponent to have it's
            // reflect function called, which happens to be this one for now.
            EditorBaseShapeComponent::Reflect(*serializeContext);

            // Deprecate: EditorSphereColliderComponent -> EditorSphereShapeComponent
            serializeContext->ClassDeprecate(
                "EditorSphereColliderComponent",
                AZ::Uuid("{9A12FC39-60D2-4237-AC79-11FEDFEDB851}"),
                &ClassConverters::DeprecateEditorSphereColliderComponent)
                ;

            serializeContext->Class<EditorSphereShapeComponent, EditorBaseShapeComponent>()
                ->Version(3, &ClassConverters::UpgradeEditorSphereShapeComponent)
                ->Field("SphereShape", &EditorSphereShapeComponent::m_sphereShape)
                ->Field("ComponentMode", &EditorSphereShapeComponent::m_componentModeDelegate)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<EditorSphereShapeComponent>(
                        "Sphere Shape", "The Sphere Shape component creates a sphere around the associated entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Sphere_Shape.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Sphere_Shape.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(
                        AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/shape/sphere-shape/")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorSphereShapeComponent::m_sphereShape,
                        "Sphere Shape",
                        "Sphere Shape Configuration")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorSphereShapeComponent::ConfigurationChanged)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &EditorSphereShapeComponent::m_componentModeDelegate,
                        "Component Mode",
                        "Sphere Shape Component Mode")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
                ;
            }
        }
    }

    void EditorSphereShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        EditorBaseShapeComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void EditorSphereShapeComponent::Init()
    {
        EditorBaseShapeComponent::Init();

        SetShapeComponentConfig(&m_sphereShape.ModifyShapeComponent());
    }

    void EditorSphereShapeComponent::Activate()
    {
        EditorBaseShapeComponent::Activate();
        m_sphereShape.Activate(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());

        const AZ::EntityComponentIdPair entityComponentIdPair(GetEntityId(), GetId());
        AzToolsFramework::RadiusManipulatorRequestBus::Handler::BusConnect(entityComponentIdPair);
        AzToolsFramework::ShapeManipulatorRequestBus::Handler::BusConnect(entityComponentIdPair);

        const bool allowAsymmetricalEditing = true;
        m_componentModeDelegate.ConnectWithSingleComponentMode<EditorSphereShapeComponent, AzToolsFramework::SphereComponentMode>(
            entityComponentIdPair, this, allowAsymmetricalEditing);
    }

    void EditorSphereShapeComponent::Deactivate()
    {
        m_componentModeDelegate.Disconnect();

        AzToolsFramework::ShapeManipulatorRequestBus::Handler::BusDisconnect();
        AzToolsFramework::RadiusManipulatorRequestBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        m_sphereShape.Deactivate();
        EditorBaseShapeComponent::Deactivate();
    }

    void EditorSphereShapeComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DisplayShape(
            debugDisplay, [this]() { return CanDraw(); },
            [this](AzFramework::DebugDisplayRequests& debugDisplay)
            {
                DrawSphereShape(
                    { m_sphereShape.GetSphereConfiguration().GetDrawColor(), m_shapeWireColor, m_displayFilled },
                    m_sphereShape.GetSphereConfiguration(), debugDisplay);
            },
            m_sphereShape.GetCurrentTransform());
    }

    void EditorSphereShapeComponent::ConfigurationChanged()
    {
        m_sphereShape.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(
            GetEntityId(), &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);

        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::Refresh,
            AZ::EntityComponentIdPair(GetEntityId(), GetId()));
    }

    void EditorSphereShapeComponent::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform&)
    {
        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::Refresh,
            AZ::EntityComponentIdPair(GetEntityId(), GetId()));
    }

    void EditorSphereShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto component = gameEntity->CreateComponent<SphereShapeComponent>())
        {
            component->SetConfiguration(m_sphereShape.GetSphereConfiguration());
        }

        if (m_visibleInGameView)
        {
            if (auto component = gameEntity->CreateComponent<SphereShapeDebugDisplayComponent>())
            {
                component->SetConfiguration(m_sphereShape.GetSphereConfiguration());
            }
        }
    }

    float EditorSphereShapeComponent::GetRadius() const
    {
        return m_sphereShape.GetSphereConfiguration().m_radius;
    }

    void EditorSphereShapeComponent::SetRadius(float radius)
    {
        m_sphereShape.SetRadius(radius);
    }

    AZ::Vector3 EditorSphereShapeComponent::GetTranslationOffset() const
    {
        return m_sphereShape.GetTranslationOffset();
    }

    void EditorSphereShapeComponent::SetTranslationOffset(const AZ::Vector3& translationOffset)
    {
        m_sphereShape.SetTranslationOffset(translationOffset);
    }

    AZ::Transform EditorSphereShapeComponent::GetManipulatorSpace() const
    {
        return GetWorldTM();
    }

    AZ::Quaternion EditorSphereShapeComponent::GetRotationOffset() const
    {
        return AZ::Quaternion::CreateIdentity();
    }
} // namespace LmbrCentral
