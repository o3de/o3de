/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        size_t actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(m_actorInstance);
        if (actorInstanceID != InvalidIndex)
        {
            m_actorInstance->SetLocalSpacePosition(value);
        }
    }

    void TranslateManipulatorCallback::UpdateOldValues()
    {
        // update the rotation, if actorinstance is still valid
        size_t actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(m_actorInstance);
        if (actorInstanceID != InvalidIndex)
        {
            m_oldValueVec = m_actorInstance->GetLocalSpaceTransform().m_position;
        }
    }

    void TranslateManipulatorCallback::ApplyTransformation()
    {
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance)
        {
            const AZ::Vector3 newPos = actorInstance->GetLocalSpaceTransform().m_position;
            actorInstance->SetLocalSpacePosition(m_oldValueVec);

            if ((m_oldValueVec - newPos).GetLength() >= MCore::Math::epsilon)
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
        size_t actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(m_actorInstance);
        if (actorInstanceID != InvalidIndex)
        {
            // temporarily update the actor instance
            m_actorInstance->SetLocalSpaceRotation(value * m_actorInstance->GetLocalSpaceTransform().m_rotation.GetNormalized());

            // update the callback parent
            ManipulatorCallback::Update(m_actorInstance->GetLocalSpaceTransform().m_rotation);
        }
    }

    void RotateManipulatorCallback::UpdateOldValues()
    {
        // update the rotation, if actorinstance is still valid
        size_t actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(m_actorInstance);
        if (actorInstanceID != InvalidIndex)
        {
            m_oldValueQuat = m_actorInstance->GetLocalSpaceTransform().m_rotation;
        }
    }

    void RotateManipulatorCallback::ApplyTransformation()
    {
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance)
        {
            const AZ::Quaternion newRot = actorInstance->GetLocalSpaceTransform().m_rotation;
            actorInstance->SetLocalSpaceRotation(m_oldValueQuat);

            const float dot = newRot.Dot(m_oldValueQuat);
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
        size_t actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(m_actorInstance);
        if (actorInstanceID != InvalidIndex)
        {
            #ifndef EMFX_SCALE_DISABLED
                return m_actorInstance->GetLocalSpaceTransform().m_scale;
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
            size_t actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(m_actorInstance);
            if (actorInstanceID != InvalidIndex)
            {
                float minScale = 0.001f;
                const AZ::Vector3 scale = AZ::Vector3(
                        MCore::Max(float(m_oldValueVec.GetX() * value.GetX()), minScale),
                        MCore::Max(float(m_oldValueVec.GetY() * value.GetY()), minScale),
                        MCore::Max(float(m_oldValueVec.GetZ() * value.GetZ()), minScale));

                m_actorInstance->SetLocalSpaceScale(scale);

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
            size_t actorInstanceID = EMotionFX::GetActorManager().FindActorInstanceIndex(m_actorInstance);
            if (actorInstanceID != InvalidIndex)
            {
                m_oldValueVec = m_actorInstance->GetLocalSpaceTransform().m_scale;
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
                AZ::Vector3 newScale = actorInstance->GetLocalSpaceTransform().m_scale;
                actorInstance->SetLocalSpaceScale(m_oldValueVec);

                if ((m_oldValueVec - newScale).GetLength() >= MCore::Math::epsilon)
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
