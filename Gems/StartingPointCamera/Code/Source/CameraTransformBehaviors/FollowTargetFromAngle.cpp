/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FollowTargetFromAngle.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "StartingPointCamera/StartingPointCameraUtilities.h"

namespace Camera
{
    void FollowTargetFromAngle::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<FollowTargetFromAngle>()
                ->Version(1)
                ->Field("Angle", &FollowTargetFromAngle::m_angleInDegrees)
                ->Field("Rotation Type", &FollowTargetFromAngle::m_rotationType)
                ->Field("Distance From Target", &FollowTargetFromAngle::m_distanceFromTarget);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<FollowTargetFromAngle>("FollowTargetFromAngle", "Follows behind the target by Angle degrees about RotationType")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(0, &FollowTargetFromAngle::m_angleInDegrees, "Angle", "The angle to rotate about RotationType")
                        ->Attribute(AZ::Edit::Attributes::Suffix, "degrees")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FollowTargetFromAngle::m_rotationType, "Rotation Type", "Choose to Yaw, Pitch or Roll Angle degrees")
                        ->EnumAttribute(EulerAngleType::Yaw, "Yaw")
                        ->EnumAttribute(EulerAngleType::Pitch, "Pitch")
                        ->EnumAttribute(EulerAngleType::Roll, "Roll")
                    ->DataElement(0, &FollowTargetFromAngle::m_distanceFromTarget, "Distance From Target", "The range at which to follow the target from")
                        ->Attribute(AZ::Edit::Attributes::Suffix, "m");
            }
        }
    }

    void FollowTargetFromAngle::AdjustCameraTransform([[maybe_unused]] float deltaTime, [[maybe_unused]] const AZ::Transform& initialCameraTransform, const AZ::Transform& targetTransform, AZ::Transform& inOutCameraTransform)
    {
        // calculate new position based on angles and distance
        AZ::Transform rotation = CreateRotationFromEulerAngle(m_rotationType, AZ::DegToRad(m_angleInDegrees));
        inOutCameraTransform = rotation;
        inOutCameraTransform.SetTranslation(targetTransform.GetTranslation() - rotation.GetBasis(ForwardBackward) * m_distanceFromTarget);
    }
} //namespace Camera
