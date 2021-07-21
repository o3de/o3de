/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LmbrCentral_precompiled.h"
#include "SphereShapeComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include "EditorSphereShapeComponent.h"
#include "EditorShapeComponentConverters.h"
#include "ShapeDisplay.h"

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
                "{9A12FC39-60D2-4237-AC79-11FEDFEDB851}",
                &ClassConverters::DeprecateEditorSphereColliderComponent)
                ;

            serializeContext->Class<EditorSphereShapeComponent, EditorBaseShapeComponent>()
                ->Version(3, &ClassConverters::UpgradeEditorSphereShapeComponent)
                ->Field("SphereShape", &EditorSphereShapeComponent::m_sphereShape)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorSphereShapeComponent>(
                    "Sphere Shape", "The Sphere Shape component creates a sphere around the associated entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Sphere_Shape.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Sphere_Shape.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/shape/sphere-shape/")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorSphereShapeComponent::m_sphereShape, "Sphere Shape", "Sphere Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorSphereShapeComponent::ConfigurationChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
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
    }

    void EditorSphereShapeComponent::Deactivate()
    {
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
                    { m_shapeColor, m_shapeWireColor, m_displayFilled },
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
} // namespace LmbrCentral
