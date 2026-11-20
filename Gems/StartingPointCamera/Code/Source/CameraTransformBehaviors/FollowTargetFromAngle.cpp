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
#include <AzFramework/Translation/TranslationDef.h>
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
                editContext->Class<FollowTargetFromAngle>(
                    QT_TRANSLATE_NOOP("Camera", "FollowTargetFromAngle"),
                    QT_TRANSLATE_NOOP("Camera", "Follows behind the target by Angle degrees about RotationType"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(0, &FollowTargetFromAngle::m_angleInDegrees,
                        QT_TRANSLATE_NOOP("Camera", "Angle"),
                        QT_TRANSLATE_NOOP("Camera", "The angle to rotate about RotationType"))
                        ->Attribute(AZ::Edit::Attributes::Suffix, QT_TRANSLATE_NOOP("Camera", "degrees"))
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &FollowTargetFromAngle::m_rotationType,
                        QT_TRANSLATE_NOOP("Camera", "Rotation Type"),
                        QT_TRANSLATE_NOOP("Camera", "Choose to Yaw, Pitch or Roll Angle degrees"))
                        ->EnumAttribute(EulerAngleType::Yaw, QT_TRANSLATE_NOOP("Camera", "Yaw"))
                        ->EnumAttribute(EulerAngleType::Pitch, QT_TRANSLATE_NOOP("Camera", "Pitch"))
                        ->EnumAttribute(EulerAngleType::Roll, QT_TRANSLATE_NOOP("Camera", "Roll"))
                    ->DataElement(0, &FollowTargetFromAngle::m_distanceFromTarget,
                        QT_TRANSLATE_NOOP("Camera", "Distance From Target"),
                        QT_TRANSLATE_NOOP("Camera", "The range at which to follow the target from"))
                        ->Attribute(AZ::Edit::Attributes::Suffix, QT_TRANSLATE_NOOP("Camera", "m"));
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
