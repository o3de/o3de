/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DiskShapeComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace LmbrCentral
{
    void DiskShapeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ShapeService"));
        provided.push_back(AZ_CRC_CE("DiskShapeService"));
    }

    void DiskShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ShapeService"));
        incompatible.push_back(AZ_CRC_CE("DiskShapeService"));
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void DiskShapeComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void DiskShapeDebugDisplayComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DiskShapeDebugDisplayComponent, EntityDebugDisplayComponent>()
                ->Version(1)
                ->Field("Configuration", &DiskShapeDebugDisplayComponent::m_diskShapeConfig)
                ;
        }
    }

    void DiskShapeDebugDisplayComponent::Activate()
    {
        EntityDebugDisplayComponent::Activate();
        ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
    }

    void DiskShapeDebugDisplayComponent::Deactivate()
    {
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
        EntityDebugDisplayComponent::Deactivate();
    }

    void DiskShapeDebugDisplayComponent::Draw(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DrawDiskShape(m_diskShapeConfig.GetDrawParams(), m_diskShapeConfig, debugDisplay);
    }

    bool DiskShapeDebugDisplayComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const DiskShapeConfig*>(baseConfig))
        {
            m_diskShapeConfig = *config;
            return true;
        }
        return false;
    }

    bool DiskShapeDebugDisplayComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<DiskShapeConfig*>(outBaseConfig))
        {
            *outConfig = m_diskShapeConfig;
            return true;
        }
        return false;
    }

    void DiskShapeDebugDisplayComponent::OnShapeChanged(ShapeChangeReasons changeReason)
    {
        if (changeReason == ShapeChangeReasons::ShapeChanged)
        {
            DiskShapeComponentRequestBus::EventResult(m_diskShapeConfig, GetEntityId(), &DiskShapeComponentRequests::GetDiskConfiguration);
        }
    }

    void DiskShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DiskShapeConfig, ShapeComponentConfig>()
                ->Version(1)
                ->Field("Radius", &DiskShapeConfig::m_radius)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<DiskShapeConfig>("Configuration", "Disk shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &DiskShapeConfig::m_radius, "Radius", "Radius of disk")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                    ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<DiskShapeConfig>()
                ->Constructor()
                ->Constructor<float>()
                ->Property("Radius", BehaviorValueProperty(&DiskShapeConfig::m_radius))
                ;
        }
    }

    void DiskShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        DiskShape::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DiskShapeComponent, AZ::Component>()
                ->Version(1)
                ->Field("DiskShape", &DiskShapeComponent::m_diskShape)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("DiskShapeComponentTypeId", BehaviorConstant(DiskShapeComponentTypeId));

            behaviorContext->EBus<DiskShapeComponentRequestBus>("DiskShapeComponentRequestsBus")
                ->Event("GetDiskConfiguration", &DiskShapeComponentRequestBus::Events::GetDiskConfiguration)
                ->Event("SetRadius", &DiskShapeComponentRequestBus::Events::SetRadius)
                ->Event("GetRadius", &DiskShapeComponentRequestBus::Events::GetRadius)
                ;
        }
    }

    void DiskShapeComponent::Activate()
    {
        m_diskShape.Activate(GetEntityId());
    }

    void DiskShapeComponent::Deactivate()
    {
        m_diskShape.Deactivate();
    }

    bool DiskShapeComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const DiskShapeConfig*>(baseConfig))
        {
            m_diskShape.SetDiskConfiguration(*config);
            return true;
        }
        return false;
    }

    bool DiskShapeComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<DiskShapeConfig*>(outBaseConfig))
        {
            *outConfig = m_diskShape.GetDiskConfiguration();
            return true;
        }
        return false;
    }
} // namespace LmbrCentral
