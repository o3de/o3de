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