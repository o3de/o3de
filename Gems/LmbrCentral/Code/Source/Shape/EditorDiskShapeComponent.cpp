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

#include "LmbrCentral_precompiled.h"
#include "DiskShapeComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include "EditorDiskShapeComponent.h"
#include "EditorShapeComponentConverters.h"
#include "ShapeDisplay.h"

namespace LmbrCentral
{
    void EditorDiskShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorDiskShapeComponent, EditorBaseShapeComponent>()
                ->Version(1)
                ->Field("DiskShape", &EditorDiskShapeComponent::m_diskShape)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorDiskShapeComponent>(
                    "Disk Shape", "The Disk Shape component creates a disk around the associated entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Disk_Shape.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Disk_Shape.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-shapes.html")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorDiskShapeComponent::m_diskShape, "Disk Shape", "Disk Shape Configuration")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorDiskShapeComponent::ConfigurationChanged)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void EditorDiskShapeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        EditorBaseShapeComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC("DiskShapeService", 0xd90c482b));
    }

    void EditorDiskShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        EditorBaseShapeComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void EditorDiskShapeComponent::Init()
    {
        EditorBaseShapeComponent::Init();

        SetShapeComponentConfig(&m_diskShape.ModifyShapeComponent());
    }

    void EditorDiskShapeComponent::Activate()
    {
        EditorBaseShapeComponent::Activate();
        m_diskShape.Activate(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void EditorDiskShapeComponent::Deactivate()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        m_diskShape.Deactivate();
        EditorBaseShapeComponent::Deactivate();
    }

    void EditorDiskShapeComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DisplayShape(
            debugDisplay, [this]() { return CanDraw(); },
            [this](AzFramework::DebugDisplayRequests& debugDisplay)
            {
                DrawDiskShape(
                    { m_shapeColor, m_shapeWireColor, m_displayFilled },
                    m_diskShape.GetDiskConfiguration(), debugDisplay);
            },
            m_diskShape.GetCurrentTransform());
    }

    void EditorDiskShapeComponent::ConfigurationChanged()
    {
        m_diskShape.InvalidateCache(InvalidateShapeCacheReason::ShapeChange);
        ShapeComponentNotificationsBus::Event(
            GetEntityId(), &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void EditorDiskShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto component = gameEntity->CreateComponent<DiskShapeComponent>())
        {
            component->SetConfiguration(m_diskShape.GetDiskConfiguration());
        }

        if (m_visibleInGameView)
        {
            if (auto component = gameEntity->CreateComponent<DiskShapeDebugDisplayComponent>())
            {
                component->SetConfiguration(m_diskShape.GetDiskConfiguration());
            }
        }
    }
} // namespace LmbrCentral
