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
        if (const auto config = azrtti_cast<const AxisAlignedBoxShapeConfig*>(baseConfig))
        {
            m_boxShapeConfig = *config;
            return true;
        }
        return false;
    }

    bool AxisAlignedBoxShapeDebugDisplayComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<AxisAlignedBoxShapeConfig*>(outBaseConfig))
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
            AxisAlignedBoxShapeComponentRequestsBus::EventResult(m_boxShapeConfig, GetEntityId(), &AxisAlignedBoxShapeComponentRequests::GetBoxConfiguration);
            AZ::NonUniformScaleRequestBus::EventResult(m_nonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);
        }
    }

    void AxisAlignedBoxShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AxisAlignedBoxShapeConfig, ShapeComponentConfig>()
                ->Version(1)
                ->Field("Dimensions", &AxisAlignedBoxShapeConfig::m_dimensions)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AxisAlignedBoxShapeConfig>("Configuration", "Axis Aligned Box shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AxisAlignedBoxShapeConfig::m_dimensions, "Dimensions", "Dimensions of the box along its axes")
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
                        ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AxisAlignedBoxShapeConfig>()
                ->Constructor()
                ->Constructor<AZ::Vector3&>()
                ->Property("Dimensions", BehaviorValueProperty(&AxisAlignedBoxShapeConfig::m_dimensions))
                ;
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

            behaviorContext->EBus<AxisAlignedBoxShapeComponentRequestsBus>("AxisAlignedBoxShapeComponentRequestsBus")
                ->Event("GetBoxConfiguration", &AxisAlignedBoxShapeComponentRequestsBus::Events::GetBoxConfiguration)
                ->Event("GetBoxDimensions", &AxisAlignedBoxShapeComponentRequestsBus::Events::GetBoxDimensions)
                ->Event("SetBoxDimensions", &AxisAlignedBoxShapeComponentRequestsBus::Events::SetBoxDimensions)
                ;
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
        if (const auto config = azrtti_cast<const AxisAlignedBoxShapeConfig*>(baseConfig))
        {
            m_aaboxShape.SetBoxConfiguration(*config);
            return true;
        }
        return false;
    }

    bool AxisAlignedBoxShapeComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<AxisAlignedBoxShapeConfig*>(outBaseConfig))
        {
            *config = m_aaboxShape.GetBoxConfiguration();
            return true;
        }
        return false;
    }

} // namespace LmbrCentral
