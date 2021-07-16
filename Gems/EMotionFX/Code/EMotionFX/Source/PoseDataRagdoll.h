/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Physics/RagdollPhysicsBus.h>
#include <EMotionFX/Source/PoseData.h>
#include <EMotionFX/Source/Transform.h>


namespace EMotionFX
{
    class EMFX_API PoseDataRagdoll
        : public PoseData
    {
    public:
        AZ_RTTI(PoseDataRagdoll, "{39D40C53-B4CA-48BB-BE6A-B7AE706DA25F}", PoseData)
        AZ_CLASS_ALLOCATOR_DECL

        PoseDataRagdoll();
        ~PoseDataRagdoll();

        void Clear();

        void LinkToActorInstance(const ActorInstance* actorInstance) override;
        void LinkToActor(const Actor* actor) override;
        void Reset() override;

        void CopyFrom(const PoseData* from) override;
        static void FastCopyNodeStates(AZStd::vector<Physics::RagdollNodeState>& to, const AZStd::vector<Physics::RagdollNodeState>& from);
        void BlendNodeState(Physics::RagdollNodeState& nodeState, const Physics::RagdollNodeState& destNodeState, const Transform& jointTransform, const Transform& destJointTransform, float weight);
        void Blend(const Pose* destPose, float weight) override;

        void Log() const;

        const AZStd::vector<Physics::RagdollNodeState>& GetRagdollNodeStates() const            { return m_nodeStates; }
        Physics::RagdollNodeState& GetRagdollNodeState(size_t ragdollNodeIndex)                 { return m_nodeStates[ragdollNodeIndex]; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::vector<Physics::RagdollNodeState> m_nodeStates;
    };
} // namespace EMotionFX
