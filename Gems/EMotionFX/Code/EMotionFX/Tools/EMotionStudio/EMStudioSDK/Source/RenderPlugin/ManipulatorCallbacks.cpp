/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "ManipulatorCallbacks.h"

#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/ActorManager.h>

namespace EMStudio
{
    void TranslateManipulatorCallback::Update(const AZ::Vector3& value)
    {
        ManipulatorCallback::Update(value);

        // update the position, if actorinstance is still valid
        uint32 actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(mActorInstance);
        if (actorInstanceID != MCORE_INVALIDINDEX32)
        {
            mActorInstance->SetLocalSpacePosition(value);
        }
    }

    void TranslateManipulatorCallback::UpdateOldValues()
    {
        // update the rotation, if actorinstance is still valid
        uint32 actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(mActorInstance);
        if (actorInstanceID != MCORE_INVALIDINDEX32)
        {
            mOldValueVec = mActorInstance->GetLocalSpaceTransform().mPosition;
        }
    }

    void TranslateManipulatorCallback::ApplyTransformation()
    {
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance)
        {
            const AZ::Vector3 newPos = actorInstance->GetLocalSpaceTransform().mPosition;
            actorInstance->SetLocalSpacePosition(mOldValueVec);

            if ((mOldValueVec - newPos).GetLength() >= MCore::Math::epsilon)
            {
                AZStd::string outResult;
                if (GetCommandManager()->ExecuteCommand(
                        AZStd::string::format("AdjustActorInstance -actorInstanceID %i -pos %s", actorInstance->GetID(), AZStd::to_string(newPos).c_str()),
                        outResult) == false)
                {
                    MCore::LogError(outResult.c_str());
                }
                else
                {
                    UpdateOldValues();
                }
            }
        }
    }

    void RotateManipulatorCallback::Update(const AZ::Quaternion& value)
    {
        // update the rotation, if actorinstance is still valid
        uint32 actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(mActorInstance);
        if (actorInstanceID != MCORE_INVALIDINDEX32)
        {
            // temporarily update the actor instance
            mActorInstance->SetLocalSpaceRotation(value * mActorInstance->GetLocalSpaceTransform().mRotation.GetNormalized());

            // update the callback parent
            ManipulatorCallback::Update(mActorInstance->GetLocalSpaceTransform().mRotation);
        }
    }

    void RotateManipulatorCallback::UpdateOldValues()
    {
        // update the rotation, if actorinstance is still valid
        uint32 actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(mActorInstance);
        if (actorInstanceID != MCORE_INVALIDINDEX32)
        {
            mOldValueQuat = mActorInstance->GetLocalSpaceTransform().mRotation;
        }
    }

    void RotateManipulatorCallback::ApplyTransformation()
    {
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance)
        {
            const AZ::Quaternion newRot = actorInstance->GetLocalSpaceTransform().mRotation;
            actorInstance->SetLocalSpaceRotation(mOldValueQuat);

            const float dot = newRot.Dot(mOldValueQuat);
            if (dot < 1.0f - MCore::Math::epsilon && dot > -1.0f + MCore::Math::epsilon)
            {
                AZStd::string outResult;
                if (GetCommandManager()->ExecuteCommand(
                        AZStd::string::format("AdjustActorInstance -actorInstanceID %i -rot \"%s\"",
                            actorInstance->GetID(),
                            AZStd::to_string(newRot).c_str()).c_str(),
                        outResult) == false)
                {
                    MCore::LogError(outResult.c_str());
                }
                else
                {
                    UpdateOldValues();
                }
            }
        }
    }

    AZ::Vector3 ScaleManipulatorCallback::GetCurrValueVec()
    {
        uint32 actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(mActorInstance);
        if (actorInstanceID != MCORE_INVALIDINDEX32)
        {
            #ifndef EMFX_SCALE_DISABLED
                return mActorInstance->GetLocalSpaceTransform().mScale;
            #else
                return AZ::Vector3::CreateOne();
            #endif
        }
        else
        {
            return AZ::Vector3::CreateOne();
        }
    }

    void ScaleManipulatorCallback::Update(const AZ::Vector3& value)
    {
        EMFX_SCALECODE
        (
            // update the position, if actorinstance is still valid
            uint32 actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(mActorInstance);
            if (actorInstanceID != MCORE_INVALIDINDEX32)
            {
                float minScale = 0.001f;
                const AZ::Vector3 scale = AZ::Vector3(
                        MCore::Max(float(mOldValueVec.GetX() * value.GetX()), minScale),
                        MCore::Max(float(mOldValueVec.GetY() * value.GetY()), minScale),
                        MCore::Max(float(mOldValueVec.GetZ() * value.GetZ()), minScale));

                mActorInstance->SetLocalSpaceScale(scale);

                // update the callback
                ManipulatorCallback::Update(scale);
            }
        )
    }

    void ScaleManipulatorCallback::UpdateOldValues()
    {
        EMFX_SCALECODE
        (
            // update the rotation, if actorinstance is still valid
            uint32 actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(mActorInstance);
            if (actorInstanceID != MCORE_INVALIDINDEX32)
            {
                mOldValueVec = mActorInstance->GetLocalSpaceTransform().mScale;
            }
        )
    }

    void ScaleManipulatorCallback::ApplyTransformation()
    {
        EMFX_SCALECODE
        (
            EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
            if (actorInstance)
            {
                AZ::Vector3 newScale = actorInstance->GetLocalSpaceTransform().mScale;
                actorInstance->SetLocalSpaceScale(mOldValueVec);

                if ((mOldValueVec - newScale).GetLength() >= MCore::Math::epsilon)
                {
                    AZStd::string outResult;
                    if (GetCommandManager()->ExecuteCommand(
                            AZStd::string::format("AdjustActorInstance -actorInstanceID %i -scale %s", actorInstance->GetID(),
                                AZStd::to_string(newScale).c_str()).c_str(),
                            outResult) == false)
                    {
                        MCore::LogError(outResult.c_str());
                    }
                    else
                    {
                        UpdateOldValues();
                    }
                }
            }
        )
    }
} // namespace EMStudio
