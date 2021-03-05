/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "StartingPointCamera_precompiled.h"
#include "Rotate.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "StartingPointCamera/StartingPointCameraUtilities.h"

namespace Camera
{
    void Rotate::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<Rotate>()
                ->Version(1)
                ->Field("Angle", &Rotate::m_angleInDegrees)
                ->Field("Axis", &Rotate::m_axisType);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<Rotate>("Rotate", "Rotate Camera Angle degrees about its Axis")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(0, &Rotate::m_angleInDegrees, "Angle", "The angle of rotation")
                        ->Attribute(AZ::Edit::Attributes::Suffix, "degrees")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &Rotate::m_axisType, "Axis", "The relative Axis of rotation")
                        ->EnumAttribute(AxisOfRotation::X_Axis, "X")
                        ->EnumAttribute(AxisOfRotation::Y_Axis, "Y")
                        ->EnumAttribute(AxisOfRotation::Z_Axis, "Z");
            }
        }
    }

    void Rotate::AdjustCameraTransform([[maybe_unused]] float deltaTime, [[maybe_unused]] const AZ::Transform& initialCameraTransform, [[maybe_unused]] const AZ::Transform& targetTransform, AZ::Transform& inOutCameraTransform)
    {
        AZ::Vector3 position = inOutCameraTransform.GetTranslation();
        inOutCameraTransform.SetTranslation(AZ::Vector3::CreateZero());
        AZ::Transform axisRotation = CreateRotationFromEulerAngle(static_cast<EulerAngleType>(m_axisType), AZ::DegToRad(m_angleInDegrees));
        inOutCameraTransform = inOutCameraTransform * axisRotation;
        inOutCameraTransform.SetTranslation(position);
    }
} // namespace Camera
