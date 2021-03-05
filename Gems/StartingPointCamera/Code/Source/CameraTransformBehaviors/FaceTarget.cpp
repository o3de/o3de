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

