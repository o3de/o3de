/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
