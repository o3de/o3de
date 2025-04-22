/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RotateCameraLookAt.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include "StartingPointCamera/StartingPointCameraUtilities.h"

namespace Camera
{
    void RotateCameraLookAt::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<RotateCameraLookAt>()
                ->Version(2)
                ->Field("Axis Of Rotation", &RotateCameraLookAt::m_axisOfRotation)
                ->Field("Event Name", &RotateCameraLookAt::m_eventName)
                ->Field("Invert Axis", &RotateCameraLookAt::m_shouldInvertAxis)
                ->Field("Rotation Speed Scale", &RotateCameraLookAt::m_rotationSpeedScale);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<RotateCameraLookAt>("Rotate Camera Target"
                    , "This will rotate a Camera Target about Axis when the EventName fires")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &RotateCameraLookAt::m_axisOfRotation, "Axis Of Rotation",
                    "This is the direction vector that will be applied to the target's movement scaled for time")
                        ->EnumAttribute(AxisOfRotation::X_Axis, "Camera Target's X Axis")
                        ->EnumAttribute(AxisOfRotation::Y_Axis, "Camera Target's Y Axis")
                        ->EnumAttribute(AxisOfRotation::Z_Axis, "Camera Target's Z Axis")
                    ->DataElement(0, &RotateCameraLookAt::m_eventName, "Event Name", "The Name of the expected Event")
                    ->DataElement(0, &RotateCameraLookAt::m_shouldInvertAxis, "Invert Axis", "True if you want to rotate along a negative axis")
                    ->DataElement(0, &RotateCameraLookAt::m_rotationSpeedScale, "Rotation Speed Scale", "Scale greater than 1 to speed up, between 0 and 1 to slow down")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshAttributesAndValues"));
            }
        }
    }

    void RotateCameraLookAt::AdjustLookAtTarget([[maybe_unused]] float deltaTime, [[maybe_unused]] const AZ::Transform& targetTransform, AZ::Transform& outLookAtTargetTransform)
    {
        float axisPolarity = m_shouldInvertAxis ? -1.0f : 1.0f;
        float rotationAmount = axisPolarity * m_rotationAmount;

        AZ::Quaternion desiredRotation = AZ::Quaternion::CreateFromAxisAngle(
            outLookAtTargetTransform.GetBasis(m_axisOfRotation), rotationAmount);
        outLookAtTargetTransform.SetRotation(desiredRotation * outLookAtTargetTransform.GetRotation());
    }

    void RotateCameraLookAt::Activate(AZ::EntityId entityId)
    {
        m_rigEntity = entityId;
        AZ::Crc32 eventNameCrc = AZ::Crc32(m_eventName.c_str());
        AZ::GameplayNotificationId actionBusId(m_rigEntity, eventNameCrc);
        AZ::GameplayNotificationBus::Handler::BusConnect(actionBusId);
    }

    void RotateCameraLookAt::Deactivate()
    {
        AZ::Crc32 eventNameCrc = AZ::Crc32(m_eventName.c_str());
        AZ::GameplayNotificationId actionBusId(m_rigEntity, eventNameCrc);
        AZ::GameplayNotificationBus::Handler::BusDisconnect(actionBusId);
    }

    void RotateCameraLookAt::OnEventBegin(const AZStd::any& value)
    {
        OnEventUpdating(value);
    }
    void RotateCameraLookAt::OnEventUpdating(const AZStd::any& value)
    {
        float frameTime = 0.0f;
        AZ::TickRequestBus::BroadcastResult(frameTime, &AZ::TickRequestBus::Events::GetTickDeltaTime);

        float floatValue = 0.0f;
        AZ_Warning("RotateCameraLookAt", AZStd::any_numeric_cast<float>(&value, floatValue), "Received bad value, expected type numerically convertable to float, got type %s", GetNameFromUuid(value.type()));
        if (AZStd::any_numeric_cast<float>(&value, floatValue))
        {
            m_rotationAmount += floatValue * frameTime * m_rotationSpeedScale;
        }
    }
    void RotateCameraLookAt::OnEventEnd(const AZStd::any&)
    {
        m_rotationAmount = 0.f;
    }
} // namespace Camera
