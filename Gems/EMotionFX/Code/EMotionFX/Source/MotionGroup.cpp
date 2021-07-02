/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "MotionGroup.h"
#include "MotionInstance.h"
#include "ActorInstance.h"
#include "EMotionFXManager.h"
#include "MotionInstancePool.h"
#include "AnimGraphPose.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionGroup, MotionAllocator, 0)


    // default constructor
    MotionGroup::MotionGroup()
        : BaseObject()
    {
        mParentMotionInstance = nullptr;
    }


    // extended constructor
    MotionGroup::MotionGroup(MotionInstance* parentMotionInstance)
        : BaseObject()
    {
        LinkToMotionInstance(parentMotionInstance);
    }


    // destructor
    MotionGroup::~MotionGroup()
    {
        RemoveAllMotionInstances();
    }


    // creation
    MotionGroup* MotionGroup::Create()
    {
        return aznew MotionGroup();
    }


    // creation
    MotionGroup* MotionGroup::Create(MotionInstance* parentMotionInstance)
    {
        return aznew MotionGroup(parentMotionInstance);
    }


    // link to a motion instance
    void MotionGroup::LinkToMotionInstance(MotionInstance* parentMotionInstance)
    {
        mParentMotionInstance = parentMotionInstance;
    }


    // add a motion to the group
    MotionInstance* MotionGroup::AddMotion(Motion* motion, PlayBackInfo* playInfo, uint32 startNodeIndex)
    {
        MCORE_ASSERT(mParentMotionInstance); // use LinkToMotionInstance before

        // create the new motion instance
        MotionInstance* newInstance = GetMotionInstancePool().RequestNew(motion, mParentMotionInstance->GetActorInstance());

        // initialize the motion instance settings
        if (playInfo == nullptr) // if no playinfo specified, use default playback settings
        {
            PlayBackInfo info;
            newInstance->InitFromPlayBackInfo(info);
        }
        else
        {
            newInstance->InitFromPlayBackInfo(*playInfo);
        }

        // add it to the motion instance array
        mMotionInstances.Add(newInstance);

        return newInstance;
    }


    // remove all motion instances from the group and from memory
    void MotionGroup::RemoveAllMotionInstances()
    {
        // remove all motion instances from memory
        const uint32 numInstances = mMotionInstances.GetLength();
        for (uint32 i = 0; i < numInstances; ++i)
        {
            GetMotionInstancePool().Free(mMotionInstances[i]);
        }

        mMotionInstances.Clear();
    }


    // remove a given motion by its motion instance
    void MotionGroup::RemoveMotionInstance(MotionInstance* instance)
    {
        if (mMotionInstances.RemoveByValue(instance))
        {
            GetMotionInstancePool().Free(instance);
        }
    }


    // remove all motion instances using a given motion
    void MotionGroup::RemoveMotion(Motion* motion)
    {
        // for all the motion instances
        for (uint32 i = 0; i < mMotionInstances.GetLength();)
        {
            // if this motion instance uses the given motion
            if (mMotionInstances[i]->GetMotion() == motion)
            {
                // remove it from memory and from the array
                GetMotionInstancePool().Free(mMotionInstances[i]);
                mMotionInstances.Remove(i);
            }
            else
            {
                i++;
            }
        }
    }


    // remove a motion instance by its index
    void MotionGroup::RemoveMotionInstance(uint32 index)
    {
        MCORE_ASSERT(index < mMotionInstances.GetLength());

        // remove it from memory and from the array
        GetMotionInstancePool().Free(mMotionInstances[index]);
        mMotionInstances.Remove(index);
    }


    // update the motion instances
    void MotionGroup::Update(float timePassed)
    {
        // update the motion instances
        const uint32 numInstances = mMotionInstances.GetLength();
        for (uint32 i = 0; i < numInstances; ++i)
        {
            mMotionInstances[i]->Update(timePassed);
        }
    }


    // perform the blending and output it in the outPose buffer
    void MotionGroup::Output(const Pose* inPose, Pose* outPose)
    {
        uint32 i;

        // calculate the total weight
        float totalWeight = 0.0f;
        const uint32 numInstances = mMotionInstances.GetLength();
        for (i = 0; i < numInstances; ++i)
        {
            totalWeight += mMotionInstances[i]->GetWeight();
        }

        // calculate the inverse of the total weight so that we can replace divides by multiplies, which is faster
        float invTotalWeight;
        if (totalWeight < 0.0001f)
        {
            invTotalWeight = 0.0f;
        }
        else
        {
            invTotalWeight = 1.0f / totalWeight;
        }

        const ActorInstance* actorInstance = inPose->GetActorInstance();
        const uint32 threadIndex = actorInstance->GetThreadIndex();
        AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(threadIndex)->GetPosePool();
        AnimGraphPose* groupAnimGraphPose = posePool.RequestPose(actorInstance);

        // get the group blend pose and make sure it's big enough
        Pose* groupBlendPose = &groupAnimGraphPose->GetPose();//mParentMotionInstance->GetActorInstance()->GetActor()->GetGroupBlendPose();
        MCORE_ASSERT(groupBlendPose->GetNumTransforms() == inPose->GetNumTransforms());

        // blend using the normalized weights
        for (i = 0; i < numInstances; ++i)
        {
            // calculate the normalized weight
            const float normalizedWeight = mMotionInstances[i]->GetWeight() * invTotalWeight;

            // output the motion output into the group blend buffer
            mMotionInstances[i]->GetMotion()->Update(inPose, groupBlendPose, mMotionInstances[i]);

            // if it's the first motion instance in the group
            if (i == 0)
            {
                // blend all transforms
                // TODO: use only enabled nodes
                const uint32 numTransforms = outPose->GetNumTransforms();
                for (uint32 t = 0; t < numTransforms; ++t)
                {
                    Transform& transform = groupBlendPose->GetLocalSpaceTransformDirect(t);
                    Transform& outTransform = outPose->GetLocalSpaceTransformDirect(t);
                    transform.mRotation.Normalize();

                    EMFX_SCALECODE
                    (
                        //transform.mScaleRotation.Normalize();
                        outTransform.mScale         = transform.mScale          * normalizedWeight;
                        //outTransform.mScaleRotation   = transform.mScaleRotation  * normalizedWeight;
                    )

                    outTransform.mPosition  = transform.mPosition * normalizedWeight;
                    outTransform.mRotation  = transform.mRotation * normalizedWeight;
                }
            }
            else
            {
                // blend all transforms
                // TODO: use only enabled nodes
                const uint32 numTransforms = outPose->GetNumTransforms();
                for (uint32 t = 0; t < numTransforms; ++t)
                {
                    Transform& transform = groupBlendPose->GetLocalSpaceTransformDirect(t);
                    Transform& outTransform = outPose->GetLocalSpaceTransformDirect(t);

                    outTransform.mPosition += transform.mPosition * normalizedWeight;

                    EMFX_SCALECODE
                    (
                        outTransform.mScale += transform.mScale * normalizedWeight;

                        // make sure we use the correct hemisphere
                        //if (outTransform.mScaleRotation.Dot( transform.mScaleRotation ) < 0.0f)
                        //transform.mScaleRotation = -transform.mScaleRotation;

                        //outTransform.mScaleRotation   += transform.mScaleRotation * normalizedWeight;
                    )

                    // make sure we use the correct hemisphere
                    if (outTransform.mRotation.Dot(transform.mRotation) < 0.0f)
                    {
                        transform.mRotation = -transform.mRotation;
                    }

                    outTransform.mRotation += transform.mRotation * normalizedWeight;
                }
            }
        } // for all motion instances in the group

        // normalize the quaternions
        const uint32 numTransforms = outPose->GetNumTransforms();
        for (uint32 t = 0; t < numTransforms; ++t)
        {
            Transform& outTransform = outPose->GetLocalSpaceTransformDirect(t);
            outTransform.mRotation.Normalize();

            //EMFX_SCALECODE
            //(
            //outTransform.mScaleRotation.Normalize();
            //)
        }

        // free the pose
        posePool.FreePose(groupAnimGraphPose);
    }
}   // namespace EMotionFX
