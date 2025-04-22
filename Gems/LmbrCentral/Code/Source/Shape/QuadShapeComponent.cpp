/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "QuadShapeComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace LmbrCentral
{
    void QuadShapeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ShapeService"));
        provided.push_back(AZ_CRC_CE("QuadShapeService"));
    }

    void QuadShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ShapeService"));
        incompatible.push_back(AZ_CRC_CE("QuadShapeService"));
    }

    void QuadShapeComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void QuadShapeComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void QuadShapeDebugDisplayComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<QuadShapeDebugDisplayComponent, EntityDebugDisplayComponent>()
                ->Version(1)
                ->Field("Configuration", &QuadShapeDebugDisplayComponent::m_quadShapeConfig)
                ;
        }
    }

    void QuadShapeDebugDisplayComponent::Activate()
    {
        EntityDebugDisplayComponent::Activate();
        ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        m_nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(m_nonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);
    }

    void QuadShapeDebugDisplayComponent::Deactivate()
    {
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
        EntityDebugDisplayComponent::Deactivate();
    }

    void QuadShapeDebugDisplayComponent::Draw(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DrawQuadShape(m_quadShapeConfig.GetDrawParams(), m_quadShapeConfig, debugDisplay, m_nonUniformScale);
    }

    bool QuadShapeDebugDisplayComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto* config = azrtti_cast<const QuadShapeConfig*>(baseConfig))
        {
            m_quadShapeConfig = *config;
            return true;
        }
        return false;
    }

    bool QuadShapeDebugDisplayComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto* outConfig = azrtti_cast<QuadShapeConfig*>(outBaseConfig))
        {
            *outConfig = m_quadShapeConfig;
            return true;
        }
        return false;
    }

    void QuadShapeDebugDisplayComponent::OnShapeChanged(ShapeChangeReasons changeReason)
    {
        if (changeReason == ShapeChangeReasons::ShapeChanged)
        {
            QuadShapeComponentRequestBus::EventResult(m_quadShapeConfig, GetEntityId(), &QuadShapeComponentRequests::GetQuadConfiguration);
            AZ::NonUniformScaleRequestBus::EventResult(m_nonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);
        }
    }

    void QuadShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<QuadShapeConfig, ShapeComponentConfig>()
                ->Version(1)
                ->Field("Width", &QuadShapeConfig::m_width)
                ->Field("Height", &QuadShapeConfig::m_height)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<QuadShapeConfig>("Configuration", "Quad shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &QuadShapeConfig::m_width, "Width", "Width of quad")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &QuadShapeConfig::m_height, "Height", "Height of quad")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<QuadShapeConfig>()
                ->Constructor()
                ->Constructor<float, float>()
                ->Property("Width", BehaviorValueProperty(&QuadShapeConfig::m_width))
                ->Property("Height", BehaviorValueProperty(&QuadShapeConfig::m_height))
                ;
        }
    }

    void QuadShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        QuadShape::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<QuadShapeComponent, AZ::Component>()
                ->Version(1)
                ->Field("QuadShape", &QuadShapeComponent::m_quadShape)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("QuadShapeComponentTypeId", BehaviorConstant(QuadShapeComponentTypeId));

            behaviorContext->EBus<QuadShapeComponentRequestBus>("QuadShapeComponentRequestsBus")
                ->Event("GetQuadConfiguration", &QuadShapeComponentRequestBus::Events::GetQuadConfiguration)
                ->Event("SetQuadWidth", &QuadShapeComponentRequestBus::Events::SetQuadWidth)
                ->Event("GetQuadWidth", &QuadShapeComponentRequestBus::Events::GetQuadWidth)
                ->Event("SetQuadHeight", &QuadShapeComponentRequestBus::Events::SetQuadHeight)
                ->Event("GetQuadHeight", &QuadShapeComponentRequestBus::Events::GetQuadHeight)
                ->Event("GetQuadOrientation", &QuadShapeComponentRequestBus::Events::GetQuadOrientation)
                ->VirtualProperty("QuadWidth", "GetQuadWidth", "SetQuadWidth")
                ->VirtualProperty("QuadHeight", "GetQuadHeight", "SetQuadHeight")
                ;
        }
    }

    void QuadShapeComponent::Activate()
    {
        m_quadShape.Activate(GetEntityId());
    }

    void QuadShapeComponent::Deactivate()
    {
        m_quadShape.Deactivate();
    }

    bool QuadShapeComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const QuadShapeConfig*>(baseConfig))
        {
            m_quadShape.SetQuadConfiguration(*config);
            return true;
        }
        return false;
    }

    bool QuadShapeComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<QuadShapeConfig*>(outBaseConfig))
        {
            *outConfig = m_quadShape.GetQuadConfiguration();
            return true;
        }
        return false;
    }
} // namespace LmbrCentral
