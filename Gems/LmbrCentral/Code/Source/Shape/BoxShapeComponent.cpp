/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "BoxShapeComponent.h"
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Shape/ShapeComponentConverters.h>
#include <Shape/ShapeDisplay.h>

namespace LmbrCentral
{
    void BoxShapeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        provided.push_back(AZ_CRC("BoxShapeService", 0x946a0032));
    }

    void BoxShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        incompatible.push_back(AZ_CRC("BoxShapeService", 0x946a0032));
    }

    void BoxShapeComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void BoxShapeComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void BoxShapeDebugDisplayComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BoxShapeDebugDisplayComponent, EntityDebugDisplayComponent>()
                ->Version(1)
                ->Field("Configuration", &BoxShapeDebugDisplayComponent::m_boxShapeConfig)
                ;
        }
    }

    void BoxShapeDebugDisplayComponent::Activate()
    {
        EntityDebugDisplayComponent::Activate();
        ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        m_nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(m_nonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);
    }

    void BoxShapeDebugDisplayComponent::Deactivate()
    {
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
        EntityDebugDisplayComponent::Deactivate();
    }

    void BoxShapeDebugDisplayComponent::Draw(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        ShapeDrawParams drawParams = g_defaultShapeDrawParams;
        drawParams.m_shapeColor = m_boxShapeConfig.GetDrawColor();
        drawParams.m_filled = m_boxShapeConfig.IsFilled();

        DrawBoxShape(drawParams, m_boxShapeConfig, debugDisplay, m_nonUniformScale);
    }

    bool BoxShapeDebugDisplayComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const BoxShapeConfig*>(baseConfig))
        {
            m_boxShapeConfig = *config;
            return true;
        }
        return false;
    }

    bool BoxShapeDebugDisplayComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<BoxShapeConfig*>(outBaseConfig))
        {
            *outConfig = m_boxShapeConfig;
            return true;
        }
        return false;
    }

    void BoxShapeDebugDisplayComponent::OnShapeChanged(ShapeChangeReasons changeReason)
    {
        if (changeReason == ShapeChangeReasons::ShapeChanged)
        {
            BoxShapeComponentRequestsBus::EventResult(m_boxShapeConfig, GetEntityId(), &BoxShapeComponentRequests::GetBoxConfiguration);
            AZ::NonUniformScaleRequestBus::EventResult(m_nonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);
        }
    }

    void BoxShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        // Don't reflect again if we're already reflected to the passed in context
        if (context->IsTypeReflected(BoxShapeConfigTypeId))
        {
            return;
        }
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BoxShapeConfig, ShapeComponentConfig>()
                ->Version(2)
                ->Field("Dimensions", &BoxShapeConfig::m_dimensions)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BoxShapeConfig>("Configuration", "Box shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BoxShapeConfig::m_dimensions, "Dimensions", "Dimensions of the box along its axes")
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
                        ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<BoxShapeConfig>()
                ->Constructor()
                ->Constructor<AZ::Vector3&>()
                ->Property("Dimensions", BehaviorValueProperty(&BoxShapeConfig::m_dimensions))
                ;
        }
    }

    void BoxShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        BoxShape::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BoxShapeComponent, Component>()
                ->Version(2, &ClassConverters::UpgradeBoxShapeComponent)
                ->Field("BoxShape", &BoxShapeComponent::m_boxShape)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("BoxShapeComponentTypeId", BehaviorConstant(BoxShapeComponentTypeId));

            behaviorContext->EBus<BoxShapeComponentRequestsBus>("BoxShapeComponentRequestsBus")
                ->Event("GetBoxConfiguration", &BoxShapeComponentRequestsBus::Events::GetBoxConfiguration)
                ->Event("GetBoxDimensions", &BoxShapeComponentRequestsBus::Events::GetBoxDimensions)
                ->Event("SetBoxDimensions", &BoxShapeComponentRequestsBus::Events::SetBoxDimensions)
                ;
        }
    }

    void BoxShapeComponent::Activate()
    {
        m_boxShape.Activate(GetEntityId());
    }

    void BoxShapeComponent::Deactivate()
    {
        m_boxShape.Deactivate();
    }

    bool BoxShapeComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const BoxShapeConfig*>(baseConfig))
        {
            m_boxShape.SetBoxConfiguration(*config);
            return true;
        }
        return false;
    }

    bool BoxShapeComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<BoxShapeConfig*>(outBaseConfig))
        {
            *config = m_boxShape.GetBoxConfiguration();
            return true;
        }
        return false;
    }
} // namespace LmbrCentral
