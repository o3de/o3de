/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/MotionLayerSystem.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/PlayBackInfo.h>
#include <EMotionFX/Source/RepositioningLayerPass.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/MotionQueue.h>
#include <EMotionFX/Source/MotionData/MotionData.h>

namespace EMotionFX
{
    MotionLayerSystem::MotionLayerSystem(ActorInstance* actorInstance)
        : MotionSystem(actorInstance)
    {
        mLayerPasses.SetMemoryCategory(EMFX_MEMCATEGORY_MOTIONS_MOTIONSYSTEMS);

        // set the motion based actor repositioning layer pass
        mRepositioningPass = RepositioningLayerPass::Create(this);
    }


    MotionLayerSystem::~MotionLayerSystem()
    {
        // remove all layer passes
        RemoveAllLayerPasses();

        // get rid of the repositioning layer pass
        mRepositioningPass->Destroy();
    }


    // create
    MotionLayerSystem* MotionLayerSystem::Create(ActorInstance* actorInstance)
    {
        return new MotionLayerSystem(actorInstance);
    }


    // remove all layer passes
    void MotionLayerSystem::RemoveAllLayerPasses(bool delFromMem)
    {
        // delete all layer passes
        const uint32 numLayerPasses = mLayerPasses.GetLength();
        for (uint32 i = 0; i < numLayerPasses; ++i)
        {
            if (delFromMem)
            {
                mLayerPasses[i]->Destroy();
            }
        }

        mLayerPasses.Clear();
    }


    // start playing a motion
    void MotionLayerSystem::StartMotion(MotionInstance* motion, PlayBackInfo* info)
    {
        // check if we have any motions playing already
        const uint32 numMotionInstances = mMotionInstances.GetLength();
        if (numMotionInstances > 0)
        {
            // find the right location in the motion instance array to insert this motion instance
            uint32 insertPos = FindInsertPos(motion->GetPriorityLevel());
            if (insertPos != MCORE_INVALIDINDEX32)
            {
                mMotionInstances.Insert(insertPos, motion);
            }
            else
            {
                mMotionInstances.Add(motion);
            }
        }
        else // no motions are playing, so just add it
        {
            mMotionInstances.Add(motion);
        }

        // trigger an event
        GetEventManager().OnStartMotionInstance(motion, info);

        // make sure it is not in pause mode
        motion->UnPause();
        motion->SetIsActive(true);

        // start the blend
        motion->SetWeight(info->mTargetWeight, info->mBlendInTime);
    }


