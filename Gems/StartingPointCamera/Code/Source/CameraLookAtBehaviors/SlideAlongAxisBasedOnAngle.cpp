/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "StartingPointCamera_precompiled.h"
#include "SlideAlongAxisBasedOnAngle.h"
#include "StartingPointCamera/StartingPointCameraUtilities.h"
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Transform.h>

namespace Camera
{
    void SlideAlongAxisBasedOnAngle::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<SlideAlongAxisBasedOnAngle>()
                ->Version(1)
                ->Field("Axis to slide along", &SlideAlongAxisBasedOnAngle::m_axisToSlideAlong)
                ->Field("Angle Type", &SlideAlongAxisBasedOnAngle::m_angleTypeToChangeFor)
                ->Field("Vector Component To Ignore", &SlideAlongAxisBasedOnAngle::m_vectorComponentToIgnore)
                ->Field("Max Positive Slide Distance", &SlideAlongAxisBasedOnAngle::m_maximumPositiveSlideDistance)
                ->Field("Max Negative Slide Distance", &SlideAlongAxisBasedOnAngle::m_maximumNegativeSlideDistance);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<SlideAlongAxisBasedOnAngle>("SlideAlongAxisBasedOnAngle", "Slide 0..SlideDistance along Axis based on Angle Type.  Maps from 90..-90 degrees")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SlideAlongAxisBasedOnAngle::m_axisToSlideAlong, "Axis to slide along", "The Axis to slide along")
                        ->EnumAttribute(RelativeAxisType::ForwardBackward, "Forwards and Backwards")
                        ->EnumAttribute(RelativeAxisType::LeftRight, "Right and Left")
                        ->EnumAttribute(RelativeAxisType::UpDown, "Up and Down")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SlideAlongAxisBasedOnAngle::m_angleTypeToChangeFor, "Angle Type", "The angle type to base the slide off of")
                        ->EnumAttribute(EulerAngleType::Pitch, "Pitch")
                        ->EnumAttribute(EulerAngleType::Roll, "Roll")
                        ->EnumAttribute(EulerAngleType::Yaw, "Yaw")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SlideAlongAxisBasedOnAngle::m_vectorComponentToIgnore, "Vector Component To Ignore", "The Vector Component To Ignore")
                        ->EnumAttribute(VectorComponentType::None, "None")
                        ->EnumAttribute(VectorComponentType::X_Component, "X")
                        ->EnumAttribute(VectorComponentType::Y_Component, "Y")
                        ->EnumAttribute(VectorComponentType::Z_Component, "Z")
                    ->DataElement(0, &SlideAlongAxisBasedOnAngle::m_maximumPositiveSlideDistance, "Max Positive Slide Distance", "The maximum distance to slide in the positive")
                        ->Attribute(AZ::Edit::Attributes::Suffix, "m")
                    ->DataElement(0, &SlideAlongAxisBasedOnAngle::m_maximumNegativeSlideDistance, "Max Negative Slide Distance", "The maximum distance to slide in the negative")
                        ->Attribute(AZ::Edit::Attributes::Suffix, "m");
            }
        }
    }

    void SlideAlongAxisBasedOnAngle::AdjustLookAtTarget([[maybe_unused]] float deltaTime, [[maybe_unused]] const AZ::Transform& targetTransform, AZ::Transform& outLookAtTargetTransform)
    {
        float angle = GetEulerAngleFromTransform(outLookAtTargetTransform, m_angleTypeToChangeFor);
        float currentPositionOnRange = -angle / AZ::Constants::HalfPi;
        float slideScale = currentPositionOnRange > 0.0f ? m_maximumPositiveSlideDistance : m_maximumNegativeSlideDistance;

        AZ::Vector3 basis = outLookAtTargetTransform.GetBasis(m_axisToSlideAlong);
        MaskComponentFromNormalizedVector(basis, m_vectorComponentToIgnore);

        outLookAtTargetTransform.SetTranslation(outLookAtTargetTransform.GetTranslation() + basis * currentPositionOnRange * slideScale);
    }
}
