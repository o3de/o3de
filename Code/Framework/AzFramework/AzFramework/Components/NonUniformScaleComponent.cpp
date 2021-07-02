/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/NonUniformScaleComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/ToString.h>
#include <AzCore/Component/Entity.h>

namespace AzFramework
{
    void NonUniformScaleComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<NonUniformScaleComponent, AZ::Component>()
                ->Version(1)
                ->Field("NonUniformScale", &NonUniformScaleComponent::m_scale)
                ;
        }
    }

    void NonUniformScaleComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("TransformService"));
    }

    void NonUniformScaleComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void NonUniformScaleComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void NonUniformScaleComponent::Activate()
    {
        AZ::NonUniformScaleRequestBus::Handler::BusConnect(GetEntityId());
    }

    void NonUniformScaleComponent::Deactivate()
    {
        AZ::NonUniformScaleRequestBus::Handler::BusDisconnect();
    }

    AZ::Vector3 NonUniformScaleComponent::GetScale() const
    {
        return m_scale;
    }

    void NonUniformScaleComponent::SetScale(const AZ::Vector3& scale)
    {
        if (scale.GetMinElement() >= AZ::MinTransformScale && scale.GetMaxElement() <= AZ::MaxTransformScale)
        {
            m_scale = scale;
        }
        else
        {
            AZ::Vector3 clampedScale = scale.GetClamp(AZ::Vector3(AZ::MinTransformScale), AZ::Vector3(AZ::MaxTransformScale));
            AZ_Warning("Non-uniform Scale Component", false, "SetScale value was clamped from %s to %s for entity %s",
                AZ::ToString(scale).c_str(), AZ::ToString(clampedScale).c_str(), GetEntity()->GetName().c_str());
            m_scale = clampedScale;
        }
        m_scaleChangedEvent.Signal(m_scale);
    }

    void NonUniformScaleComponent::RegisterScaleChangedEvent(AZ::NonUniformScaleChangedEvent::Handler& handler)
    {
        handler.Connect(m_scaleChangedEvent);
    }
} // namespace AzFramework
