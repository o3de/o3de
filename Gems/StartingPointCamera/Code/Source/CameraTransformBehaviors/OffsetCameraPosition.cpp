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
#include "OffsetCameraPosition.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace Camera
{
    void OffsetCameraPosition::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<OffsetCameraPosition>()
                ->Version(1)
                ->Field("Offset", &OffsetCameraPosition::m_offset)
                ->Field("Is Offset Relative", &OffsetCameraPosition::m_isRelativeOffset);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<OffsetCameraPosition>("Offset Position", "Offset the Camera's position")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(0, &OffsetCameraPosition::m_offset, "Offset", "The displacement you wish to move the Camera by")
                        ->Attribute(AZ::Edit::Attributes::Suffix, "m")
                    ->DataElement(0, &OffsetCameraPosition::m_isRelativeOffset, "Is Offset Relative", "If yes then the displacement will occur from the perspective of the camera");
            }
        }
    }

    void OffsetCameraPosition::AdjustCameraTransform([[maybe_unused]] float deltaTime, [[maybe_unused]] const AZ::Transform& initialCameraTransform, const AZ::Transform& targetTransform, AZ::Transform& inOutCameraTransform)
    {
        AZ::Vector3 currentPosition = targetTransform.GetTranslation();
        inOutCameraTransform.SetTranslation(AZ::Vector3::CreateZero());
        AZ::Transform rotation = m_isRelativeOffset ? inOutCameraTransform : AZ::Transform::CreateIdentity();
        inOutCameraTransform.SetTranslation(currentPosition + rotation.TransformPoint(m_offset));
    }
} //namespace Camera
