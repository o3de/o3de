/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SlideAlongAxisBasedOnAngle.h"
#include "StartingPointCamera/StartingPointCameraUtilities.h"
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/EditContext.h>

namespace Camera
{
    void SlideAlongAxisBasedOnAngle::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<SlideAlongAxisBasedOnAngle>()
                ->Version(2)
                ->Field("Axis to slide along", &SlideAlongAxisBasedOnAngle::m_axisToSlideAlong)
                ->Field("Angle Type", &SlideAlongAxisBasedOnAngle::m_angleTypeToChangeFor)
                ->Field("Ignore X Component", &SlideAlongAxisBasedOnAngle::m_ignoreX)
                ->Field("Ignore Y Component", &SlideAlongAxisBasedOnAngle::m_ignoreY)
                ->Field("Ignore Z Component", &SlideAlongAxisBasedOnAngle::m_ignoreZ)
                ->Field("Max Positive Slide Distance", &SlideAlongAxisBasedOnAngle::m_maximumPositiveSlideDistance)
                ->Field("Max Negative Slide Distance", &SlideAlongAxisBasedOnAngle::m_maximumNegativeSlideDistance);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<SlideAlongAxisBasedOnAngle>("SlideAlongAxisBasedOnAngle",
                    "Slide 0..SlideDistance along Axis based on Angle Type.  Maps from 90..-90 degrees")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SlideAlongAxisBasedOnAngle::m_axisToSlideAlong, "Axis to slide along",
                            "The Axis to slide along")
                            ->EnumAttribute(RelativeAxisType::ForwardBackward, "Forwards and Backwards")
                            ->EnumAttribute(RelativeAxisType::LeftRight, "Right and Left")
                            ->EnumAttribute(RelativeAxisType::UpDown, "Up and Down")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SlideAlongAxisBasedOnAngle::m_angleTypeToChangeFor, "Angle Type",
                            "The angle type to base the slide off of")
                            ->EnumAttribute(EulerAngleType::Pitch, "Pitch")
                            ->EnumAttribute(EulerAngleType::Roll, "Roll")
                            ->EnumAttribute(EulerAngleType::Yaw, "Yaw")
                        ->DataElement(0, &SlideAlongAxisBasedOnAngle::m_maximumPositiveSlideDistance, "Max Positive Slide Distance",
                            "The maximum distance to slide in the positive")
                            ->Attribute(AZ::Edit::Attributes::Suffix, "m")
                        ->DataElement(0, &SlideAlongAxisBasedOnAngle::m_maximumNegativeSlideDistance, "Max Negative Slide Distance",
                            "The maximum distance to slide in the negative")
                            ->Attribute(AZ::Edit::Attributes::Suffix, "m")
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Vector Components To Ignore")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(0, &SlideAlongAxisBasedOnAngle::m_ignoreX, "X", "When active, the X Component will be ignored.")
                                ->Attribute(AZ::Edit::Attributes::ReadOnly, &SlideAlongAxisBasedOnAngle::YAndZIgnored)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                            ->DataElement(0, &SlideAlongAxisBasedOnAngle::m_ignoreY, "Y", "When active, the Y Component will be ignored.")
                                ->Attribute(AZ::Edit::Attributes::ReadOnly, &SlideAlongAxisBasedOnAngle::XAndZIgnored)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                            ->DataElement(0, &SlideAlongAxisBasedOnAngle::m_ignoreZ, "Z", "When active, the Z Component will be ignored.")
                                ->Attribute(AZ::Edit::Attributes::ReadOnly, &SlideAlongAxisBasedOnAngle::XAndYIgnored)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ;
            }
        }
    }

    void SlideAlongAxisBasedOnAngle::AdjustLookAtTarget(
        [[maybe_unused]] float deltaTime, [[maybe_unused]] const AZ::Transform& targetTransform, AZ::Transform& outLookAtTargetTransform)
    {
        float angle = GetEulerAngleFromTransform(outLookAtTargetTransform, m_angleTypeToChangeFor);
        float currentPositionOnRange = -angle / AZ::Constants::HalfPi;
        float slideScale = currentPositionOnRange > 0.0f ? m_maximumPositiveSlideDistance : m_maximumNegativeSlideDistance;

        AZ::Vector3 basis = outLookAtTargetTransform.GetBasis(m_axisToSlideAlong);
        MaskComponentFromNormalizedVector(basis, m_ignoreX, m_ignoreY, m_ignoreZ);

        outLookAtTargetTransform.SetTranslation(outLookAtTargetTransform.GetTranslation() + basis * currentPositionOnRange * slideScale);
    }

    bool SlideAlongAxisBasedOnAngle::XAndYIgnored() const
    {
        return m_ignoreX && m_ignoreY;
    }

    bool SlideAlongAxisBasedOnAngle::XAndZIgnored() const
    {
        return m_ignoreX && m_ignoreZ;
    }

    bool SlideAlongAxisBasedOnAngle::YAndZIgnored() const
    {
        return m_ignoreY && m_ignoreZ;
    }

} // namespace Camera
