/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FollowTargetFromDistance.h"
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include "StartingPointCamera/StartingPointCameraConstants.h"
#include "StartingPointCamera/StartingPointCameraUtilities.h"


namespace Camera
{
    void FollowTargetFromDistance::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<FollowTargetFromDistance>()
                ->Version(2)
                ->Field("Follow Distance", &FollowTargetFromDistance::m_followDistance)
                ->Field("Minimum Follow Distance", &FollowTargetFromDistance::m_minFollowDistance)
                ->Field("Maximum Follow Distance", &FollowTargetFromDistance::m_maxFollowDistance)
                ->Field("Zoom In Event Name", &FollowTargetFromDistance::m_zoomInEventName)
                ->Field("Zoom Out Event Name", &FollowTargetFromDistance::m_zoomOutEventName)
                ->Field("Zoom Speed Scale", &FollowTargetFromDistance::m_zoomSpeedScale)
                ->Field("Input Source Entity", &FollowTargetFromDistance::m_channelId);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<FollowTargetFromDistance>("FollowTargetFromDistance", "Follows behind the target by Follow Distance meters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(0, &FollowTargetFromDistance::m_followDistance, "Follow Distance", "The distance to follow behind the target in meters")
                        ->Attribute(AZ::Edit::Attributes::Suffix, "m")
                        ->Attribute(AZ::Edit::Attributes::Min, &FollowTargetFromDistance::GetMinimumFollowDistance)
                        ->Attribute(AZ::Edit::Attributes::Max, &FollowTargetFromDistance::GetMaximumFollowDistance)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshAttributesAndValues"))
                    ->DataElement(0, &FollowTargetFromDistance::m_minFollowDistance, "Minimum Follow Distance", "The MINIMUM distance to follow the behind the target in meters")
                        ->Attribute(AZ::Edit::Attributes::Suffix, "m")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, &FollowTargetFromDistance::GetMaximumFollowDistance)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshAttributesAndValues"))
                    ->DataElement(0, &FollowTargetFromDistance::m_maxFollowDistance, "Maximum Follow Distance", "The MAXIMUM distance to follow the behind the target in meters")
                        ->Attribute(AZ::Edit::Attributes::Suffix, "m")
                        ->Attribute(AZ::Edit::Attributes::Min, &FollowTargetFromDistance::GetMinimumFollowDistance)
                        ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshAttributesAndValues"))
                    ->DataElement(0, &FollowTargetFromDistance::m_zoomInEventName, "Zoom In Event Name", "The name of the event to trigger a zoom in")
                    ->DataElement(0, &FollowTargetFromDistance::m_zoomOutEventName, "Zoom Out Event Name", "The name of the event to trigger a zoom out")
                    ->DataElement(0, &FollowTargetFromDistance::m_zoomSpeedScale, "Zoom Speed Scale", "The amount to scale the incoming zoom event by");
            }
        }
    }

    void FollowTargetFromDistance::AdjustCameraTransform(float /*deltaTime*/, const AZ::Transform& /*initialCameraTransform*/, const AZ::Transform& targetTransform, AZ::Transform& inOutCameraTransform)
    {
        inOutCameraTransform.SetTranslation(targetTransform.GetTranslation() - targetTransform.GetBasis(ForwardBackward) * m_followDistance);
    }

    void FollowTargetFromDistance::Activate(AZ::EntityId channelId)
    {
        m_channelId = channelId;
        if (!m_zoomInEventName.empty())
        {
            AZ::GameplayNotificationId busId(m_channelId, AZ_CRC(m_zoomInEventName.c_str()));
            AZ::GameplayNotificationBus::MultiHandler::BusConnect(busId);
        }

        if (!m_zoomOutEventName.empty())
        {
            AZ::GameplayNotificationId busId(m_channelId, AZ_CRC(m_zoomOutEventName.c_str()));
            AZ::GameplayNotificationBus::MultiHandler::BusConnect(busId);
        }
    }

    void FollowTargetFromDistance::Deactivate()
    {
        if (!m_zoomInEventName.empty())
        {
            AZ::GameplayNotificationId busId(m_channelId, AZ_CRC(m_zoomInEventName.c_str()));
            AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(busId);
        }

        if (!m_zoomOutEventName.empty())
        {
            AZ::GameplayNotificationId busId(m_channelId, AZ_CRC(m_zoomOutEventName.c_str()));
            AZ::GameplayNotificationBus::MultiHandler::BusDisconnect(busId);
        }
    }

    void FollowTargetFromDistance::OnEventBegin(const AZStd::any& value)
    {
        float floatValue = 0.0f;
        AZ_Warning("FollowTargetFromDistance", AZStd::any_numeric_cast<float>(&value, floatValue), "Received bad value, expected type numerically convertable to float, got type %s", GetNameFromUuid(value.type()));
        if (AZStd::any_numeric_cast<float>(&value, floatValue))
        {
            m_followDistance = AZ::GetClamp(m_followDistance - (floatValue * m_zoomSpeedScale), m_minFollowDistance, m_maxFollowDistance);
        }
    }
} // namespace Camera

