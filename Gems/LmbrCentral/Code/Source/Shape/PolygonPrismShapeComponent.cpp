/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(nonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);
        m_polygonPrism.SetNonUniformScale(nonUniformScale);
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
            m_polygonPrism.GetHeight(), m_polygonPrism.GetNonUniformScale(), m_polygonPrismMesh);
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
