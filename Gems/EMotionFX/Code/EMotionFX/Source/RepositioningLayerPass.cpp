/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "EMotionFXConfig.h"
#include "RepositioningLayerPass.h"
#include "MotionLayerSystem.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "TransformData.h"
#include "MotionInstance.h"
#include "Node.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(RepositioningLayerPass, LayerPassAllocator, 0)

    // the constructor
    RepositioningLayerPass::RepositioningLayerPass(MotionLayerSystem* motionLayerSystem)
        : LayerPass(motionLayerSystem)
    {
        mHierarchyPath.SetMemoryCategory(EMFX_MEMCATEGORY_MOTIONS_MOTIONSYSTEMS);
        mLastReposNode = MCORE_INVALIDINDEX32;
    }


    // the destructor
    RepositioningLayerPass::~RepositioningLayerPass()
    {
    }


    // create
    RepositioningLayerPass* RepositioningLayerPass::Create(MotionLayerSystem* motionLayerSystem)
    {
        return aznew RepositioningLayerPass(motionLayerSystem);
    }


    // get the layer pass type
    uint32 RepositioningLayerPass::GetType() const
    {
        return RepositioningLayerPass::TYPE_ID;
    }


    // The main function that processes the pass.
    void RepositioningLayerPass::Process()
    {
        ActorInstance* actorInstance = mMotionSystem->GetActorInstance();
        if (!actorInstance->GetMotionExtractionEnabled())
        {
            actorInstance->SetTrajectoryDeltaTransform(Transform::CreateIdentityWithZeroScale());
            return;
        }

        // Get the motion extraction node and check if we are actually playing any motions.
        Actor* actor = actorInstance->GetActor();
        Node* motionExtractNode = actor->GetMotionExtractionNode();
        if (!motionExtractNode || mMotionSystem->GetNumMotionInstances() == 0)
        {
            actorInstance->SetTrajectoryDeltaTransform(Transform::CreateIdentityWithZeroScale());
            return;
        }

        Transform finalTrajectoryDelta = Transform::CreateIdentityWithZeroScale(); 
        Transform trajectoryDelta;

        // Get the original transform data pointer, which we need for the additive blending.
        Pose* bindPose = actorInstance->GetTransformData()->GetBindPose();

        // Bottom up traversal of the layers.
        bool firstBlend = true;
        const uint32 numMotionInstances = mMotionSystem->GetNumMotionInstances();
        for (uint32 i = numMotionInstances - 1; i != MCORE_INVALIDINDEX32; --i)
        {
            MotionInstance* motionInstance = mMotionSystem->GetMotionInstance(i);
            if (!motionInstance->GetMotionExtractionEnabled())
            {
                continue;
            }

            // This motion doesn't influence the motion extraction node.
            if (!motionInstance->ExtractMotion(trajectoryDelta))
            {
                continue;
            }

            // Blend the relative movement.
            const float weight = motionInstance->GetWeight();
            if (motionInstance->GetBlendMode() != BLENDMODE_ADDITIVE || firstBlend)
            {
                finalTrajectoryDelta.Blend(trajectoryDelta, weight);
                firstBlend = false;
            }
            else
            {
                finalTrajectoryDelta.BlendAdditive(trajectoryDelta, bindPose->GetLocalSpaceTransform(motionExtractNode->GetNodeIndex()), weight);
            }
        }

        // Apply the final trajectory delta transform the the actor instance.
        actorInstance->SetTrajectoryDeltaTransform(finalTrajectoryDelta);
        actorInstance->ApplyMotionExtractionDelta();
    }
} // namespace EMotionFX
