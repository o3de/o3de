/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "AxisAlignedBoxShapeComponent.h"
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Shape/ShapeComponentConverters.h>
#include <Shape/ShapeDisplay.h>

namespace LmbrCentral
{
    void AxisAlignedBoxShapeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ShapeService"));
        provided.push_back(AZ_CRC_CE("BoxShapeService"));
        provided.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
    }

    void AxisAlignedBoxShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ShapeService"));
        incompatible.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
    }

    void AxisAlignedBoxShapeComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void AxisAlignedBoxShapeComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void AxisAlignedBoxShapeDebugDisplayComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AxisAlignedBoxShapeDebugDisplayComponent, EntityDebugDisplayComponent>()
                ->Version(1)->Field(
                "Configuration", &AxisAlignedBoxShapeDebugDisplayComponent::m_boxShapeConfig)
                ;
        }
    }

    void AxisAlignedBoxShapeDebugDisplayComponent::Activate()
    {
        EntityDebugDisplayComponent::Activate();
        ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        m_nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(m_nonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);
    }

    void AxisAlignedBoxShapeDebugDisplayComponent::Deactivate()
    {
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
        EntityDebugDisplayComponent::Deactivate();
    }

    void AxisAlignedBoxShapeDebugDisplayComponent::Draw(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ::Matrix3x4 saveMatrix;
        ShapeDrawParams drawParams = g_defaultShapeDrawParams;
        drawParams.m_shapeColor = m_boxShapeConfig.GetDrawColor();
        drawParams.m_filled = m_boxShapeConfig.IsFilled();
        AZ::Transform transform = GetCurrentTransform();
        transform.SetRotation(AZ::Quaternion::CreateIdentity());
        saveMatrix = debugDisplay.PopPremultipliedMatrix();
        debugDisplay.PushMatrix(transform);
        DrawBoxShape(drawParams, m_boxShapeConfig, debugDisplay, m_nonUniformScale);
        debugDisplay.PopMatrix();
        debugDisplay.PushPremultipliedMatrix(saveMatrix);
    }

    bool AxisAlignedBoxShapeDebugDisplayComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const BoxShapeConfig*>(baseConfig))
        {
            m_boxShapeConfig = *config;
            return true;
        }
        return false;
    }

    bool AxisAlignedBoxShapeDebugDisplayComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<BoxShapeConfig*>(outBaseConfig))
        {
            *outConfig = m_boxShapeConfig;
            return true;
        }
        return false;
    }

    void AxisAlignedBoxShapeDebugDisplayComponent::OnShapeChanged(ShapeChangeReasons changeReason)
    {
        if (changeReason == ShapeChangeReasons::ShapeChanged)
        {
            BoxShapeComponentRequestsBus::EventResult(m_boxShapeConfig, GetEntityId(), &BoxShapeComponentRequests::GetBoxConfiguration);
            AZ::NonUniformScaleRequestBus::EventResult(m_nonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);
        }
    }

    void AxisAlignedBoxShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        AxisAlignedBoxShape::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AxisAlignedBoxShapeComponent, Component>()
                ->Version(1)
                ->Field("AxisAlignedBoxShape", &AxisAlignedBoxShapeComponent::m_aaboxShape)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("AxisAlignedBoxShapeComponentTypeId", BehaviorConstant(AxisAlignedBoxShapeComponentTypeId));
        }
    }

    void AxisAlignedBoxShapeComponent::Activate()
    {
        m_aaboxShape.Activate(GetEntityId());
    }

    void AxisAlignedBoxShapeComponent::Deactivate()
    {
        m_aaboxShape.Deactivate();
    }

    bool AxisAlignedBoxShapeComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const BoxShapeConfig*>(baseConfig))
        {
            m_aaboxShape.SetBoxConfiguration(*config);
            return true;
        }
        return false;
    }

    bool AxisAlignedBoxShapeComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<BoxShapeConfig*>(outBaseConfig))
        {
            *config = m_aaboxShape.GetBoxConfiguration();
            return true;
        }
        return false;
    }

} // namespace LmbrCentral
