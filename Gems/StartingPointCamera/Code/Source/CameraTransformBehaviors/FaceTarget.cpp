/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FaceTarget.h"
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/EditContext.h>
#include "StartingPointCamera/StartingPointCameraConstants.h"
#include <AzCore/Math/Quaternion.h>
#include <MathConversion.h>
#include <StartingPointCamera/StartingPointCameraUtilities.h>


namespace Camera
{
    void FaceTarget::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<FaceTarget>()
                ->Version(1)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<FaceTarget>("FaceTarget", "Causes the camera to face the target")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "");
            }
        }
    }

    void FaceTarget::AdjustCameraTransform(float /*deltaTime*/, [[maybe_unused]] const AZ::Transform& initialCameraTransform, const AZ::Transform& targetTransform, AZ::Transform& inOutCameraTransform)
    {
        AZ::Vector3 newLookVector = (targetTransform.GetTranslation() - inOutCameraTransform.GetTranslation()).GetNormalized();
        if (newLookVector.GetLengthSq() < AZ::Constants::FloatEpsilon)
        {
            newLookVector = targetTransform.GetBasis(ForwardBackward);
        }
        inOutCameraTransform.SetRotation(CreateQuaternionFromViewVector(newLookVector.GetNormalized()));
    }
} // namespace Camera

