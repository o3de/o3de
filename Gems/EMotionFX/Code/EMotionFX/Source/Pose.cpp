/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MorphSetupInstance.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/PoseDataFactory.h>
#include <EMotionFX/Source/TransformData.h>

namespace EMotionFX
{
    // default constructor
    Pose::Pose()
    {
        mActorInstance  = nullptr;
        mActor          = nullptr;
        mSkeleton       = nullptr;
        mLocalSpaceTransforms.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_POSE);
        mModelSpaceTransforms.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_POSE);
        mFlags.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_POSE);
        mMorphWeights.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_POSE);

        // reset morph weights
        //mMorphWeights.Reserve(32);
        //mLocalSpaceTransforms.Reserve(128);
        //mModelSpaceTransforms.Reserve(128);
        //mFlags.Reserve(128);
    }


    // copy constructor
    Pose::Pose(const Pose& other)
    {
        InitFromPose(&other);
    }


    // destructor
    Pose::~Pose()
    {
        Clear(true);
    }


    // link the pose to a given actor instance
    void Pose::LinkToActorInstance(const ActorInstance* actorInstance, uint8 initialFlags)
    {
        // store the pointer to the actor instance etc
        mActorInstance  = actorInstance;
        mActor          = actorInstance->GetActor();
        mSkeleton       = mActor->GetSkeleton();

        // resize the buffers
        const uint32 numTransforms = mActor->GetSkeleton()->GetNumNodes();
        mLocalSpaceTransforms.ResizeFast(numTransforms);
        mModelSpaceTransforms.ResizeFast(numTransforms);
        mFlags.ResizeFast(numTransforms);
        mMorphWeights.ResizeFast(actorInstance->GetMorphSetupInstance()->GetNumMorphTargets());

        for (const auto& poseDataItem : m_poseDatas)
        {
            PoseData* poseData = poseDataItem.second.get();
            poseData->LinkToActorInstance(actorInstance);
        }

        ClearFlags(initialFlags);
    }


    // link the pose to a given actor
    void Pose::LinkToActor(const Actor* actor, uint8 initialFlags, bool clearAllFlags)
    {
        mActorInstance  = nullptr;
        mActor          = actor;
        mSkeleton       = actor->GetSkeleton();

        // resize the buffers
        const uint32 numTransforms = mActor->GetSkeleton()->GetNumNodes();
        mLocalSpaceTransforms.ResizeFast(numTransforms);
        mModelSpaceTransforms.ResizeFast(numTransforms);

        const uint32 oldSize = mFlags.GetLength();
        mFlags.ResizeFast(numTransforms);
        if (oldSize < numTransforms && clearAllFlags == false)
        {
            for (uint32 i = oldSize; i < numTransforms; ++i)
            {
                mFlags[i] = initialFlags;
            }
        }

        MorphSetup* morphSetup = mActor->GetMorphSetup(0);
        mMorphWeights.ResizeFast((morphSetup) ? morphSetup->GetNumMorphTargets() : 0);

        for (const auto& poseDataItem : m_poseDatas)
        {
            PoseData* poseData = poseDataItem.second.get();
            poseData->LinkToActor(actor);
        }

        if (clearAllFlags)
        {
            ClearFlags(initialFlags);
        }
    }

    //
    void Pose::SetNumTransforms(uint32 numTransforms)
    {
        // resize the buffers
        mLocalSpaceTransforms.ResizeFast(numTransforms);
        mModelSpaceTransforms.ResizeFast(numTransforms);

        const uint32 oldSize = mFlags.GetLength();
        mFlags.ResizeFast(numTransforms);

        for (uint32 i = oldSize; i < numTransforms; ++i)
        {
            mFlags[i] = 0;
            SetLocalSpaceTransform(i, Transform::CreateIdentity());
        }
    }


    void Pose::Clear(bool clearMem)
    {
        mLocalSpaceTransforms.Clear(clearMem);
        mModelSpaceTransforms.Clear(clearMem);
        mFlags.Clear(clearMem);
        mMorphWeights.Clear(clearMem);

        ClearPoseDatas();
    }


    // clear the pose flags
    void Pose::ClearFlags(uint8 newFlags)
    {
        MCore::MemSet((uint8*)mFlags.GetPtr(), newFlags, sizeof(uint8) * mFlags.GetLength());
    }


    /*
    // init from a set of local space transformations
    void Pose::InitFromLocalTransforms(ActorInstance* actorInstance, const Transform* localTransforms)
    {
        // link to an actor instance
        LinkToActorInstance( actorInstance, FLAG_LOCALTRANSFORMREADY );

        // reset all flags
        //MCore::MemSet( (uint8*)mFlags.GetPtr(), FLAG_LOCALTRANSFORMREADY, sizeof(uint8)*mFlags.GetLength() );

        // copy over the local transforms
        MCore::MemCopy((uint8*)mLocalTransforms.GetPtr(), (uint8*)localTransforms, sizeof(Transform)*mLocalTransforms.GetLength());
    }
    */

    // initialize this pose to the bind pose
    void Pose::InitFromBindPose(const ActorInstance* actorInstance)
    {
        // init to the bind pose
        const Pose* bindPose = actorInstance->GetTransformData()->GetBindPose();
        if (bindPose)
        {
            InitFromPose(bindPose);
        }

        // compensate for motion extraction
        // we already moved our actor instance's position and rotation at this point
        // so we have to cancel/compensate this delta offset from the motion extraction node, so that we don't double-transform
        // basically this will keep the motion in-place rather than moving it away from the origin
        //CompensateForMotionExtractionDirect();
    }


    // init to the actor bind pose
    void Pose::InitFromBindPose(const Actor* actor)
    {
        InitFromPose(actor->GetBindPose());
    }


    /*
    // initialize this pose from some given set of local space transformations
    void Pose::InitFromLocalBindSpaceTransforms(Actor* actor)
    {
        // link to an actor
        LinkToActor(actor);

        // reset all flags
        MCore::MemSet( (uint8*)mFlags.GetPtr(), FLAG_LOCALTRANSFORMREADY, sizeof(uint8)*mFlags.GetLength() );

        // copy over the local transforms
        MCore::MemCopy((uint8*)mLocalTransforms.GetPtr(), (uint8*)actor->GetBindPose().GetLocalTransforms(), sizeof(Transform)*mLocalTransforms.GetLength());

        // reset the morph targets
        const uint32 numMorphWeights = mMorphWeights.GetLength();
        for (uint32 i=0; i<numMorphWeights; ++i)
            mMorphWeights[i] = 0.0f;
    }
    */

    // update the full local space pose
    void Pose::ForceUpdateFullLocalSpacePose()
    {
        Skeleton* skeleton = mActor->GetSkeleton();
        const uint32 numNodes = skeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            const uint32 parentIndex = skeleton->GetNode(i)->GetParentIndex();
            if (parentIndex != MCORE_INVALIDINDEX32)
            {
                GetModelSpaceTransform(parentIndex, &mLocalSpaceTransforms[i]);
                mLocalSpaceTransforms[i].Inverse();
                mLocalSpaceTransforms[i].PreMultiply(mModelSpaceTransforms[i]);
            }
            else
            {
                mLocalSpaceTransforms[i] = mModelSpaceTransforms[i];
            }

            mFlags[i] |= FLAG_LOCALTRANSFORMREADY;
        }
    }


    // update the full model space pose
    void Pose::ForceUpdateFullModelSpacePose()
    {
        // iterate from root towards child nodes recursively, updating all model space transforms on the way
        Skeleton* skeleton = mActor->GetSkeleton();
        const uint32 numNodes = skeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            const uint32 parentIndex = skeleton->GetNode(i)->GetParentIndex();
            if (parentIndex != MCORE_INVALIDINDEX32)
            {
                mModelSpaceTransforms[parentIndex].PreMultiply(mLocalSpaceTransforms[i], &mModelSpaceTransforms[i]);
            }
            else
            {
                mModelSpaceTransforms[i] = mLocalSpaceTransforms[i];
            }

            mFlags[i] |= FLAG_MODELTRANSFORMREADY;
        }
    }


    // recursively update
    void Pose::UpdateModelSpaceTransform(uint32 nodeIndex) const
    {
        Skeleton* skeleton = mActor->GetSkeleton();

        const uint32 parentIndex = skeleton->GetNode(nodeIndex)->GetParentIndex();
        if (parentIndex != MCORE_INVALIDINDEX32 && !(mFlags[parentIndex] & FLAG_MODELTRANSFORMREADY))
        {
            UpdateModelSpaceTransform(parentIndex);
        }

        // update the model space transform if needed
        if ((mFlags[nodeIndex] & FLAG_MODELTRANSFORMREADY) == false)
        {
            const Transform& localTransform = GetLocalSpaceTransform(nodeIndex);
            if (parentIndex != MCORE_INVALIDINDEX32)
            {
                mModelSpaceTransforms[parentIndex].PreMultiply(localTransform, &mModelSpaceTransforms[nodeIndex]);
            }
            else
            {
                mModelSpaceTransforms[nodeIndex] = mLocalSpaceTransforms[nodeIndex];
            }

            mFlags[nodeIndex] |= FLAG_MODELTRANSFORMREADY;
        }
    }


    // update the local transform
    void Pose::UpdateLocalSpaceTransform(uint32 nodeIndex) const
    {
        const uint32 flags = mFlags[nodeIndex];
        if (flags & FLAG_LOCALTRANSFORMREADY)
        {
            return;
        }

        MCORE_ASSERT(flags & FLAG_MODELTRANSFORMREADY); // the model space transform has to be updated already, otherwise we cannot possibly calculate the local space one
        //if ((flags & FLAG_GLOBALTRANSFORMREADY) == false)
        //      DebugBreak();

        Skeleton* skeleton = mActor->GetSkeleton();
        const uint32 parentIndex = skeleton->GetNode(nodeIndex)->GetParentIndex();
        if (parentIndex != MCORE_INVALIDINDEX32)
        {
            GetModelSpaceTransform(parentIndex, &mLocalSpaceTransforms[nodeIndex]);
            mLocalSpaceTransforms[nodeIndex].Inverse();
            mLocalSpaceTransforms[nodeIndex].PreMultiply(mModelSpaceTransforms[nodeIndex]);
        }
        else
        {
            mLocalSpaceTransforms[nodeIndex] = mModelSpaceTransforms[nodeIndex];
        }

        mFlags[nodeIndex] |= FLAG_LOCALTRANSFORMREADY;
    }


    // get the local transform
    const Transform& Pose::GetLocalSpaceTransform(uint32 nodeIndex) const
    {
        UpdateLocalSpaceTransform(nodeIndex);
        return mLocalSpaceTransforms[nodeIndex];
    }


    const Transform& Pose::GetModelSpaceTransform(uint32 nodeIndex) const
    {
        UpdateModelSpaceTransform(nodeIndex);
        return mModelSpaceTransforms[nodeIndex];
    }


    Transform Pose::GetWorldSpaceTransform(uint32 nodeIndex) const
    {
        UpdateModelSpaceTransform(nodeIndex);
        return mModelSpaceTransforms[nodeIndex].Multiplied(mActorInstance->GetWorldSpaceTransform());
    }


    void Pose::GetWorldSpaceTransform(uint32 nodeIndex, Transform* outResult) const
    {
        UpdateModelSpaceTransform(nodeIndex);
        *outResult = mModelSpaceTransforms[nodeIndex];
        outResult->Multiply(mActorInstance->GetWorldSpaceTransform());
    }


    // calculate a local transform
    void Pose::GetLocalSpaceTransform(uint32 nodeIndex, Transform* outResult) const
    {
        if ((mFlags[nodeIndex] & FLAG_LOCALTRANSFORMREADY) == false)
        {
            UpdateLocalSpaceTransform(nodeIndex);
        }

        *outResult = mLocalSpaceTransforms[nodeIndex];
    }


    void Pose::GetModelSpaceTransform(uint32 nodeIndex, Transform* outResult) const
    {
        UpdateModelSpaceTransform(nodeIndex);
        *outResult = mModelSpaceTransforms[nodeIndex];
    }


    // set the local transform
    void Pose::SetLocalSpaceTransform(uint32 nodeIndex, const Transform& newTransform, bool invalidateGlobalTransforms)
    {
        mLocalSpaceTransforms[nodeIndex] = newTransform;
        mFlags[nodeIndex] |= FLAG_LOCALTRANSFORMREADY;

        // mark all child node model space transforms as dirty (recursively)
        if (invalidateGlobalTransforms)
        {
            if (mFlags[nodeIndex] & FLAG_MODELTRANSFORMREADY)
            {
                RecursiveInvalidateModelSpaceTransforms(mActor, nodeIndex);
            }
        }
    }


    // mark all child nodes recursively as dirty
    void Pose::RecursiveInvalidateModelSpaceTransforms(const Actor* actor, uint32 nodeIndex)
    {
        // if this model space transform ain't ready yet assume all child nodes are also not
        if ((mFlags[nodeIndex] & FLAG_MODELTRANSFORMREADY) == false)
        {
            return;
        }

        // mark the global transform as invalid
        mFlags[nodeIndex] &= ~FLAG_MODELTRANSFORMREADY;

        // recurse through all child nodes
        Skeleton* skeleton = actor->GetSkeleton();
        Node* node = skeleton->GetNode(nodeIndex);
        const uint32 numChildNodes = node->GetNumChildNodes();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            RecursiveInvalidateModelSpaceTransforms(actor, node->GetChildIndex(i));
        }
    }


    void Pose::SetModelSpaceTransform(uint32 nodeIndex, const Transform& newTransform, bool invalidateChildGlobalTransforms)
    {
        mModelSpaceTransforms[nodeIndex] = newTransform;

        // invalidate the local transform
        mFlags[nodeIndex] &= ~FLAG_LOCALTRANSFORMREADY;

        // recursively invalidate all model space transforms of all child nodes
        if (invalidateChildGlobalTransforms)
        {
            RecursiveInvalidateModelSpaceTransforms(mActor, nodeIndex);
        }

        // mark this model space transform as ready
        mFlags[nodeIndex] |= FLAG_MODELTRANSFORMREADY;
        UpdateLocalSpaceTransform(nodeIndex);
    }


    void Pose::SetWorldSpaceTransform(uint32 nodeIndex, const Transform& newTransform, bool invalidateChildGlobalTransforms)
    {
        mModelSpaceTransforms[nodeIndex] = newTransform.Multiplied(mActorInstance->GetWorldSpaceTransformInversed());
        mFlags[nodeIndex] &= ~FLAG_LOCALTRANSFORMREADY;

        if (invalidateChildGlobalTransforms)
        {
            RecursiveInvalidateModelSpaceTransforms(mActor, nodeIndex);
        }

        mFlags[nodeIndex] |= FLAG_MODELTRANSFORMREADY;
        UpdateLocalSpaceTransform(nodeIndex);
    }


    // invalidate all local transforms
    void Pose::InvalidateAllLocalSpaceTransforms()
    {
        const uint32 numFlags = mFlags.GetLength();
        for (uint32 i = 0; i < numFlags; ++i)
        {
            mFlags[i] &= ~FLAG_LOCALTRANSFORMREADY;
        }
    }


    void Pose::InvalidateAllModelSpaceTransforms()
    {
        const uint32 numFlags = mFlags.GetLength();
        for (uint32 i = 0; i < numFlags; ++i)
        {
            mFlags[i] &= ~FLAG_MODELTRANSFORMREADY;
        }
    }


    void Pose::InvalidateAllLocalAndModelSpaceTransforms()
    {
        const uint32 numFlags = mFlags.GetLength();
        for (uint32 i = 0; i < numFlags; ++i)
        {
            mFlags[i] &= ~(FLAG_LOCALTRANSFORMREADY | FLAG_MODELTRANSFORMREADY);
        }
    }


    Transform Pose::CalcTrajectoryTransform() const
    {
        MCORE_ASSERT(mActor);
        const uint32 motionExtractionNodeIndex = mActor->GetMotionExtractionNodeIndex();
        if (motionExtractionNodeIndex == MCORE_INVALIDINDEX32)
        {
            return Transform::CreateIdentity();
        }

        return GetWorldSpaceTransform(motionExtractionNodeIndex).ProjectedToGroundPlane();
    }


    void Pose::UpdateAllLocalSpaceTranforms()
    {
        Skeleton* skeleton = mActor->GetSkeleton();
        const uint32 numNodes = skeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            UpdateLocalSpaceTransform(i);
        }
    }


    void Pose::UpdateAllModelSpaceTranforms()
    {
        Skeleton* skeleton = mActor->GetSkeleton();
        const uint32 numNodes = skeleton->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            UpdateModelSpaceTransform(i);
        }
    }


    //-----------------------------------------------------------------

    // blend two poses, non mixing
    void Pose::BlendNonMixed(const Pose* destPose, float weight, const MotionInstance* instance, Pose* outPose)
    {
        // make sure the number of transforms are equal
        MCORE_ASSERT(destPose);
        MCORE_ASSERT(outPose);
        MCORE_ASSERT(mLocalSpaceTransforms.GetLength() == destPose->mLocalSpaceTransforms.GetLength());
        MCORE_ASSERT(mLocalSpaceTransforms.GetLength() == outPose->mLocalSpaceTransforms.GetLength());
        MCORE_ASSERT(instance->GetIsMixing() == false);

        // get some motion instance properties which we use to decide the optimized blending routine
        const bool additive = (instance->GetBlendMode() == BLENDMODE_ADDITIVE);
        ActorInstance* actorInstance = instance->GetActorInstance();

        // check if we want an additive blend or not
        if (!additive)
        {
            // if the dest pose has full influence, simply copy over that pose instead of performing blending
            if (weight >= 1.0f)
            {
                outPose->InitFromPose(destPose);
            }
            else
            {
                if (weight > 0.0f)
                {
                    uint32 nodeNr;
                    const uint32 numNodes = actorInstance->GetNumEnabledNodes();
                    for (uint32 i = 0; i < numNodes; ++i)
                    {
                        nodeNr = actorInstance->GetEnabledNode(i);
                        Transform transform = GetLocalSpaceTransform(nodeNr);
                        transform.Blend(destPose->GetLocalSpaceTransform(nodeNr), weight);
                        outPose->SetLocalSpaceTransform(nodeNr, transform, false);
                    }
                    outPose->InvalidateAllModelSpaceTransforms();
                }
                else // if the weight is 0, so the source
                {
                    if (outPose != this)
                    {
                        outPose->InitFromPose(this); // TODO: make it use the motionInstance->GetActorInstance()?
                    }
                }
            }

            // blend the morph weights
            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(actorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] = MCore::LinearInterpolate<float>(mMorphWeights[i], destPose->mMorphWeights[i], weight);
            }
        }
        else
        {
            TransformData* transformData = instance->GetActorInstance()->GetTransformData();
            const Pose* bindPose = transformData->GetBindPose();
            uint32 nodeNr;
            Transform result;
            const uint32 numNodes = actorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                nodeNr = actorInstance->GetEnabledNode(i);
                const Transform& base = bindPose->GetLocalSpaceTransform(nodeNr);
                BlendTransformAdditiveUsingBindPose(base, GetLocalSpaceTransform(nodeNr), destPose->GetLocalSpaceTransform(nodeNr), weight, &result);
                outPose->SetLocalSpaceTransform(nodeNr, result, false);
            }
            outPose->InvalidateAllModelSpaceTransforms();

            // blend the morph weights
            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(actorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] += destPose->mMorphWeights[i] * weight;
            }
        }
    }


    // blend two poses, mixing
    void Pose::BlendMixed(const Pose* destPose, float weight, const MotionInstance* instance, Pose* outPose)
    {
        // make sure the number of transforms are equal
        MCORE_ASSERT(destPose);
        MCORE_ASSERT(outPose);
        MCORE_ASSERT(mLocalSpaceTransforms.GetLength() == destPose->mLocalSpaceTransforms.GetLength());
        MCORE_ASSERT(mLocalSpaceTransforms.GetLength() == outPose->mLocalSpaceTransforms.GetLength());
        MCORE_ASSERT(instance->GetIsMixing());

        const bool additive = (instance->GetBlendMode() == BLENDMODE_ADDITIVE);

        // get the actor from the instance
        ActorInstance* actorInstance = instance->GetActorInstance();

        // get the transform data
        TransformData* transformData = instance->GetActorInstance()->GetTransformData();
        Transform result;

        const MotionLinkData* motionLinkData = instance->GetMotion()->GetMotionData()->FindMotionLinkData(actorInstance->GetActor());
        AZ_Assert(motionLinkData->GetJointDataLinks().size() == mLocalSpaceTransforms.GetLength(), "Expecting there to be the same amount of motion links as pose transforms.");

        // blend all transforms
        if (!additive)
        {
            uint32 nodeNr;
            const uint32 numNodes = actorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                nodeNr = actorInstance->GetEnabledNode(i);

                // try to find the motion link
                // if we cannot find it, this node/transform is not influenced by the motion, so we skip it
                if (!motionLinkData->IsJointActive(nodeNr))
                {
                    continue;
                }

                // blend the source into dest with the given weight and output it inside the output pose
                BlendTransformWithWeightCheck(GetLocalSpaceTransform(nodeNr), destPose->GetLocalSpaceTransform(nodeNr), weight, &result);
                outPose->SetLocalSpaceTransform(nodeNr, result, false);
            }

            outPose->InvalidateAllModelSpaceTransforms();

            // blend the morph weights
            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(actorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] = MCore::LinearInterpolate<float>(mMorphWeights[i], destPose->mMorphWeights[i], weight);
            }
        }
        else
        {
            Pose* bindPose = transformData->GetBindPose();
            uint32 nodeNr;
            const uint32 numNodes = actorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                nodeNr = actorInstance->GetEnabledNode(i);

                // try to find the motion link
                // if we cannot find it, this node/transform is not influenced by the motion, so we skip it
                if (!motionLinkData->IsJointActive(nodeNr))
                {
                    continue;
                }

                // blend the source into dest with the given weight and output it inside the output pose
                BlendTransformAdditiveUsingBindPose(bindPose->GetLocalSpaceTransform(nodeNr), GetLocalSpaceTransform(nodeNr), destPose->GetLocalSpaceTransform(nodeNr), weight, &result);
                outPose->SetLocalSpaceTransform(nodeNr, result, false);
            }
            outPose->InvalidateAllModelSpaceTransforms();

            // blend the morph weights
            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(actorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] += destPose->mMorphWeights[i] * weight;
            }
        }
    }


    // init from another pose
    void Pose::InitFromPose(const Pose* sourcePose)
    {
        if (!sourcePose)
        {
            if (mActorInstance)
            {
                InitFromBindPose(mActorInstance);
            }
            else
            {
                InitFromBindPose(mActor);
            }

            return;
        }

        mModelSpaceTransforms.MemCopyContentsFrom(sourcePose->mModelSpaceTransforms);
        mLocalSpaceTransforms.MemCopyContentsFrom(sourcePose->mLocalSpaceTransforms);
        mFlags.MemCopyContentsFrom(sourcePose->mFlags);
        mMorphWeights.MemCopyContentsFrom(sourcePose->mMorphWeights);

        // Deactivate pose datas from the current pose that are not in the source that we copy from.
        // This is needed in order to prevent leftover pose datas and to avoid de-/allocations.
        for (auto& poseDataItem : m_poseDatas)
        {
            const AZ::TypeId& typeId = poseDataItem.first;
            if (!sourcePose->HasPoseData(typeId))
            {
                PoseData* poseData = poseDataItem.second.get();
                poseData->SetIsUsed(false);
            }
        }

        // Make sure the current pose has all the pose datas from the source pose, and copy them over.
        const auto& sourcePoseDatas = sourcePose->GetPoseDatas();
        for (const auto& sourcePoseDataItem : sourcePoseDatas)
        {
            const AZ::TypeId& sourceTypeId = sourcePoseDataItem.first;
            const PoseData* sourcePoseData = sourcePoseDataItem.second.get();

            PoseData* poseData = GetPoseDataByType(sourceTypeId);
            if (!poseData)
            {
                poseData = PoseDataFactory::Create(this, sourceTypeId);
                AddPoseData(poseData);
            }

            *poseData = *sourcePoseData;
        }
    }


    // blend, and auto-detect if it is a mixing or non mixing blend
    void Pose::Blend(const Pose* destPose, float weight, const MotionInstance* instance)
    {
        if (instance->GetIsMixing() == false)
        {
            BlendNonMixed(destPose, weight, instance, this);
        }
        else
        {
            BlendMixed(destPose, weight, instance, this);
        }

        InvalidateAllModelSpaceTransforms();
    }


    // blend, and auto-detect if it is a mixing or non mixing blend
    void Pose::Blend(const Pose* destPose, float weight, const MotionInstance* instance, Pose* outPose)
    {
        if (instance->GetIsMixing() == false)
        {
            BlendNonMixed(destPose, weight, instance, outPose);
        }
        else
        {
            BlendMixed(destPose, weight, instance, outPose);
        }

        InvalidateAllModelSpaceTransforms();
    }


    // reset all transforms to zero
    void Pose::Zero()
    {
        if (mActorInstance)
        {
            uint32 nodeNr;
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                nodeNr = mActorInstance->GetEnabledNode(i);
                mLocalSpaceTransforms[nodeNr].Zero();
            }

            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(mActorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] = 0.0f;
            }
        }
        else
        {
            const uint32 numNodes = mActor->GetSkeleton()->GetNumNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                mLocalSpaceTransforms[i].Zero();
            }

            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(mActor->GetMorphSetup(0)->GetNumMorphTargets() == numMorphs);
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] = 0.0f;
            }
        }

        InvalidateAllModelSpaceTransforms();
    }


    // normalize all quaternions
    void Pose::NormalizeQuaternions()
    {
        if (mActorInstance)
        {
            uint32 nodeNr;
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                nodeNr = mActorInstance->GetEnabledNode(i);
                UpdateLocalSpaceTransform(nodeNr);
                mLocalSpaceTransforms[nodeNr].mRotation.Normalize();
            }
        }
        else
        {
            const uint32 numNodes = mActor->GetSkeleton()->GetNumNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                UpdateLocalSpaceTransform(i);
                mLocalSpaceTransforms[i].mRotation.Normalize();
            }
        }
    }


    // add the transforms of another pose to this one
    void Pose::Sum(const Pose* other, float weight)
    {
        if (mActorInstance)
        {
            uint32 nodeNr;
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                nodeNr = mActorInstance->GetEnabledNode(i);

                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(nodeNr));
                const Transform& otherTransform = other->GetLocalSpaceTransform(nodeNr);
                transform.Add(otherTransform, weight);
            }

            // blend the morph weights
            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(mActorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == other->GetNumMorphWeights());
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] += other->mMorphWeights[i] * weight;
            }
        }
        else
        {
            const uint32 numNodes = mActor->GetSkeleton()->GetNumNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(i));
                const Transform& otherTransform = other->GetLocalSpaceTransform(i);
                transform.Add(otherTransform, weight);
            }

            // blend the morph weights
            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(mActor->GetMorphSetup(0)->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == other->GetNumMorphWeights());
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] += other->mMorphWeights[i] * weight;
            }
        }

        InvalidateAllModelSpaceTransforms();
    }


    // blend, without motion instance
    void Pose::Blend(const Pose* destPose, float weight)
    {
        if (mActorInstance)
        {
            uint32 nodeNr;
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                nodeNr = mActorInstance->GetEnabledNode(i);
                Transform& curTransform = const_cast<Transform&>(GetLocalSpaceTransform(nodeNr));
                curTransform.Blend(destPose->GetLocalSpaceTransform(nodeNr), weight);
            }

            // blend the morph weights
            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(mActorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] = MCore::LinearInterpolate<float>(mMorphWeights[i], destPose->mMorphWeights[i], weight);
            }

            for (const auto& poseDataItem : m_poseDatas)
            {
                PoseData* poseData = poseDataItem.second.get();
                poseData->Blend(destPose, weight);
            }
        }
        else
        {
            const uint32 numNodes = mActor->GetSkeleton()->GetNumNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                Transform& curTransform = const_cast<Transform&>(GetLocalSpaceTransform(i));
                curTransform.Blend(destPose->GetLocalSpaceTransform(i), weight);
            }

            // blend the morph weights
            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(mActor->GetMorphSetup(0)->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] = MCore::LinearInterpolate<float>(mMorphWeights[i], destPose->mMorphWeights[i], weight);
            }

            for (const auto& poseDataItem : m_poseDatas)
            {
                PoseData* poseData = poseDataItem.second.get();
                poseData->Blend(destPose, weight);
            }
        }

        InvalidateAllModelSpaceTransforms();
    }


    Pose& Pose::MakeRelativeTo(const Pose& other)
    {
        AZ_Assert(mLocalSpaceTransforms.GetLength() == other.mLocalSpaceTransforms.GetLength(), "Poses must be of the same size");
        if (mActorInstance)
        {
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                uint16 nodeNr = mActorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(nodeNr));
                transform = transform.CalcRelativeTo(other.GetLocalSpaceTransform(nodeNr));
            }
        }
        else
        {
            const uint32 numNodes = mLocalSpaceTransforms.GetLength();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(i));
                transform = transform.CalcRelativeTo(other.GetLocalSpaceTransform(i));
            }
        }

        const uint32 numMorphs = mMorphWeights.GetLength();
        AZ_Assert(numMorphs == other.GetNumMorphWeights(), "Number of morphs in the pose doesn't match the number of morphs inside the provided input pose.");
        for (uint32 i = 0; i < numMorphs; ++i)
        {
            mMorphWeights[i] -= other.mMorphWeights[i];
        }

        InvalidateAllModelSpaceTransforms();
        return *this;
    }


    Pose& Pose::ApplyAdditive(const Pose& additivePose, float weight)
    {
        AZ_Assert(mLocalSpaceTransforms.GetLength() == additivePose.mLocalSpaceTransforms.GetLength(), "Poses must be of the same size");
        if (mActorInstance)
        {
            AZ_Assert(weight > -MCore::Math::epsilon && weight < (1 + MCore::Math::epsilon), "Expected weight to be between 0..1");
        }
        static const float weightCloseToOne = 1.0f - MCore::Math::epsilon;

        if (weight < MCore::Math::epsilon)
        {
            return *this;
        }
        else if (weight > weightCloseToOne)
        {

            return ApplyAdditive(additivePose);
        }
        else
        {
            AZ_Assert(mLocalSpaceTransforms.GetLength() == additivePose.mLocalSpaceTransforms.GetLength(), "Poses must be of the same size");
            if (mActorInstance)
            {
                const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    uint16 nodeNr = mActorInstance->GetEnabledNode(i);
                    Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(nodeNr));
                    const Transform& additiveTransform = additivePose.GetLocalSpaceTransform(nodeNr);
                    transform.mPosition += additiveTransform.mPosition * weight;
                    transform.mRotation = transform.mRotation.NLerp(additiveTransform.mRotation * transform.mRotation, weight);
                    EMFX_SCALECODE
                    (
                        transform.mScale *= AZ::Vector3::CreateOne().Lerp(additiveTransform.mScale, weight);
                    )
                    transform.mRotation.Normalize();
                }
            }
            else
            {
                const uint32 numNodes = mLocalSpaceTransforms.GetLength();
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(i));
                    const Transform& additiveTransform = additivePose.GetLocalSpaceTransform(i);
                    transform.mPosition += additiveTransform.mPosition * weight;
                    transform.mRotation = transform.mRotation.NLerp(additiveTransform.mRotation * transform.mRotation, weight);
                    EMFX_SCALECODE
                    (
                        transform.mScale *= AZ::Vector3::CreateOne().Lerp(additiveTransform.mScale, weight);
                    )
                    transform.mRotation.Normalize();
                }
            }

            const uint32 numMorphs = mMorphWeights.GetLength();
            AZ_Assert(numMorphs == additivePose.GetNumMorphWeights(), "Number of morphs in the pose doesn't match the number of morphs inside the provided input pose.");
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] += additivePose.mMorphWeights[i] * weight;
            }

            InvalidateAllModelSpaceTransforms();
            return *this;
        }
    }


    Pose& Pose::ApplyAdditive(const Pose& additivePose)
    {
        AZ_Assert(mLocalSpaceTransforms.GetLength() == additivePose.mLocalSpaceTransforms.GetLength(), "Poses must be of the same size");
        if (mActorInstance)
        {
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                uint16 nodeNr = mActorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(nodeNr));
                const Transform& additiveTransform = additivePose.GetLocalSpaceTransform(nodeNr);
                transform.mPosition += additiveTransform.mPosition;
                transform.mRotation = transform.mRotation * additiveTransform.mRotation;
                EMFX_SCALECODE
                (
                    transform.mScale *= additiveTransform.mScale;
                )
                transform.mRotation.Normalize();
            }
        }
        else
        {
            const uint32 numNodes = mLocalSpaceTransforms.GetLength();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(i));
                const Transform& additiveTransform = additivePose.GetLocalSpaceTransform(i);
                transform.mPosition += additiveTransform.mPosition;
                transform.mRotation = transform.mRotation * additiveTransform.mRotation;
                EMFX_SCALECODE
                (
                    transform.mScale *= additiveTransform.mScale;
                )
                transform.mRotation.Normalize();
            }
        }

        const uint32 numMorphs = mMorphWeights.GetLength();
        AZ_Assert(numMorphs == additivePose.GetNumMorphWeights(), "Number of morphs in the pose doesn't match the number of morphs inside the provided input pose.");
        for (uint32 i = 0; i < numMorphs; ++i)
        {
            mMorphWeights[i] += additivePose.mMorphWeights[i];
        }

        InvalidateAllModelSpaceTransforms();
        return *this;
    }


    Pose& Pose::MakeAdditive(const Pose& refPose)
    {
        AZ_Assert(mLocalSpaceTransforms.GetLength() == refPose.mLocalSpaceTransforms.GetLength(), "Poses must be of the same size");
        if (mActorInstance)
        {
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                uint16 nodeNr = mActorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(nodeNr));
                const Transform& refTransform = refPose.GetLocalSpaceTransform(nodeNr);
                transform.mPosition = transform.mPosition - refTransform.mPosition;
                transform.mRotation = refTransform.mRotation.GetConjugate() * transform.mRotation;
                EMFX_SCALECODE
                (
                    transform.mScale *= refTransform.mScale;
                )
            }
        }
        else
        {
            const uint32 numNodes = mLocalSpaceTransforms.GetLength();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(i));
                const Transform& refTransform = refPose.GetLocalSpaceTransform(i);
                transform.mPosition = transform.mPosition - refTransform.mPosition;
                transform.mRotation = refTransform.mRotation.GetConjugate() * transform.mRotation;
                EMFX_SCALECODE
                (
                    transform.mScale *= refTransform.mScale;
                )
            }
        }

        const uint32 numMorphs = mMorphWeights.GetLength();
        AZ_Assert(numMorphs == refPose.GetNumMorphWeights(), "Number of morphs in the pose doesn't match the number of morphs inside the provided input pose.");
        for (uint32 i = 0; i < numMorphs; ++i)
        {
            mMorphWeights[i] -= refPose.mMorphWeights[i];
        }

        InvalidateAllModelSpaceTransforms();
        return *this;
    }


    // additive blend
    void Pose::BlendAdditiveUsingBindPose(const Pose* destPose, float weight)
    {
        if (mActorInstance)
        {
            const TransformData* transformData = mActorInstance->GetTransformData();
            Pose* bindPose = transformData->GetBindPose();
            Transform result;

            uint32 nodeNr;
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                nodeNr = mActorInstance->GetEnabledNode(i);
                BlendTransformAdditiveUsingBindPose(bindPose->GetLocalSpaceTransform(nodeNr), GetLocalSpaceTransform(nodeNr), destPose->GetLocalSpaceTransform(nodeNr), weight, &result);
                SetLocalSpaceTransform(nodeNr, result, false);
            }

            // blend the morph weights
            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(mActorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] += destPose->mMorphWeights[i] * weight;
            }
        }
        else
        {
            const TransformData* transformData = mActorInstance->GetTransformData();
            Pose* bindPose = transformData->GetBindPose();
            Transform result;

            const uint32 numNodes = mActor->GetSkeleton()->GetNumNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                BlendTransformAdditiveUsingBindPose(bindPose->GetLocalSpaceTransform(i), GetLocalSpaceTransform(i), destPose->GetLocalSpaceTransform(i), weight, &result);
                SetLocalSpaceTransform(i, result, false);
            }

            // blend the morph weights
            const uint32 numMorphs = mMorphWeights.GetLength();
            MCORE_ASSERT(mActor->GetMorphSetup(0)->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (uint32 i = 0; i < numMorphs; ++i)
            {
                mMorphWeights[i] += destPose->mMorphWeights[i] * weight;
            }
        }

        InvalidateAllModelSpaceTransforms();
    }


    // blend a transformation with weight check optimization
    void Pose::BlendTransformWithWeightCheck(const Transform& source, const Transform& dest, float weight, Transform* outTransform)
    {
        if (weight >= 1.0f)
        {
            *outTransform = dest;
        }
        else
        {
            if (weight > 0.0f)
            {
                *outTransform = source;
                outTransform->Blend(dest, weight);
            }
            else
            {
                *outTransform = source;
            }
        }
    }


    // blend two transformations additively
    void Pose::BlendTransformAdditiveUsingBindPose(const Transform& baseLocalTransform, const Transform& source, const Transform& dest, float weight, Transform* outTransform)
    {
        // get the node index
        *outTransform = source;
        outTransform->BlendAdditive(dest, baseLocalTransform, weight);
    }


    Pose& Pose::operator=(const Pose& other)
    {
        InitFromPose(&other);
        return *this;
    }

    bool Pose::HasPoseData(const AZ::TypeId& typeId) const
    {
        return m_poseDatas.find(typeId) != m_poseDatas.end();
    }

    PoseData* Pose::GetPoseDataByType(const AZ::TypeId& typeId) const
    {
        const auto iterator = m_poseDatas.find(typeId);
        if (iterator != m_poseDatas.end())
        {
            return iterator->second.get();
        }

        return nullptr;
    }

    void Pose::AddPoseData(PoseData* poseData)
    {
        m_poseDatas.emplace(poseData->RTTI_GetType(), poseData);
    }

    void Pose::ClearPoseDatas()
    {
        m_poseDatas.clear();
    }

    const AZStd::unordered_map<AZ::TypeId, AZStd::unique_ptr<PoseData> >& Pose::GetPoseDatas() const
    {
        return m_poseDatas;
    }

    PoseData* Pose::GetAndPreparePoseData(const AZ::TypeId& typeId, ActorInstance* linkToActorInstance)
    {
        PoseData* poseData = GetPoseDataByType(typeId);
        if (!poseData)
        {
            poseData = PoseDataFactory::Create(this, typeId);
            AddPoseData(poseData);

            poseData->LinkToActorInstance(linkToActorInstance);
            poseData->Reset();
        }
        else
        {
            poseData->LinkToActorInstance(linkToActorInstance);

            if (!poseData->IsUsed())
            {
                poseData->Reset();
            }
        }

        poseData->SetIsUsed(true);
        return poseData;
    }

    // compensate for motion extraction, basically making it in-place
    void Pose::CompensateForMotionExtractionDirect(EMotionExtractionFlags motionExtractionFlags)
    {
        const uint32 motionExtractionNodeIndex = mActor->GetMotionExtractionNodeIndex();
        if (motionExtractionNodeIndex != MCORE_INVALIDINDEX32)
        {
            Transform motionExtractionNodeTransform = GetLocalSpaceTransformDirect(motionExtractionNodeIndex);
            mActorInstance->MotionExtractionCompensate(motionExtractionNodeTransform, motionExtractionFlags);
            SetLocalSpaceTransformDirect(motionExtractionNodeIndex, motionExtractionNodeTransform);
        }
    }


    // compensate for motion extraction, basically making it in-place
    void Pose::CompensateForMotionExtraction(EMotionExtractionFlags motionExtractionFlags)
    {
        const uint32 motionExtractionNodeIndex = mActor->GetMotionExtractionNodeIndex();
        if (motionExtractionNodeIndex != MCORE_INVALIDINDEX32)
        {
            Transform motionExtractionNodeTransform = GetLocalSpaceTransform(motionExtractionNodeIndex);
            mActorInstance->MotionExtractionCompensate(motionExtractionNodeTransform, motionExtractionFlags);
            SetLocalSpaceTransform(motionExtractionNodeIndex, motionExtractionNodeTransform);
        }
    }


    // apply the morph target weights to the morph setup instance of the given actor instance
    void Pose::ApplyMorphWeightsToActorInstance()
    {
        MorphSetupInstance* morphSetupInstance = mActorInstance->GetMorphSetupInstance();
        const uint32 numMorphs = morphSetupInstance->GetNumMorphTargets();
        for (uint32 m = 0; m < numMorphs; ++m)
        {
            MorphSetupInstance::MorphTarget* morphTarget = morphSetupInstance->GetMorphTarget(m);
            if (morphTarget->GetIsInManualMode() == false)
            {
                morphTarget->SetWeight(mMorphWeights[m]);
            }
        }
    }


    // zero all morph weights
    void Pose::ZeroMorphWeights()
    {
        const uint32 numMorphs = mMorphWeights.GetLength();
        for (uint32 m = 0; m < numMorphs; ++m)
        {
            mMorphWeights[m] = 0.0f;
        }
    }


    void Pose::ResizeNumMorphs(uint32 numMorphTargets)
    {
        mMorphWeights.Resize(numMorphTargets);
    }


    Pose& Pose::PreMultiply(const Pose& other)
    {
        AZ_Assert(mLocalSpaceTransforms.GetLength() == other.mLocalSpaceTransforms.GetLength(), "Poses must be of the same size");
        if (mActorInstance)
        {
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                uint16 nodeNr = mActorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(nodeNr));
                Transform otherTransform = other.GetLocalSpaceTransform(nodeNr);
                transform = otherTransform * transform;
            }
        }
        else
        {
            const uint32 numNodes = mLocalSpaceTransforms.GetLength();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(i));
                Transform otherTransform = other.GetLocalSpaceTransform(i);
                transform = otherTransform * transform;
            }
        }

        InvalidateAllModelSpaceTransforms();
        return *this;
    }


    Pose& Pose::Multiply(const Pose& other)
    {
        AZ_Assert(mLocalSpaceTransforms.GetLength() == other.mLocalSpaceTransforms.GetLength(), "Poses must be of the same size");
        if (mActorInstance)
        {
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                uint16 nodeNr = mActorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(nodeNr));
                transform.Multiply(other.GetLocalSpaceTransform(nodeNr));
            }
        }
        else
        {
            const uint32 numNodes = mLocalSpaceTransforms.GetLength();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(i));
                transform.Multiply(other.GetLocalSpaceTransform(i));
            }
        }

        InvalidateAllModelSpaceTransforms();
        return *this;
    }


    Pose& Pose::MultiplyInverse(const Pose& other)
    {
        AZ_Assert(mLocalSpaceTransforms.GetLength() == other.mLocalSpaceTransforms.GetLength(), "Poses must be of the same size");
        if (mActorInstance)
        {
            const uint32 numNodes = mActorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                uint16 nodeNr = mActorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(nodeNr));
                Transform otherTransform = other.GetLocalSpaceTransform(nodeNr);
                otherTransform.Inverse();
                transform.PreMultiply(otherTransform);
            }
        }
        else
        {
            const uint32 numNodes = mLocalSpaceTransforms.GetLength();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(i));
                Transform otherTransform = other.GetLocalSpaceTransform(i);
                otherTransform.Inverse();
                transform.PreMultiply(otherTransform);
            }
        }

        InvalidateAllModelSpaceTransforms();
        return *this;
    }


    Transform Pose::GetMeshNodeWorldSpaceTransform(AZ::u32 lodLevel, AZ::u32 nodeIndex) const
    {
        if (!mActorInstance)
        {
            return Transform::CreateIdentity();
        }

        Actor* actor = mActorInstance->GetActor();
        if (actor->CheckIfHasSkinningDeformer(lodLevel, nodeIndex))
        {
            return mActorInstance->GetWorldSpaceTransform();
        }

        return GetWorldSpaceTransform(nodeIndex);
    }


    void Pose::Mirror(const MotionLinkData* motionLinkData)
    {
        AZ_Assert(motionLinkData, "Expecting valid motionLinkData pointer.");
        AZ_Assert(mActorInstance, "Mirroring is only possible in combination with an actor instance.");

        const Actor* actor = mActorInstance->GetActor();
        const TransformData* transformData = mActorInstance->GetTransformData();
        const Pose* bindPose = transformData->GetBindPose();
        const AZStd::vector<AZ::u32>& jointLinks = motionLinkData->GetJointDataLinks();

        AnimGraphPose* tempPose = GetEMotionFX().GetThreadData(mActorInstance->GetThreadIndex())->GetPosePool().RequestPose(mActorInstance);
        Pose& unmirroredPose = tempPose->GetPose();
        unmirroredPose = *this;

        const AZ::u32 numNodes = mActorInstance->GetNumEnabledNodes();
        for (AZ::u32 i = 0; i < numNodes; ++i)
        {
            const AZ::u32 nodeNumber = mActorInstance->GetEnabledNode(i);
            const AZ::u32 jointDataIndex = jointLinks[nodeNumber];
            if (jointDataIndex == InvalidIndex32)
            {
                continue;
            }

            const Actor::NodeMirrorInfo& mirrorInfo = actor->GetNodeMirrorInfo(nodeNumber);

            Transform mirrored = bindPose->GetLocalSpaceTransform(nodeNumber);
            AZ::Vector3 mirrorAxis = AZ::Vector3::CreateZero();
            mirrorAxis.SetElement(mirrorInfo.mAxis, 1.0f);
            mirrored.ApplyDeltaMirrored(bindPose->GetLocalSpaceTransform(mirrorInfo.mSourceNode), unmirroredPose.GetLocalSpaceTransform(mirrorInfo.mSourceNode), mirrorAxis, mirrorInfo.mFlags);

            SetLocalSpaceTransformDirect(nodeNumber, mirrored);
        }

        GetEMotionFX().GetThreadData(mActorInstance->GetThreadIndex())->GetPosePool().FreePose(tempPose);
    }
}   // namespace EMotionFX