    // find the location where to insert a new motion with a given priority
    uint32 MotionLayerSystem::FindInsertPos(uint32 priorityLevel) const
    {
        const uint32 numInstances = mMotionInstances.GetLength();
        for (uint32 i = 0; i < numInstances; ++i)
        {
            if (mMotionInstances[i]->GetPriorityLevel() <= priorityLevel)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // update the motion layer system
    void MotionLayerSystem::Update(float timePassed, bool updateNodes)
    {
        // update the motion instances instances
        UpdateMotionInstances(timePassed);

        // update the motion tree, which basically removes motion instances that are not required anymore
        UpdateMotionTree();

        // update the motion queue
        mMotionQueue->Update();

        // process all layer passes
        const uint32 numPasses = mLayerPasses.GetLength();
        for (uint32 i = 0; i < numPasses; ++i)
        {
            mLayerPasses[i]->Process();
        }

        // process the repositioning as last
        if (mRepositioningPass)
        {
            mRepositioningPass->Process();
        }

        // update the global transform now that we have an updated local transform of the actor instance itself (modified by motion extraction for example)
        mActorInstance->UpdateWorldTransform();

        // if we need to update the node transforms because the character is visible
        if (updateNodes)
        {
            UpdateNodes();
        }
    }

    
    // update the motion tree
    void MotionLayerSystem::UpdateMotionTree()
    {
        for (uint32 i = 0; i < mMotionInstances.GetLength(); ++i)
        {
            MotionInstance* source = mMotionInstances[i];

            // if we aren't stopping this motion yet
            if (!source->GetIsStopping())
            {
                // if numloops not infinite and numloops-1 is the current number of loops
                // and the current time - blendouttime has been reached
                if (source->GetBlendOutBeforeEnded())
                {
                    // if the motion has to stop
                    if ((!source->GetIsPlayingForever() &&
                        (source->GetNumCurrentLoops() >= source->GetMaxLoops()) &&
                        (source->GetTimeDifToLoopPoint() <= source->GetFadeTime()) &&
                        !source->GetFreezeAtLastFrame()) ||
                        (source->GetHasEnded() && !source->GetFreezeAtLastFrame()))
                    {
                        // if we have haven't looped yet, so we are still in time to fade out
                        if (!source->GetHasEnded())
                        {
                            source->Stop(source->GetTimeDifToLoopPoint());  // start to fade out in the time left until we reach the end of the motion
                        }
                        else
                        {
                            source->Stop(0.0f); // we should have already stopped, but update rate was too low, so stop it immediately
                        }
                    }
                }
                else
                {
                    // if it has reached the end
                    if (source->GetHasEnded() && !source->GetFreezeAtLastFrame())
                    {
                        source->Stop();
                    }
                }

                if (source->GetHasEnded())
                {
                    GetEventManager().OnHasReachedMaxNumLoops(source);
                }

                // if we want to use the maximum play time
                if (source->GetMaxPlayTime() > 0.0f)
                {
                    // if we played the motion long enough, stop it
                    if (source->GetTotalPlayTime() >= source->GetMaxPlayTime())
                    {
                        GetEventManager().OnHasReachedMaxPlayTime(source);
                        source->Stop();
                    }
                }
            }

            // if we're still blending we didn't reach any ends
            if (source->GetIsBlending())
            {
                continue;
            }

            // if the layer faded out, remove it when we should
            if (source->GetWeight() <= 0.0f && (source->GetDeleteOnZeroWeight() || source->GetIsStopping()))
            {
                RemoveMotionInstance(source);
                i--;
                continue;
            }

            // if we reached our destination weight value, we have to stop blending
            if (source->GetWeight() >= 1.0f)
            {
                // if this is a non-mixing motion, and its not additive
                if (!source->GetIsMixing() && (source->GetBlendMode() != BLENDMODE_ADDITIVE))
                {
                    // we can delete the complete hierarchy below this layer, since we are fully blended
                    // into this layer, which overwrites the complete other transformations
                    // so the other layers won't have any influence anymore
                    // if the motion instance is allowed to overwrite and delete other layers completely
                    if (source->GetCanOverwrite())
                    {
                        // remove all motions that got overwritten by the current one
                        const uint32 numToRemove = mMotionInstances.GetLength() - (i + 1);
                        for (uint32 a = 0; a < numToRemove; ++a)
                        {
                            RemoveMotionInstance(mMotionInstances[i + 1]);
                        }
                    }
                }
            }
        }
    }


    // remove all layers below a given layer
    uint32 MotionLayerSystem::RemoveLayersBelow(MotionInstance* source)
    {
        uint32 numRemoved = 0;

        // start from the bottom up
        for (uint32 i = mMotionInstances.GetLength() - 1; i != MCORE_INVALIDINDEX32;)
        {
            MotionInstance* curInstance = mMotionInstances[i];

            // if we reached the current motion instance we are done
            if (curInstance == source)
            {
                return numRemoved;
            }

            numRemoved++;
            RemoveMotionInstance(curInstance);
            i--;
        }

        return numRemoved;
    }


    // recursively find the first layer (top down search) which has a motion layer which is not in mixing mode
    MotionInstance* MotionLayerSystem::FindFirstNonMixingMotionInstance() const
    {
        // if there aren't any motion instances, return nullptr
        const uint32 numInstances = mMotionInstances.GetLength();
        if (numInstances == 0)
        {
            return nullptr;
        }

        for (uint32 i = 0; i < numInstances; ++i)
        {
            if (mMotionInstances[i]->GetIsMixing() == false)
            {
                return mMotionInstances[i];
            }
        }

        return nullptr;
    }


    // update the node transformations
    void MotionLayerSystem::UpdateNodes()
    {
        // get the two pose buffers we need
        const uint32 threadIndex = mActorInstance->GetThreadIndex();
        AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(threadIndex)->GetPosePool();
        AnimGraphPose* tempAnimGraphPose = posePool.RequestPose(mActorInstance);

        const bool motionExtractionEnabled = mActorInstance->GetMotionExtractionEnabled();

        Pose* tempActorPose = &tempAnimGraphPose->GetPose();

        const uint32 numMotionInstances = mMotionInstances.GetLength();
        if (numMotionInstances > 0)
        {
            if (numMotionInstances > 1)
            {
                TransformData* transformData = mActorInstance->GetTransformData();
                Pose* finalPose = transformData->GetCurrentPose();
                finalPose->InitFromBindPose(mActorInstance);

                // blend the layers
                for (uint32 i = numMotionInstances - 1; i != MCORE_INVALIDINDEX32; --i)
                {
                    // skip inactive motion instances
                    MotionInstance* instance = mMotionInstances[i]; // the motion to be blended
                    if (instance->GetIsActive() == false || instance->GetWeight() < 0.0001f)
                    {
                        continue;
                    }

                    // update the temp actor pose with the result from the motion
                    instance->GetMotion()->Update(finalPose, tempActorPose, instance); // output the results of the single motion

                    // compensate for motion extraction
                    if (instance->GetMotionExtractionEnabled() && motionExtractionEnabled && !instance->GetMotion()->GetMotionData()->IsAdditive())
                    {
                        tempActorPose->CompensateForMotionExtractionDirect(instance->GetMotion()->GetMotionExtractionFlags());
                    }

                    // perform the blending based on settings in the motion instance
                    finalPose->Blend(tempActorPose, instance->GetWeight(), instance);
                }
            }
            else // there is just one motion playing
            {
                // skip inactive motion instances
                MotionInstance* instance = mMotionInstances[0]; // the motion to be blended
                if (instance->GetIsActive() && instance->GetWeight() >= 0.9999f)
                {
                    TransformData* transformData = mActorInstance->GetTransformData();
                    Pose* finalPose = transformData->GetCurrentPose();
                    finalPose->InitFromBindPose(mActorInstance);

                    instance->GetMotion()->Update(finalPose, finalPose, instance); // output the results of the single motion

                    // compensate for motion extraction
                    if (instance->GetMotionExtractionEnabled() && motionExtractionEnabled && !instance->GetMotion()->GetMotionData()->IsAdditive())
                    {
                        finalPose->CompensateForMotionExtractionDirect(instance->GetMotion()->GetMotionExtractionFlags());
                    }
                }
                else
                if (instance->GetIsActive() && instance->GetWeight() < 0.0001f) // almost not active
                {
                    TransformData* transformData = mActorInstance->GetTransformData();
                    transformData->GetCurrentPose()->InitFromBindPose(mActorInstance);
                }
                else // semi active
                {
                    TransformData* transformData = mActorInstance->GetTransformData();
                    Pose* finalPose = transformData->GetCurrentPose();

                    finalPose->InitFromBindPose(mActorInstance);
                    instance->GetMotion()->Update(finalPose, tempActorPose, instance); // output the results of the single motion

                    // compensate for motion extraction
                    if (instance->GetMotionExtractionEnabled() && motionExtractionEnabled && !instance->GetMotion()->GetMotionData()->IsAdditive())
                    {
                        tempActorPose->CompensateForMotionExtractionDirect(instance->GetMotion()->GetMotionExtractionFlags());
                    }

                    // perform the blending based on settings in the motion instance
                    finalPose->Blend(tempActorPose, instance->GetWeight(), instance);
                }
            }
        }
        else // no motion playing
        {
            // update all node transforms
            TransformData* transformData = mActorInstance->GetTransformData();
            transformData->GetCurrentPose()->InitFromBindPose(mActorInstance);
        }

        // free the poses back to the pool
        posePool.FreePose(tempAnimGraphPose);
    }


    // add a new pass
    void MotionLayerSystem::AddLayerPass(LayerPass* newPass)
    {
        mLayerPasses.Add(newPass);
    }


    // get the number of layer passes
    uint32 MotionLayerSystem::GetNumLayerPasses() const
    {
        return mLayerPasses.GetLength();
    }


    // remove a given pass
    void MotionLayerSystem::RemoveLayerPass(uint32 nr, bool delFromMem)
    {
        if (delFromMem)
        {
            mLayerPasses[nr]->Destroy();
        }

        mLayerPasses.Remove(nr);
    }


    // remove a given pass
    void MotionLayerSystem::RemoveLayerPass(LayerPass* pass, bool delFromMem)
    {
        mLayerPasses.RemoveByValue(pass);

        if (delFromMem)
        {
            pass->Destroy();
        }
    }


    // insert a layer pass at a given position
    void MotionLayerSystem::InsertLayerPass(uint32 insertPos, LayerPass* pass)
    {
        mLayerPasses.Insert(insertPos, pass);
    }


    // get the motion system type
    uint32 MotionLayerSystem::GetType() const
    {
        return MotionLayerSystem::TYPE_ID;
    }


    // get the motion type string
    const char* MotionLayerSystem::GetTypeString() const
    {
        return "MotionLayerSystem";
    }


    // remove the repositioning pass
    void MotionLayerSystem::RemoveRepositioningLayerPass()
    {
        if (mRepositioningPass)
        {
            mRepositioningPass->Destroy();
        }

        mRepositioningPass = nullptr;
    }


    LayerPass* MotionLayerSystem::GetLayerPass(uint32 index) const
    {
        return mLayerPasses[index];
    }
} // namespace EMotionFX
