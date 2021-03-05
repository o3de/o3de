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
#include "PolygonPrismShapeComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Shape/ShapeDisplay.h>

namespace LmbrCentral
{
    void PolygonPrismShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PolygonPrismShapeConfig, ShapeComponentConfig>()
                ->Version(1)
            ;
        }
    }

    void PolygonPrismShapeDebugDisplayComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PolygonPrismShapeDebugDisplayComponent, EntityDebugDisplayComponent>()
                ->Version(2)
                ->Field("PolygonPrism", &PolygonPrismShapeDebugDisplayComponent::m_polygonPrism)
                ->Field("PolygonShapeConfig", &PolygonPrismShapeDebugDisplayComponent::m_polygonShapeConfig)
                ;
        }
    }

    void PolygonPrismShapeDebugDisplayComponent::SetShapeConfig(const PolygonPrismShapeConfig& shapeConfig)
    {
        m_polygonShapeConfig = shapeConfig;
    }

    void PolygonPrismShapeDebugDisplayComponent::Activate()
    {
        EntityDebugDisplayComponent::Activate();
        ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        GenerateVertices();
    }

    void PolygonPrismShapeDebugDisplayComponent::Deactivate()
    {
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
        EntityDebugDisplayComponent::Deactivate();
    }

    void PolygonPrismShapeDebugDisplayComponent::Draw(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DrawPolygonPrismShape(m_polygonShapeConfig.GetDrawParams(), m_polygonPrismMesh, debugDisplay);
    }

    void PolygonPrismShapeDebugDisplayComponent::OnShapeChanged(ShapeChangeReasons changeReason)
    {
        if (changeReason == ShapeChangeReasons::ShapeChanged)
        {
            AZ::PolygonPrismPtr polygonPrismPtr = nullptr;
            PolygonPrismShapeComponentRequestBus::EventResult(polygonPrismPtr, GetEntityId(), &PolygonPrismShapeComponentRequests::GetPolygonPrism);
            if (polygonPrismPtr)
            {
                m_polygonPrism = *polygonPrismPtr;
                GenerateVertices();
            }
        }
    }

    void PolygonPrismShapeDebugDisplayComponent::GenerateVertices()
    {
        GeneratePolygonPrismMesh(
            m_polygonPrism.m_vertexContainer.GetVertices(),
            m_polygonPrism.GetHeight(), m_polygonPrismMesh);
    }

    void PolygonPrismShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        PolygonPrismShape::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PolygonPrismShapeComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &PolygonPrismShapeComponent::m_polygonPrismShape)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<PolygonPrismShapeComponentRequestBus>("PolygonPrismShapeComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                ->Attribute(AZ::Script::Attributes::Module, "shape")
                ->Event("GetPolygonPrism", &PolygonPrismShapeComponentRequestBus::Events::GetPolygonPrism)
                ->Event("SetHeight", &PolygonPrismShapeComponentRequestBus::Events::SetHeight)
                ->Event("AddVertex", &PolygonPrismShapeComponentRequestBus::Events::AddVertex)
                ->Event("UpdateVertex", &PolygonPrismShapeComponentRequestBus::Events::UpdateVertex)
                ->Event("InsertVertex", &PolygonPrismShapeComponentRequestBus::Events::InsertVertex)
                ->Event("RemoveVertex", &PolygonPrismShapeComponentRequestBus::Events::RemoveVertex)
                ->Event("ClearVertices", &PolygonPrismShapeComponentRequestBus::Events::ClearVertices)
                ;
        }
    }

    void PolygonPrismShapeComponent::Activate()
    {
        m_polygonPrismShape.Activate(GetEntityId());
    }

    void PolygonPrismShapeComponent::Deactivate()
    {
        m_polygonPrismShape.Deactivate();
    }
} // namespace LmbrCentral
