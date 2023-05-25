/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/algorithm.h>
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
    AZ_CLASS_ALLOCATOR_IMPL(MotionLayerSystem, MotionAllocator)

    MotionLayerSystem::MotionLayerSystem(ActorInstance* actorInstance)
        : MotionSystem(actorInstance)
    {
        // set the motion based actor repositioning layer pass
        m_repositioningPass = RepositioningLayerPass::Create(this);
    }


    MotionLayerSystem::~MotionLayerSystem()
    {
        // remove all layer passes
        RemoveAllLayerPasses();

        // get rid of the repositioning layer pass
        m_repositioningPass->Destroy();
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
        for (LayerPass* layerPass : m_layerPasses)
        {
            if (delFromMem)
            {
                layerPass->Destroy();
            }
        }

        m_layerPasses.clear();
    }


    // start playing a motion
    void MotionLayerSystem::StartMotion(MotionInstance* motion, PlayBackInfo* info)
    {
        // check if we have any motions playing already
        const size_t numMotionInstances = m_motionInstances.size();
        if (numMotionInstances > 0)
        {
            // find the right location in the motion instance array to insert this motion instance
            size_t insertPos = FindInsertPos(motion->GetPriorityLevel());
            if (insertPos != InvalidIndex)
            {
                m_motionInstances.emplace(AZStd::next(begin(m_motionInstances), insertPos), motion);
            }
            else
            {
                m_motionInstances.emplace_back(motion);
            }
        }
        else // no motions are playing, so just add it
        {
            m_motionInstances.emplace_back(motion);
        }

        // trigger an event
        GetEventManager().OnStartMotionInstance(motion, info);

        // make sure it is not in pause mode
        motion->UnPause();
        motion->SetIsActive(true);

        // start the blend
        motion->SetWeight(info->m_targetWeight, info->m_blendInTime);
    }


    // find the location where to insert a new motion with a given priority
    size_t MotionLayerSystem::FindInsertPos(size_t priorityLevel) const
    {
        const auto* foundInsertPosition = AZStd::lower_bound(begin(m_motionInstances), end(m_motionInstances), priorityLevel, [](const MotionInstance* motionInstance, size_t level)
        {
            return motionInstance->GetPriorityLevel() < level;
        });
        return foundInsertPosition != end(m_motionInstances) ? AZStd::distance(begin(m_motionInstances), foundInsertPosition) : InvalidIndex;
    }


    // update the motion layer system
    void MotionLayerSystem::Update(float timePassed, bool updateNodes)
    {
        // update the motion instances instances
        UpdateMotionInstances(timePassed);

        // update the motion tree, which basically removes motion instances that are not required anymore
        UpdateMotionTree();

        // update the motion queue
        m_motionQueue->Update();

        // process all layer passes
        for (LayerPass* layerPass : m_layerPasses)
        {
            layerPass->Process();
        }

        // process the repositioning as last
        if (m_repositioningPass)
        {
            m_repositioningPass->Process();
        }

        // update the global transform now that we have an updated local transform of the actor instance itself (modified by motion extraction for example)
        m_actorInstance->UpdateWorldTransform();

        // if we need to update the node transforms because the character is visible
        if (updateNodes)
        {
            UpdateNodes();
        }
    }

    
    // update the motion tree
    void MotionLayerSystem::UpdateMotionTree()
    {
        for (size_t i = 0; i < m_motionInstances.size(); ++i)
        {
            MotionInstance* source = m_motionInstances[i];

            // if we aren't stopping this motion yet
            if (!source->GetIsStopping())
            {
                // if numloops not infinite and numloops-1 is the current number of loops
                // and the current time - blendouttime has been reached
                if (source->GetBlendOutBeforeEnded() && !source->GetIsPlayingForever())
                {
                    // if the motion has to stop
                    if ((source->GetTimeDifToLoopPoint() <= source->GetFadeTime() && !source->GetFreezeAtLastFrame()) ||
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
                        const size_t numToRemove = m_motionInstances.size() - (i + 1);
                        for (size_t a = 0; a < numToRemove; ++a)
                        {
                            RemoveMotionInstance(m_motionInstances[i + 1]);
                        }
                    }
                }
            }
        }
    }


    // remove all layers below a given layer
    size_t MotionLayerSystem::RemoveLayersBelow(MotionInstance* source)
    {
        size_t numRemoved = 0;

        // start from the bottom up
        for (auto iter = rbegin(m_motionInstances); iter != rend(m_motionInstances); ++iter)
        {
            MotionInstance* curInstance = *iter;

            // if we reached the current motion instance we are done
            if (curInstance == source)
            {
                return numRemoved;
            }

            numRemoved++;
            RemoveMotionInstance(curInstance);
        }

        return numRemoved;
    }


    // recursively find the first layer (top down search) which has a motion layer which is not in mixing mode
    MotionInstance* MotionLayerSystem::FindFirstNonMixingMotionInstance() const
    {
        // if there aren't any motion instances, return nullptr
        const auto foundMotionInstance = AZStd::find_if(begin(m_motionInstances), end(m_motionInstances), [](const MotionInstance* motionInstance)
        {
            return !motionInstance->GetIsMixing();
        });
        return foundMotionInstance != end(m_motionInstances) ? *foundMotionInstance : nullptr;
    }


    // update the node transformations
    void MotionLayerSystem::UpdateNodes()
    {
        // get the two pose buffers we need
        const uint32 threadIndex = m_actorInstance->GetThreadIndex();
        AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(threadIndex)->GetPosePool();
        AnimGraphPose* tempAnimGraphPose = posePool.RequestPose(m_actorInstance);

        const bool motionExtractionEnabled = m_actorInstance->GetMotionExtractionEnabled();

        Pose* tempActorPose = &tempAnimGraphPose->GetPose();

        const size_t numMotionInstances = m_motionInstances.size();
        if (numMotionInstances > 0)
        {
            if (numMotionInstances > 1)
            {
                TransformData* transformData = m_actorInstance->GetTransformData();
                Pose* finalPose = transformData->GetCurrentPose();
                finalPose->InitFromBindPose(m_actorInstance);

                // blend the layers
                for (auto iter = rbegin(m_motionInstances); iter != rend(m_motionInstances); ++iter)
                {
                    // skip inactive motion instances
                    MotionInstance* instance = *iter; // the motion to be blended
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
                MotionInstance* instance = m_motionInstances[0]; // the motion to be blended
                if (instance->GetIsActive() && instance->GetWeight() >= 0.9999f)
                {
                    TransformData* transformData = m_actorInstance->GetTransformData();
                    Pose* finalPose = transformData->GetCurrentPose();
                    finalPose->InitFromBindPose(m_actorInstance);

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
                    TransformData* transformData = m_actorInstance->GetTransformData();
                    transformData->GetCurrentPose()->InitFromBindPose(m_actorInstance);
                }
                else // semi active
                {
                    TransformData* transformData = m_actorInstance->GetTransformData();
                    Pose* finalPose = transformData->GetCurrentPose();

                    finalPose->InitFromBindPose(m_actorInstance);
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
            TransformData* transformData = m_actorInstance->GetTransformData();
            transformData->GetCurrentPose()->InitFromBindPose(m_actorInstance);
        }

        // free the poses back to the pool
        posePool.FreePose(tempAnimGraphPose);
    }


    // add a new pass
    void MotionLayerSystem::AddLayerPass(LayerPass* newPass)
    {
        m_layerPasses.emplace_back(newPass);
    }


    // get the number of layer passes
    size_t MotionLayerSystem::GetNumLayerPasses() const
    {
        return m_layerPasses.size();
    }


    // remove a given pass
    void MotionLayerSystem::RemoveLayerPass(size_t nr, bool delFromMem)
    {
        if (delFromMem)
        {
            m_layerPasses[nr]->Destroy();
        }

        m_layerPasses.erase(AZStd::next(begin(m_layerPasses), nr));
    }


    // remove a given pass
    void MotionLayerSystem::RemoveLayerPass(LayerPass* pass, bool delFromMem)
    {
        if (const auto it = AZStd::find(begin(m_layerPasses), end(m_layerPasses), pass); it != end(m_layerPasses))
        {
            m_layerPasses.erase(it);
        }

        if (delFromMem)
        {
            pass->Destroy();
        }
    }


    // insert a layer pass at a given position
    void MotionLayerSystem::InsertLayerPass(size_t insertPos, LayerPass* pass)
    {
        m_layerPasses.emplace(AZStd::next(begin(m_layerPasses), insertPos), pass);
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
        if (m_repositioningPass)
        {
            m_repositioningPass->Destroy();
        }

        m_repositioningPass = nullptr;
    }


    LayerPass* MotionLayerSystem::GetLayerPass(size_t index) const
    {
        return m_layerPasses[index];
    }
} // namespace EMotionFX
