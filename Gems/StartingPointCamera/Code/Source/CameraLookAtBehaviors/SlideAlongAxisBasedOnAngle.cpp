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
#include <AzFramework/Translation/TranslationDef.h>

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
                editContext->Class<SlideAlongAxisBasedOnAngle>(
                    QT_TRANSLATE_NOOP("Camera", "SlideAlongAxisBasedOnAngle"),
                    QT_TRANSLATE_NOOP("Camera", "Slide 0..SlideDistance along Axis based on Angle Type.  Maps from 90..-90 degrees"))
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SlideAlongAxisBasedOnAngle::m_axisToSlideAlong,
                            QT_TRANSLATE_NOOP("Camera", "Axis to slide along"),
                            QT_TRANSLATE_NOOP("Camera", "The Axis to slide along"))
                            ->EnumAttribute(RelativeAxisType::ForwardBackward, QT_TRANSLATE_NOOP("Camera", "Forwards and Backwards"))
                            ->EnumAttribute(RelativeAxisType::LeftRight, QT_TRANSLATE_NOOP("Camera", "Right and Left"))
                            ->EnumAttribute(RelativeAxisType::UpDown, QT_TRANSLATE_NOOP("Camera", "Up and Down"))
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &SlideAlongAxisBasedOnAngle::m_angleTypeToChangeFor,
                            QT_TRANSLATE_NOOP("Camera", "Angle Type"),
                            QT_TRANSLATE_NOOP("Camera", "The angle type to base the slide off of"))
                            ->EnumAttribute(EulerAngleType::Pitch, QT_TRANSLATE_NOOP("Camera", "Pitch"))
                            ->EnumAttribute(EulerAngleType::Roll, QT_TRANSLATE_NOOP("Camera", "Roll"))
                            ->EnumAttribute(EulerAngleType::Yaw, QT_TRANSLATE_NOOP("Camera", "Yaw"))
                        ->DataElement(0, &SlideAlongAxisBasedOnAngle::m_maximumPositiveSlideDistance,
                            QT_TRANSLATE_NOOP("Camera", "Max Positive Slide Distance"),
                            QT_TRANSLATE_NOOP("Camera", "The maximum distance to slide in the positive"))
                            ->Attribute(AZ::Edit::Attributes::Suffix, QT_TRANSLATE_NOOP("Camera", "m"))
                        ->DataElement(0, &SlideAlongAxisBasedOnAngle::m_maximumNegativeSlideDistance,
                            QT_TRANSLATE_NOOP("Camera", "Max Negative Slide Distance"),
                            QT_TRANSLATE_NOOP("Camera", "The maximum distance to slide in the negative"))
                            ->Attribute(AZ::Edit::Attributes::Suffix, QT_TRANSLATE_NOOP("Camera", "m"))
                        ->ClassElement(AZ::Edit::ClassElements::Group, QT_TRANSLATE_NOOP("Camera", "Vector Components To Ignore"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(0, &SlideAlongAxisBasedOnAngle::m_ignoreX,
                                QT_TRANSLATE_NOOP("Camera", "X"),
                                QT_TRANSLATE_NOOP("Camera", "When active, the X Component will be ignored."))
                                ->Attribute(AZ::Edit::Attributes::ReadOnly, &SlideAlongAxisBasedOnAngle::YAndZIgnored)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                            ->DataElement(0, &SlideAlongAxisBasedOnAngle::m_ignoreY,
                                QT_TRANSLATE_NOOP("Camera", "Y"),
                                QT_TRANSLATE_NOOP("Camera", "When active, the Y Component will be ignored."))
                                ->Attribute(AZ::Edit::Attributes::ReadOnly, &SlideAlongAxisBasedOnAngle::XAndZIgnored)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                            ->DataElement(0, &SlideAlongAxisBasedOnAngle::m_ignoreZ,
                                QT_TRANSLATE_NOOP("Camera", "Z"),
                                QT_TRANSLATE_NOOP("Camera", "When active, the Z Component will be ignored."))
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
