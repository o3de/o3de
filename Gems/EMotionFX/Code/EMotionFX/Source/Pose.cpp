/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        m_actorInstance  = nullptr;
        m_actor          = nullptr;
        m_skeleton       = nullptr;
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
        m_actorInstance  = actorInstance;
        m_actor          = actorInstance->GetActor();
        m_skeleton       = m_actor->GetSkeleton();

        // resize the buffers
        const size_t numTransforms = m_actor->GetSkeleton()->GetNumNodes();
        m_localSpaceTransforms.resize_no_construct(numTransforms);
        m_modelSpaceTransforms.resize_no_construct(numTransforms);
        m_flags.resize_no_construct(numTransforms);
        m_morphWeights.resize_no_construct(actorInstance->GetMorphSetupInstance()->GetNumMorphTargets());

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
        m_actorInstance  = nullptr;
        m_actor          = actor;
        m_skeleton       = actor->GetSkeleton();

        // resize the buffers
        const size_t numTransforms = m_actor->GetSkeleton()->GetNumNodes();
        m_localSpaceTransforms.resize_no_construct(numTransforms);
        m_modelSpaceTransforms.resize_no_construct(numTransforms);

        const size_t oldSize = m_flags.size();
        m_flags.resize_no_construct(numTransforms);
        if (oldSize < numTransforms && clearAllFlags == false)
        {
            for (size_t i = oldSize; i < numTransforms; ++i)
            {
                m_flags[i] = initialFlags;
            }
        }

        MorphSetup* morphSetup = m_actor->GetMorphSetup(0);
        m_morphWeights.resize_no_construct((morphSetup) ? morphSetup->GetNumMorphTargets() : 0);

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
    void Pose::SetNumTransforms(size_t numTransforms)
    {
        // resize the buffers
        m_localSpaceTransforms.resize_no_construct(numTransforms);
        m_modelSpaceTransforms.resize_no_construct(numTransforms);

        const size_t oldSize = m_flags.size();
        m_flags.resize_no_construct(numTransforms);

        for (size_t i = oldSize; i < numTransforms; ++i)
        {
            m_flags[i] = 0;
            SetLocalSpaceTransform(i, Transform::CreateIdentity());
        }
    }


    void Pose::Clear(bool clearMem)
    {
        const auto clear = [clearMem](auto& vector)
        {
            vector.clear();
            if (clearMem)
            {
                vector.shrink_to_fit();
            }
        };
        clear(m_localSpaceTransforms);
        clear(m_modelSpaceTransforms);
        clear(m_flags);
        clear(m_morphWeights);

        ClearPoseDatas();
    }


    // clear the pose flags
    void Pose::ClearFlags(uint8 newFlags)
    {
        MCore::MemSet((uint8*)m_flags.data(), newFlags, sizeof(uint8) * m_flags.size());
    }


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


    // update the full local space pose
    void Pose::ForceUpdateFullLocalSpacePose()
    {
        Skeleton* skeleton = m_actor->GetSkeleton();
        const size_t numNodes = skeleton->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            const size_t parentIndex = skeleton->GetNode(i)->GetParentIndex();
            if (parentIndex != InvalidIndex)
            {
                GetModelSpaceTransform(parentIndex, &m_localSpaceTransforms[i]);
                m_localSpaceTransforms[i].Inverse();
                m_localSpaceTransforms[i].PreMultiply(m_modelSpaceTransforms[i]);
            }
            else
            {
                m_localSpaceTransforms[i] = m_modelSpaceTransforms[i];
            }

            m_flags[i] |= FLAG_LOCALTRANSFORMREADY;
        }
    }


    // update the full model space pose
    void Pose::ForceUpdateFullModelSpacePose()
    {
        // iterate from root towards child nodes recursively, updating all model space transforms on the way
        Skeleton* skeleton = m_actor->GetSkeleton();
        const size_t numNodes = skeleton->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            const size_t parentIndex = skeleton->GetNode(i)->GetParentIndex();
            if (parentIndex != InvalidIndex)
            {
                m_modelSpaceTransforms[parentIndex].PreMultiply(m_localSpaceTransforms[i], &m_modelSpaceTransforms[i]);
            }
            else
            {
                m_modelSpaceTransforms[i] = m_localSpaceTransforms[i];
            }

            m_flags[i] |= FLAG_MODELTRANSFORMREADY;
        }
    }


    // recursively update
    void Pose::UpdateModelSpaceTransform(size_t nodeIndex) const
    {
        Skeleton* skeleton = m_actor->GetSkeleton();

        const size_t parentIndex = skeleton->GetNode(nodeIndex)->GetParentIndex();
        if (parentIndex != InvalidIndex && !(m_flags[parentIndex] & FLAG_MODELTRANSFORMREADY))
        {
            UpdateModelSpaceTransform(parentIndex);
        }

        // update the model space transform if needed
        if ((m_flags[nodeIndex] & FLAG_MODELTRANSFORMREADY) == false)
        {
            const Transform& localTransform = GetLocalSpaceTransform(nodeIndex);
            if (parentIndex != InvalidIndex)
            {
                m_modelSpaceTransforms[parentIndex].PreMultiply(localTransform, &m_modelSpaceTransforms[nodeIndex]);
            }
            else
            {
                m_modelSpaceTransforms[nodeIndex] = m_localSpaceTransforms[nodeIndex];
            }

            m_flags[nodeIndex] |= FLAG_MODELTRANSFORMREADY;
        }
    }


    // update the local transform
    void Pose::UpdateLocalSpaceTransform(size_t nodeIndex) const
    {
        const uint8 flags = m_flags[nodeIndex];
        if (flags & FLAG_LOCALTRANSFORMREADY)
        {
            return;
        }

        MCORE_ASSERT(flags & FLAG_MODELTRANSFORMREADY); // the model space transform has to be updated already, otherwise we cannot possibly calculate the local space one

        Skeleton* skeleton = m_actor->GetSkeleton();
        const size_t parentIndex = skeleton->GetNode(nodeIndex)->GetParentIndex();
        if (parentIndex != InvalidIndex)
        {
            GetModelSpaceTransform(parentIndex, &m_localSpaceTransforms[nodeIndex]);
            m_localSpaceTransforms[nodeIndex].Inverse();
            m_localSpaceTransforms[nodeIndex].PreMultiply(m_modelSpaceTransforms[nodeIndex]);
        }
        else
        {
            m_localSpaceTransforms[nodeIndex] = m_modelSpaceTransforms[nodeIndex];
        }

        m_flags[nodeIndex] |= FLAG_LOCALTRANSFORMREADY;
    }


    // get the local transform
    const Transform& Pose::GetLocalSpaceTransform(size_t nodeIndex) const
    {
        UpdateLocalSpaceTransform(nodeIndex);
        return m_localSpaceTransforms[nodeIndex];
    }


    const Transform& Pose::GetModelSpaceTransform(size_t nodeIndex) const
    {
        UpdateModelSpaceTransform(nodeIndex);
        return m_modelSpaceTransforms[nodeIndex];
    }


    Transform Pose::GetWorldSpaceTransform(size_t nodeIndex) const
    {
        UpdateModelSpaceTransform(nodeIndex);
        return m_modelSpaceTransforms[nodeIndex].Multiplied(m_actorInstance->GetWorldSpaceTransform());
    }


    void Pose::GetWorldSpaceTransform(size_t nodeIndex, Transform* outResult) const
    {
        UpdateModelSpaceTransform(nodeIndex);
        *outResult = m_modelSpaceTransforms[nodeIndex];
        outResult->Multiply(m_actorInstance->GetWorldSpaceTransform());
    }


    // calculate a local transform
    void Pose::GetLocalSpaceTransform(size_t nodeIndex, Transform* outResult) const
    {
        if ((m_flags[nodeIndex] & FLAG_LOCALTRANSFORMREADY) == false)
        {
            UpdateLocalSpaceTransform(nodeIndex);
        }

        *outResult = m_localSpaceTransforms[nodeIndex];
    }


    void Pose::GetModelSpaceTransform(size_t nodeIndex, Transform* outResult) const
    {
        UpdateModelSpaceTransform(nodeIndex);
        *outResult = m_modelSpaceTransforms[nodeIndex];
    }


    // set the local transform
    void Pose::SetLocalSpaceTransform(size_t nodeIndex, const Transform& newTransform, bool invalidateGlobalTransforms)
    {
        m_localSpaceTransforms[nodeIndex] = newTransform;
        m_flags[nodeIndex] |= FLAG_LOCALTRANSFORMREADY;

        // mark all child node model space transforms as dirty (recursively)
        if (invalidateGlobalTransforms)
        {
            if (m_flags[nodeIndex] & FLAG_MODELTRANSFORMREADY)
            {
                RecursiveInvalidateModelSpaceTransforms(m_actor, nodeIndex);
            }
        }
    }


    // mark all child nodes recursively as dirty
    void Pose::RecursiveInvalidateModelSpaceTransforms(const Actor* actor, size_t nodeIndex)
    {
        // if this model space transform ain't ready yet assume all child nodes are also not
        if ((m_flags[nodeIndex] & FLAG_MODELTRANSFORMREADY) == false)
        {
            return;
        }

        // mark the global transform as invalid
        m_flags[nodeIndex] &= ~FLAG_MODELTRANSFORMREADY;

        // recurse through all child nodes
        Skeleton* skeleton = actor->GetSkeleton();
        Node* node = skeleton->GetNode(nodeIndex);
        const size_t numChildNodes = node->GetNumChildNodes();
        for (size_t i = 0; i < numChildNodes; ++i)
        {
            RecursiveInvalidateModelSpaceTransforms(actor, node->GetChildIndex(i));
        }
    }


    void Pose::SetModelSpaceTransform(size_t nodeIndex, const Transform& newTransform, bool invalidateChildGlobalTransforms)
    {
        m_modelSpaceTransforms[nodeIndex] = newTransform;

        // invalidate the local transform
        m_flags[nodeIndex] &= ~FLAG_LOCALTRANSFORMREADY;

        // recursively invalidate all model space transforms of all child nodes
        if (invalidateChildGlobalTransforms)
        {
            RecursiveInvalidateModelSpaceTransforms(m_actor, nodeIndex);
        }

        // mark this model space transform as ready
        m_flags[nodeIndex] |= FLAG_MODELTRANSFORMREADY;
        UpdateLocalSpaceTransform(nodeIndex);
    }


    void Pose::SetWorldSpaceTransform(size_t nodeIndex, const Transform& newTransform, bool invalidateChildGlobalTransforms)
    {
        m_modelSpaceTransforms[nodeIndex] = newTransform.Multiplied(m_actorInstance->GetWorldSpaceTransformInversed());
        m_flags[nodeIndex] &= ~FLAG_LOCALTRANSFORMREADY;

        if (invalidateChildGlobalTransforms)
        {
            RecursiveInvalidateModelSpaceTransforms(m_actor, nodeIndex);
        }

        m_flags[nodeIndex] |= FLAG_MODELTRANSFORMREADY;
        UpdateLocalSpaceTransform(nodeIndex);
    }


    // invalidate all local transforms
    void Pose::InvalidateAllLocalSpaceTransforms()
    {
        for (uint8& flag : m_flags)
        {
            flag &= ~FLAG_LOCALTRANSFORMREADY;
        }
    }


    void Pose::InvalidateAllModelSpaceTransforms()
    {
        for (uint8& flag : m_flags)
        {
            flag &= ~FLAG_MODELTRANSFORMREADY;
        }
    }


    void Pose::InvalidateAllLocalAndModelSpaceTransforms()
    {
        for (uint8& flag : m_flags)
        {
            flag &= ~(FLAG_LOCALTRANSFORMREADY | FLAG_MODELTRANSFORMREADY);
        }
    }


    Transform Pose::CalcTrajectoryTransform() const
    {
        MCORE_ASSERT(m_actor);
        const size_t motionExtractionNodeIndex = m_actor->GetMotionExtractionNodeIndex();
        if (motionExtractionNodeIndex == InvalidIndex)
        {
            return Transform::CreateIdentity();
        }

        return GetWorldSpaceTransform(motionExtractionNodeIndex).ProjectedToGroundPlane();
    }


    void Pose::UpdateAllLocalSpaceTranforms()
    {
        Skeleton* skeleton = m_actor->GetSkeleton();
        const size_t numNodes = skeleton->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            UpdateLocalSpaceTransform(i);
        }
    }


    void Pose::UpdateAllModelSpaceTranforms()
    {
        Skeleton* skeleton = m_actor->GetSkeleton();
        const size_t numNodes = skeleton->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
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
        MCORE_ASSERT(m_localSpaceTransforms.size() == destPose->m_localSpaceTransforms.size());
        MCORE_ASSERT(m_localSpaceTransforms.size() == outPose->m_localSpaceTransforms.size());
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
                    const size_t numNodes = actorInstance->GetNumEnabledNodes();
                    for (size_t i = 0; i < numNodes; ++i)
                    {
                        const uint16 nodeNr = actorInstance->GetEnabledNode(i);
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
            const size_t numMorphs = m_morphWeights.size();
            MCORE_ASSERT(actorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (size_t i = 0; i < numMorphs; ++i)
            {
                m_morphWeights[i] = MCore::LinearInterpolate<float>(m_morphWeights[i], destPose->m_morphWeights[i], weight);
            }
        }
        else
        {
            TransformData* transformData = instance->GetActorInstance()->GetTransformData();
            const Pose* bindPose = transformData->GetBindPose();
            Transform result;
            const size_t numNodes = actorInstance->GetNumEnabledNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                const uint16 nodeNr = actorInstance->GetEnabledNode(i);
                const Transform& base = bindPose->GetLocalSpaceTransform(nodeNr);
                BlendTransformAdditiveUsingBindPose(base, GetLocalSpaceTransform(nodeNr), destPose->GetLocalSpaceTransform(nodeNr), weight, &result);
                outPose->SetLocalSpaceTransform(nodeNr, result, false);
            }
            outPose->InvalidateAllModelSpaceTransforms();

            // blend the morph weights
            const size_t numMorphs = m_morphWeights.size();
            MCORE_ASSERT(actorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (size_t i = 0; i < numMorphs; ++i)
            {
                m_morphWeights[i] += destPose->m_morphWeights[i] * weight;
            }
        }
    }


    // blend two poses, mixing
    void Pose::BlendMixed(const Pose* destPose, float weight, const MotionInstance* instance, Pose* outPose)
    {
        // make sure the number of transforms are equal
        MCORE_ASSERT(destPose);
        MCORE_ASSERT(outPose);
        MCORE_ASSERT(m_localSpaceTransforms.size() == destPose->m_localSpaceTransforms.size());
        MCORE_ASSERT(m_localSpaceTransforms.size() == outPose->m_localSpaceTransforms.size());
        MCORE_ASSERT(instance->GetIsMixing());

        const bool additive = (instance->GetBlendMode() == BLENDMODE_ADDITIVE);

        // get the actor from the instance
        ActorInstance* actorInstance = instance->GetActorInstance();

        // get the transform data
        TransformData* transformData = instance->GetActorInstance()->GetTransformData();
        Transform result;

        const MotionLinkData* motionLinkData = instance->GetMotion()->GetMotionData()->FindMotionLinkData(actorInstance->GetActor());
        AZ_Assert(motionLinkData->GetJointDataLinks().size() == m_localSpaceTransforms.size(), "Expecting there to be the same amount of motion links as pose transforms.");

        // blend all transforms
        if (!additive)
        {
            const size_t numNodes = actorInstance->GetNumEnabledNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                const uint16 nodeNr = actorInstance->GetEnabledNode(i);

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
            const size_t numMorphs = m_morphWeights.size();
            MCORE_ASSERT(actorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (size_t i = 0; i < numMorphs; ++i)
            {
                m_morphWeights[i] = MCore::LinearInterpolate<float>(m_morphWeights[i], destPose->m_morphWeights[i], weight);
            }
        }
        else
        {
            Pose* bindPose = transformData->GetBindPose();
            const size_t numNodes = actorInstance->GetNumEnabledNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                const uint16 nodeNr = actorInstance->GetEnabledNode(i);

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
            const size_t numMorphs = m_morphWeights.size();
            MCORE_ASSERT(actorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (size_t i = 0; i < numMorphs; ++i)
            {
                m_morphWeights[i] += destPose->m_morphWeights[i] * weight;
            }
        }
    }


    // init from another pose
    void Pose::InitFromPose(const Pose* sourcePose)
    {
        if (!sourcePose)
        {
            if (m_actorInstance)
            {
                InitFromBindPose(m_actorInstance);
            }
            else
            {
                InitFromBindPose(m_actor);
            }

            return;
        }

        m_modelSpaceTransforms = sourcePose->m_modelSpaceTransforms;
        m_localSpaceTransforms = sourcePose->m_localSpaceTransforms;
        m_flags = sourcePose->m_flags;
        m_morphWeights = sourcePose->m_morphWeights;

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
        if (m_actorInstance)
        {
            const size_t numNodes = m_actorInstance->GetNumEnabledNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                const uint16 nodeNr = m_actorInstance->GetEnabledNode(i);
                m_localSpaceTransforms[nodeNr].Zero();
            }

            [[maybe_unused]] const size_t numMorphs = m_morphWeights.size();
            MCORE_ASSERT(m_actorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            for (float& morphWeight : m_morphWeights)
            {
                morphWeight = 0.0f;
            }
        }
        else
        {
            const size_t numNodes = m_actor->GetSkeleton()->GetNumNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                m_localSpaceTransforms[i].Zero();
            }

            [[maybe_unused]] const size_t numMorphs = m_morphWeights.size();
            MCORE_ASSERT(m_actor->GetMorphSetup(0)->GetNumMorphTargets() == numMorphs);
            for (float& morphWeight : m_morphWeights)
            {
                morphWeight = 0.0f;
            }
        }

        InvalidateAllModelSpaceTransforms();
    }


    // normalize all quaternions
    void Pose::NormalizeQuaternions()
    {
        if (m_actorInstance)
        {
            const size_t numNodes = m_actorInstance->GetNumEnabledNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                const uint16 nodeNr = m_actorInstance->GetEnabledNode(i);
                UpdateLocalSpaceTransform(nodeNr);
                m_localSpaceTransforms[nodeNr].m_rotation.Normalize();
            }
        }
        else
        {
            const size_t numNodes = m_actor->GetSkeleton()->GetNumNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                UpdateLocalSpaceTransform(i);
                m_localSpaceTransforms[i].m_rotation.Normalize();
            }
        }
    }


    // add the transforms of another pose to this one
    void Pose::Sum(const Pose* other, float weight)
    {
        if (m_actorInstance)
        {
            const size_t numNodes = m_actorInstance->GetNumEnabledNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                const uint16 nodeNr = m_actorInstance->GetEnabledNode(i);

                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(nodeNr));
                const Transform& otherTransform = other->GetLocalSpaceTransform(nodeNr);
                transform.Add(otherTransform, weight);
            }

            // blend the morph weights
            const size_t numMorphs = m_morphWeights.size();
            MCORE_ASSERT(m_actorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == other->GetNumMorphWeights());
            for (size_t i = 0; i < numMorphs; ++i)
            {
                m_morphWeights[i] += other->m_morphWeights[i] * weight;
            }
        }
        else
        {
            const size_t numNodes = m_actor->GetSkeleton()->GetNumNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(i));
                const Transform& otherTransform = other->GetLocalSpaceTransform(i);
                transform.Add(otherTransform, weight);
            }

            // blend the morph weights
            const size_t numMorphs = m_morphWeights.size();
            MCORE_ASSERT(m_actor->GetMorphSetup(0)->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == other->GetNumMorphWeights());
            for (size_t i = 0; i < numMorphs; ++i)
            {
                m_morphWeights[i] += other->m_morphWeights[i] * weight;
            }
        }

        InvalidateAllModelSpaceTransforms();
    }


    // blend, without motion instance
    void Pose::Blend(const Pose* destPose, float weight)
    {
        if (m_actorInstance)
        {
            const size_t numNodes = m_actorInstance->GetNumEnabledNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                const uint16 nodeNr = m_actorInstance->GetEnabledNode(i);
                Transform& curTransform = const_cast<Transform&>(GetLocalSpaceTransform(nodeNr));
                curTransform.Blend(destPose->GetLocalSpaceTransform(nodeNr), weight);
            }

            // blend the morph weights
            const size_t numMorphs = m_morphWeights.size();
            MCORE_ASSERT(m_actorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (size_t i = 0; i < numMorphs; ++i)
            {
                m_morphWeights[i] = MCore::LinearInterpolate<float>(m_morphWeights[i], destPose->m_morphWeights[i], weight);
            }

            for (const auto& poseDataItem : m_poseDatas)
            {
                PoseData* poseData = poseDataItem.second.get();
                poseData->Blend(destPose, weight);
            }
        }
        else
        {
            const size_t numNodes = m_actor->GetSkeleton()->GetNumNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                Transform& curTransform = const_cast<Transform&>(GetLocalSpaceTransform(i));
                curTransform.Blend(destPose->GetLocalSpaceTransform(i), weight);
            }

            // blend the morph weights
            const size_t numMorphs = m_morphWeights.size();
            MCORE_ASSERT(m_actor->GetMorphSetup(0)->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (size_t i = 0; i < numMorphs; ++i)
            {
                m_morphWeights[i] = MCore::LinearInterpolate<float>(m_morphWeights[i], destPose->m_morphWeights[i], weight);
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
        AZ_Assert(m_localSpaceTransforms.size() == other.m_localSpaceTransforms.size(), "Poses must be of the same size");
        if (m_actorInstance)
        {
            const size_t numNodes = m_actorInstance->GetNumEnabledNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                const uint16 nodeNr = m_actorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(nodeNr));
                transform = transform.CalcRelativeTo(other.GetLocalSpaceTransform(nodeNr));
            }
        }
        else
        {
            const size_t numNodes = m_localSpaceTransforms.size();
            for (size_t i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(i));
                transform = transform.CalcRelativeTo(other.GetLocalSpaceTransform(i));
            }
        }

        const size_t numMorphs = m_morphWeights.size();
        AZ_Assert(numMorphs == other.GetNumMorphWeights(), "Number of morphs in the pose doesn't match the number of morphs inside the provided input pose.");
        for (size_t i = 0; i < numMorphs; ++i)
        {
            m_morphWeights[i] -= other.m_morphWeights[i];
        }

        InvalidateAllModelSpaceTransforms();
        return *this;
    }


    Pose& Pose::ApplyAdditive(const Pose& additivePose, float weight)
    {
        AZ_Assert(m_localSpaceTransforms.size() == additivePose.m_localSpaceTransforms.size(), "Poses must be of the same size");
        if (m_actorInstance)
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
            AZ_Assert(m_localSpaceTransforms.size() == additivePose.m_localSpaceTransforms.size(), "Poses must be of the same size");
            if (m_actorInstance)
            {
                const size_t numNodes = m_actorInstance->GetNumEnabledNodes();
                for (size_t i = 0; i < numNodes; ++i)
                {
                    uint16 nodeNr = m_actorInstance->GetEnabledNode(i);
                    Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(nodeNr));
                    const Transform& additiveTransform = additivePose.GetLocalSpaceTransform(nodeNr);
                    transform.m_position += additiveTransform.m_position * weight;
                    transform.m_rotation = transform.m_rotation.NLerp(additiveTransform.m_rotation * transform.m_rotation, weight);
                    EMFX_SCALECODE
                    (
                        transform.m_scale *= AZ::Vector3::CreateOne().Lerp(additiveTransform.m_scale, weight);
                    )
                    transform.m_rotation.Normalize();
                }
            }
            else
            {
                const size_t numNodes = m_localSpaceTransforms.size();
                for (size_t i = 0; i < numNodes; ++i)
                {
                    Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(i));
                    const Transform& additiveTransform = additivePose.GetLocalSpaceTransform(i);
                    transform.m_position += additiveTransform.m_position * weight;
                    transform.m_rotation = transform.m_rotation.NLerp(additiveTransform.m_rotation * transform.m_rotation, weight);
                    EMFX_SCALECODE
                    (
                        transform.m_scale *= AZ::Vector3::CreateOne().Lerp(additiveTransform.m_scale, weight);
                    )
                    transform.m_rotation.Normalize();
                }
            }

            const size_t numMorphs = m_morphWeights.size();
            AZ_Assert(numMorphs == additivePose.GetNumMorphWeights(), "Number of morphs in the pose doesn't match the number of morphs inside the provided input pose.");
            for (size_t i = 0; i < numMorphs; ++i)
            {
                m_morphWeights[i] += additivePose.m_morphWeights[i] * weight;
            }

            InvalidateAllModelSpaceTransforms();
            return *this;
        }
    }


    Pose& Pose::ApplyAdditive(const Pose& additivePose)
    {
        AZ_Assert(m_localSpaceTransforms.size() == additivePose.m_localSpaceTransforms.size(), "Poses must be of the same size");
        if (m_actorInstance)
        {
            const size_t numNodes = m_actorInstance->GetNumEnabledNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                uint16 nodeNr = m_actorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(nodeNr));
                const Transform& additiveTransform = additivePose.GetLocalSpaceTransform(nodeNr);
                transform.m_position += additiveTransform.m_position;
                transform.m_rotation = transform.m_rotation * additiveTransform.m_rotation;
                EMFX_SCALECODE
                (
                    transform.m_scale *= additiveTransform.m_scale;
                )
                transform.m_rotation.Normalize();
            }
        }
        else
        {
            const size_t numNodes = m_localSpaceTransforms.size();
            for (size_t i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(i));
                const Transform& additiveTransform = additivePose.GetLocalSpaceTransform(i);
                transform.m_position += additiveTransform.m_position;
                transform.m_rotation = transform.m_rotation * additiveTransform.m_rotation;
                EMFX_SCALECODE
                (
                    transform.m_scale *= additiveTransform.m_scale;
                )
                transform.m_rotation.Normalize();
            }
        }

        const size_t numMorphs = m_morphWeights.size();
        AZ_Assert(numMorphs == additivePose.GetNumMorphWeights(), "Number of morphs in the pose doesn't match the number of morphs inside the provided input pose.");
        for (size_t i = 0; i < numMorphs; ++i)
        {
            m_morphWeights[i] += additivePose.m_morphWeights[i];
        }

        InvalidateAllModelSpaceTransforms();
        return *this;
    }


    Pose& Pose::MakeAdditive(const Pose& refPose)
    {
        AZ_Assert(m_localSpaceTransforms.size() == refPose.m_localSpaceTransforms.size(), "Poses must be of the same size");
        if (m_actorInstance)
        {
            const size_t numNodes = m_actorInstance->GetNumEnabledNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                uint16 nodeNr = m_actorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(nodeNr));
                const Transform& refTransform = refPose.GetLocalSpaceTransform(nodeNr);
                transform.m_position = transform.m_position - refTransform.m_position;
                transform.m_rotation = refTransform.m_rotation.GetConjugate() * transform.m_rotation;
                EMFX_SCALECODE
                (
                    transform.m_scale *= refTransform.m_scale;
                )
            }
        }
        else
        {
            const size_t numNodes = m_localSpaceTransforms.size();
            for (size_t i = 0; i < numNodes; ++i)
            {
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(i));
                const Transform& refTransform = refPose.GetLocalSpaceTransform(i);
                transform.m_position = transform.m_position - refTransform.m_position;
                transform.m_rotation = refTransform.m_rotation.GetConjugate() * transform.m_rotation;
                EMFX_SCALECODE
                (
                    transform.m_scale *= refTransform.m_scale;
                )
            }
        }

        const size_t numMorphs = m_morphWeights.size();
        AZ_Assert(numMorphs == refPose.GetNumMorphWeights(), "Number of morphs in the pose doesn't match the number of morphs inside the provided input pose.");
        for (size_t i = 0; i < numMorphs; ++i)
        {
            m_morphWeights[i] -= refPose.m_morphWeights[i];
        }

        InvalidateAllModelSpaceTransforms();
        return *this;
    }


    // additive blend
    void Pose::BlendAdditiveUsingBindPose(const Pose* destPose, float weight)
    {
        if (m_actorInstance)
        {
            const TransformData* transformData = m_actorInstance->GetTransformData();
            Pose* bindPose = transformData->GetBindPose();
            Transform result;

            const size_t numNodes = m_actorInstance->GetNumEnabledNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                const uint16 nodeNr = m_actorInstance->GetEnabledNode(i);
                BlendTransformAdditiveUsingBindPose(bindPose->GetLocalSpaceTransform(nodeNr), GetLocalSpaceTransform(nodeNr), destPose->GetLocalSpaceTransform(nodeNr), weight, &result);
                SetLocalSpaceTransform(nodeNr, result, false);
            }

            // blend the morph weights
            const size_t numMorphs = m_morphWeights.size();
            MCORE_ASSERT(m_actorInstance->GetMorphSetupInstance()->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (size_t i = 0; i < numMorphs; ++i)
            {
                m_morphWeights[i] += destPose->m_morphWeights[i] * weight;
            }
        }
        else
        {
            const TransformData* transformData = m_actorInstance->GetTransformData();
            Pose* bindPose = transformData->GetBindPose();
            Transform result;

            const size_t numNodes = m_actor->GetSkeleton()->GetNumNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                BlendTransformAdditiveUsingBindPose(bindPose->GetLocalSpaceTransform(i), GetLocalSpaceTransform(i), destPose->GetLocalSpaceTransform(i), weight, &result);
                SetLocalSpaceTransform(i, result, false);
            }

            // blend the morph weights
            const size_t numMorphs = m_morphWeights.size();
            MCORE_ASSERT(m_actor->GetMorphSetup(0)->GetNumMorphTargets() == numMorphs);
            MCORE_ASSERT(numMorphs == destPose->GetNumMorphWeights());
            for (size_t i = 0; i < numMorphs; ++i)
            {
                m_morphWeights[i] += destPose->m_morphWeights[i] * weight;
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
        const size_t motionExtractionNodeIndex = m_actor->GetMotionExtractionNodeIndex();
        if (motionExtractionNodeIndex != InvalidIndex)
        {
            Transform motionExtractionNodeTransform = GetLocalSpaceTransformDirect(motionExtractionNodeIndex);
            m_actorInstance->MotionExtractionCompensate(motionExtractionNodeTransform, motionExtractionFlags);
            SetLocalSpaceTransformDirect(motionExtractionNodeIndex, motionExtractionNodeTransform);
        }
    }


    // compensate for motion extraction, basically making it in-place
    void Pose::CompensateForMotionExtraction(EMotionExtractionFlags motionExtractionFlags)
    {
        const size_t motionExtractionNodeIndex = m_actor->GetMotionExtractionNodeIndex();
        if (motionExtractionNodeIndex != InvalidIndex)
        {
            Transform motionExtractionNodeTransform = GetLocalSpaceTransform(motionExtractionNodeIndex);
            m_actorInstance->MotionExtractionCompensate(motionExtractionNodeTransform, motionExtractionFlags);
            SetLocalSpaceTransform(motionExtractionNodeIndex, motionExtractionNodeTransform);
        }
    }


    // apply the morph target weights to the morph setup instance of the given actor instance
    void Pose::ApplyMorphWeightsToActorInstance()
    {
        MorphSetupInstance* morphSetupInstance = m_actorInstance->GetMorphSetupInstance();
        const size_t numMorphs = morphSetupInstance->GetNumMorphTargets();
        for (size_t m = 0; m < numMorphs; ++m)
        {
            MorphSetupInstance::MorphTarget* morphTarget = morphSetupInstance->GetMorphTarget(m);
            if (morphTarget->GetIsInManualMode() == false)
            {
                morphTarget->SetWeight(m_morphWeights[m]);
            }
        }
    }


    // zero all morph weights
    void Pose::ZeroMorphWeights()
    {
        AZStd::fill(begin(m_morphWeights), end(m_morphWeights), 0.0f);
    }


    void Pose::ResizeNumMorphs(size_t numMorphTargets)
    {
        m_morphWeights.resize(numMorphTargets);
    }


    Pose& Pose::PreMultiply(const Pose& other)
    {
        AZ_Assert(m_localSpaceTransforms.size() == other.m_localSpaceTransforms.size(), "Poses must be of the same size");
        if (m_actorInstance)
        {
            const size_t numNodes = m_actorInstance->GetNumEnabledNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                const uint16 nodeNr = m_actorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(nodeNr));
                Transform otherTransform = other.GetLocalSpaceTransform(nodeNr);
                transform = otherTransform * transform;
            }
        }
        else
        {
            const size_t numNodes = m_localSpaceTransforms.size();
            for (size_t i = 0; i < numNodes; ++i)
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
        AZ_Assert(m_localSpaceTransforms.size() == other.m_localSpaceTransforms.size(), "Poses must be of the same size");
        if (m_actorInstance)
        {
            const size_t numNodes = m_actorInstance->GetNumEnabledNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                uint16 nodeNr = m_actorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(nodeNr));
                transform.Multiply(other.GetLocalSpaceTransform(nodeNr));
            }
        }
        else
        {
            const size_t numNodes = m_localSpaceTransforms.size();
            for (size_t i = 0; i < numNodes; ++i)
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
        AZ_Assert(m_localSpaceTransforms.size() == other.m_localSpaceTransforms.size(), "Poses must be of the same size");
        if (m_actorInstance)
        {
            const size_t numNodes = m_actorInstance->GetNumEnabledNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                uint16 nodeNr = m_actorInstance->GetEnabledNode(i);
                Transform& transform = const_cast<Transform&>(GetLocalSpaceTransform(nodeNr));
                Transform otherTransform = other.GetLocalSpaceTransform(nodeNr);
                otherTransform.Inverse();
                transform.PreMultiply(otherTransform);
            }
        }
        else
        {
            const size_t numNodes = m_localSpaceTransforms.size();
            for (size_t i = 0; i < numNodes; ++i)
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


    Transform Pose::GetMeshNodeWorldSpaceTransform(size_t lodLevel, size_t nodeIndex) const
    {
        if (!m_actorInstance)
        {
            return Transform::CreateIdentity();
        }

        Actor* actor = m_actorInstance->GetActor();
        if (actor->CheckIfHasSkinningDeformer(lodLevel, nodeIndex))
        {
            return m_actorInstance->GetWorldSpaceTransform();
        }

        return GetWorldSpaceTransform(nodeIndex);
    }


    void Pose::Mirror(const MotionLinkData* motionLinkData)
    {
        AZ_Assert(motionLinkData, "Expecting valid motionLinkData pointer.");
        AZ_Assert(m_actorInstance, "Mirroring is only possible in combination with an actor instance.");

        const Actor* actor = m_actorInstance->GetActor();
        const TransformData* transformData = m_actorInstance->GetTransformData();
        const Pose* bindPose = transformData->GetBindPose();
        const AZStd::vector<size_t>& jointLinks = motionLinkData->GetJointDataLinks();

        AnimGraphPose* tempPose = GetEMotionFX().GetThreadData(m_actorInstance->GetThreadIndex())->GetPosePool().RequestPose(m_actorInstance);
        Pose& unmirroredPose = tempPose->GetPose();
        unmirroredPose = *this;

        const size_t numNodes = m_actorInstance->GetNumEnabledNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            const size_t nodeNumber = m_actorInstance->GetEnabledNode(i);
            const size_t jointDataIndex = jointLinks[nodeNumber];
            if (jointDataIndex == InvalidIndex)
            {
                continue;
            }

            const Actor::NodeMirrorInfo& mirrorInfo = actor->GetNodeMirrorInfo(nodeNumber);

            Transform mirrored = bindPose->GetLocalSpaceTransform(nodeNumber);
            AZ::Vector3 mirrorAxis = AZ::Vector3::CreateZero();
            mirrorAxis.SetElement(mirrorInfo.m_axis, 1.0f);
            mirrored.ApplyDeltaMirrored(bindPose->GetLocalSpaceTransform(mirrorInfo.m_sourceNode), unmirroredPose.GetLocalSpaceTransform(mirrorInfo.m_sourceNode), mirrorAxis, mirrorInfo.m_flags);

            SetLocalSpaceTransformDirect(nodeNumber, mirrored);
        }

        GetEMotionFX().GetThreadData(m_actorInstance->GetThreadIndex())->GetPosePool().FreePose(tempPose);
    }
}   // namespace EMotionFX
