/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <EMotionFX/Source/EMotionFXConfig.h>


namespace EMotionFX
{
    class Actor;
    class ActorInstance;
    class Pose;

    class EMFX_API PoseData
    {
    public:
        AZ_RTTI(PoseData, "{4F8BF249-8C9C-4F60-9642-8F1189D5CC7C}")
        AZ_CLASS_ALLOCATOR_DECL

        PoseData();
        virtual ~PoseData();

        virtual void LinkToActorInstance(const ActorInstance* actorInstance);
        virtual void LinkToActor(const Actor* actor);
        virtual void Reset() = 0;

        void SetPose(Pose* pose);
        PoseData& operator=(const PoseData& from);
        virtual void CopyFrom(const PoseData* from) = 0;

        virtual void Blend(const Pose* destPose, float weight) = 0;

        bool IsUsed() const                     { return m_isUsed; }
        void SetIsUsed(bool isUsed)             { m_isUsed = isUsed; }

        static void Reflect(AZ::ReflectContext* context);

    protected:
        Pose* m_pose;
        bool m_isUsed;
    };
} // namespace EMotionFX
