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
#include "TubeShapeComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <Shape/ShapeDisplay.h>

namespace LmbrCentral
{
    void TubeShapeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        provided.push_back(AZ_CRC("TubeShapeService", 0x3fe791b4));
    }

    void TubeShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        incompatible.push_back(AZ_CRC("TubeShapeService", 0x3fe791b4));
    }

    void TubeShapeComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        required.push_back(AZ_CRC("SplineService", 0x2b674d3c));
    }

    void TubeShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            SplineAttribute<float>::Reflect(*serializeContext);
            TubeShape::Reflect(*serializeContext);

            serializeContext->Class<TubeShapeComponent, AZ::Component>()
                ->Version(1)
                ->Field("TubeShape", &TubeShapeComponent::m_tubeShape)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("TubeShapeComponentTypeId", BehaviorConstant(TubeShapeComponentTypeId));

            behaviorContext->EBus<TubeShapeComponentRequestsBus>("TubeShapeComponentRequestsBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Shape")
                ->Event("SetRadius", &TubeShapeComponentRequestsBus::Events::SetRadius)
                ->Event("GetRadius", &TubeShapeComponentRequestsBus::Events::GetRadius)
                ->Event("SetVariableRadius", &TubeShapeComponentRequestsBus::Events::SetVariableRadius)
                ->Event("GetVariableRadius", &TubeShapeComponentRequestsBus::Events::GetVariableRadius)
                ->Event("GetTotalRadius", &TubeShapeComponentRequestsBus::Events::GetTotalRadius)
                ;
        }
    }

    TubeShapeComponent::TubeShapeComponent(const TubeShape& tubeShape)
        : m_tubeShape(tubeShape) {}

    void TubeShapeComponent::Activate()
    {
        m_tubeShape.Activate(GetEntityId());
    }

    void TubeShapeComponent::Deactivate()
    {
        m_tubeShape.Deactivate();
    }

    void TubeShapeDebugDisplayComponent::Reflect(AZ::ReflectContext* context)
    {
        TubeShapeMeshConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TubeShapeDebugDisplayComponent, EntityDebugDisplayComponent>()
                ->Version(1)
                ->Field("Radius", &TubeShapeDebugDisplayComponent::m_radius)
                ->Field("Spline", &TubeShapeDebugDisplayComponent::m_spline)
                ->Field("TubeShapeMeshConfig", &TubeShapeDebugDisplayComponent::m_tubeShapeMeshConfig)
                ->Field("RadiusAttribute", &TubeShapeDebugDisplayComponent::m_radiusAttribute)
                ;
        }
    }

    void TubeShapeDebugDisplayComponent::Activate()
    {
        EntityDebugDisplayComponent::Activate();

        SplineComponentRequestBus::EventResult(m_spline, GetEntityId(), &SplineComponentRequests::GetSpline);
        TubeShapeComponentRequestsBus::EventResult(m_radius, GetEntityId(), &TubeShapeComponentRequests::GetRadius);
        TubeShapeComponentRequestsBus::EventResult(m_radiusAttribute, GetEntityId(), &TubeShapeComponentRequests::GetRadiusAttribute);
        ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());

        GenerateVertices();
    }

    void TubeShapeDebugDisplayComponent::Deactivate()
    {
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
        EntityDebugDisplayComponent::Deactivate();
    }

    void TubeShapeDebugDisplayComponent::Draw(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DrawShape(debugDisplay, m_tubeShapeMeshConfig.m_shapeComponentConfig.GetDrawParams(), m_tubeShapeMesh);
    }

    void TubeShapeDebugDisplayComponent::OnShapeChanged(ShapeChangeReasons changeReason)
    {
        if (changeReason == ShapeChangeReasons::ShapeChanged)
        {
            TubeShapeComponentRequestsBus::EventResult(m_radius, GetEntityId(), &TubeShapeComponentRequests::GetRadius);
            TubeShapeComponentRequestsBus::EventResult(m_radiusAttribute, GetEntityId(), &TubeShapeComponentRequests::GetRadiusAttribute);
            GenerateVertices();
        }
    }

    void TubeShapeDebugDisplayComponent::GenerateVertices()
    {
        GenerateTubeMesh(
            m_spline, m_radiusAttribute, m_radius, m_tubeShapeMeshConfig.m_endSegments,
            m_tubeShapeMeshConfig.m_sides, m_tubeShapeMesh.m_vertexBuffer,
            m_tubeShapeMesh.m_indexBuffer, m_tubeShapeMesh.m_lineBuffer);
    }
} // namespace LmbrCentral