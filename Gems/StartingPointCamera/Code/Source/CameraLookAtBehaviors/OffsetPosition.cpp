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
#include "OffsetPosition.h"
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Quaternion.h>

namespace Camera
{
    void OffsetPosition::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<OffsetPosition>()
                ->Version(1)
                ->Field("Positional Offset", &OffsetPosition::m_positionalOffset)
                ->Field("Offset Is Relative", &OffsetPosition::m_isRelativeOffset);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<OffsetPosition>("OffsetPosition", "Offset the acquired position of the camera's current target")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(0, &OffsetPosition::m_positionalOffset, "Positional Offset", "The vector offset from the current position")
                    ->DataElement(0, &OffsetPosition::m_isRelativeOffset, "Offset Is Relative", "Uses world coordinates for the offset when false and local coordinates when true");
            }
        }
    }

    void OffsetPosition::AdjustLookAtTarget([[maybe_unused]] float deltaTime, [[maybe_unused]] const AZ::Transform& targetTransform, AZ::Transform& outLookAtTargetTransform)
    {
        if (m_isRelativeOffset)
        {
            outLookAtTargetTransform.SetTranslation(outLookAtTargetTransform.GetTranslation() + outLookAtTargetTransform.GetRotation().TransformVector(m_positionalOffset));
        }
        else
        {
            outLookAtTargetTransform.SetTranslation(outLookAtTargetTransform.GetTranslation() + m_positionalOffset);
        }
    }
} // namespace Camera
