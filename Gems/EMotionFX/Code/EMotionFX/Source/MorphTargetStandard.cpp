/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "EMotionFXConfig.h"
#include "Node.h"
#include "Actor.h"
#include "MorphTargetStandard.h"
#include "Mesh.h"
#include "ActorInstance.h"
#include "MorphMeshDeformer.h"
#include "MeshDeformerStack.h"
#include "TransformData.h"
#include <MCore/Source/Compare.h>
#include <MCore/Source/Vector.h>
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MorphTargetStandard, DeformerAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(MorphTargetStandard::DeformData, DeformerAllocator, 0)

    MorphTargetStandard::MorphTargetStandard(const char* name)
        : MorphTarget(name)
    {
    }

    MorphTargetStandard::MorphTargetStandard(bool captureTransforms, Actor* neutralPose, Actor* targetPose, const char* name)
        : MorphTarget(name)
    {
        InitFromPose(captureTransforms, neutralPose, targetPose);
    }

    MorphTargetStandard::~MorphTargetStandard()
    {
        RemoveAllDeformDatas();
    }

    MorphTargetStandard* MorphTargetStandard::Create(const char* name)
    {
        return aznew MorphTargetStandard(name);
    }

    MorphTargetStandard* MorphTargetStandard::Create(bool captureTransforms, Actor* neutralPose, Actor* targetPose, const char* name)
    {
        return aznew MorphTargetStandard(captureTransforms, neutralPose, targetPose, name);
    }

    uint32 MorphTargetStandard::GetType() const
    {
        return TYPE_ID;
    }

    // initialize the morph target from a given actor pose
    void MorphTargetStandard::InitFromPose(bool captureTransforms, Actor* neutralPose, Actor* targetPose)
    {
        MCORE_ASSERT(neutralPose);
        MCORE_ASSERT(targetPose);

        if (captureTransforms)
        {
            // get a bone list, if we also captured meshes, because in that case we want
            // to disable capturing transforms for the bones since the deforms already have been captured
            // by the mesh capture
            //Array<Node*> boneList;
            //if (mCaptureMeshDeforms)
            //pose->ExtractBoneList(0, &boneList);

            Skeleton* targetSkeleton    = targetPose->GetSkeleton();
            Skeleton* neutralSkeleton   = neutralPose->GetSkeleton();

            const Pose& neutralBindPose = *neutralPose->GetBindPose();
            const Pose& targetBindPose  = *targetPose->GetBindPose();

            //      Transform*  neutralData = neutralPose->GetBindPoseLocalTransforms();
            //      Transform*  targetData  = targetPose->GetBindPoseLocalTransforms();

            // check for transformation changes
            const uint32 numPoseNodes = targetSkeleton->GetNumNodes();
            for (uint32 i = 0; i < numPoseNodes; ++i)
            {
                // get a node id (both nodes will have the same id since they represent their names)
                const uint32 nodeID = targetSkeleton->GetNode(i)->GetID();

                // try to find the node with the same name inside the neutral pose actor
                Node* neutralNode = neutralSkeleton->FindNodeByID(nodeID);
                if (neutralNode == nullptr)
                {
                    continue;
                }

                // get the node indices of both nodes
                const uint32 neutralNodeIndex = neutralNode->GetNodeIndex();
                const uint32 targetNodeIndex  = targetSkeleton->GetNode(i)->GetNodeIndex();

                // skip bones in the bone list
                //if (mCaptureMeshDeforms)
                //if (boneList.Contains( nodeA ))
                //continue;

                const Transform& neutralTransform   = neutralBindPose.GetLocalSpaceTransform(neutralNodeIndex);
                const Transform& targetTransform    = targetBindPose.GetLocalSpaceTransform(targetNodeIndex);

                AZ::Vector3         neutralPos      = neutralTransform.mPosition;
                AZ::Vector3         targetPos       = targetTransform.mPosition;
                AZ::Quaternion      neutralRot      = neutralTransform.mRotation;
                AZ::Quaternion      targetRot       = targetTransform.mRotation;

                EMFX_SCALECODE
                (
                    AZ::Vector3      neutralScale    = neutralTransform.mScale;
                    AZ::Vector3      targetScale     = targetTransform.mScale;
                    //AZ::Quaternion neutralScaleRot = neutralTransform.mScaleRotation;
                    //AZ::Quaternion targetScaleRot  = targetTransform.mScaleRotation;
                )

                // check if the position changed
                bool changed = (MCore::Compare<AZ::Vector3>::CheckIfIsClose(neutralPos, targetPos, MCore::Math::epsilon) == false);

                // check if the rotation changed
                if (changed == false)
                {
                    changed = (MCore::Compare<AZ::Quaternion>::CheckIfIsClose(neutralRot, targetRot, MCore::Math::epsilon) == false);
                }

                EMFX_SCALECODE
                (
                    // check if the scale changed
                    if (changed == false)
                    {
                        changed = (MCore::Compare<AZ::Vector3>::CheckIfIsClose(neutralScale, targetScale, MCore::Math::epsilon) == false);
                    }

                    // check if the scale rotation changed
                    //              if (changed == false)
                    //              changed = (MCore::Compare<AZ::Quaternion>::CheckIfIsClose(neutralScaleRot, targetScaleRot, MCore::Math::epsilon) == false);
                )

                // if this node changed transformation
                if (changed)
                {
                    // create a transform object form the node in the pose
                    Transformation transform;
                    transform.mPosition     = targetPos - neutralPos;
                    transform.mRotation     = targetRot;

                    EMFX_SCALECODE
                    (
                        //transform.mScaleRotation= targetScaleRot;
                        transform.mScale    = targetScale - neutralScale;
                    )

                    transform.mNodeIndex    = neutralNodeIndex;

                    // add the new transform
                    AddTransformation(transform);
                }
            }

            //LogInfo("Num transforms = %d", mTransforms.GetLength());
        }
    }


    // apply the relative transformation to the specified node
    // store the result in the position, rotation and scale parameters
    void MorphTargetStandard::ApplyTransformation(ActorInstance* actorInstance, uint32 nodeIndex, AZ::Vector3& position, AZ::Quaternion& rotation, AZ::Vector3& scale, float weight)
    {
        // calculate the normalized weight (in range of 0..1)
        const float newWeight = MCore::Clamp<float>(weight, mRangeMin, mRangeMax); // make sure its within the range
        const float normalizedWeight = CalcNormalizedWeight(newWeight); // convert in range of 0..1

        // calculate the new transformations for all nodes of this morph target
        const uint32 numTransforms = mTransforms.GetLength();
        for (uint32 i = 0; i < numTransforms; ++i)
        {
            // if this is the node that gets modified by this transform
            if (mTransforms[i].mNodeIndex != nodeIndex)
            {
                continue;
            }

            position += mTransforms[i].mPosition * newWeight;
            scale += mTransforms[i].mScale * newWeight;

            // rotate additively
            const AZ::Quaternion& orgRot = actorInstance->GetTransformData()->GetBindPose()->GetLocalSpaceTransform(nodeIndex).mRotation;
            const AZ::Quaternion rot = orgRot.NLerp(mTransforms[i].mRotation, normalizedWeight);
            rotation = rotation * (orgRot.GetInverseFull() * rot);
            rotation.Normalize();

            // all remaining nodes in the transform won't modify this current node
            break;
        }
    }


    // check if this morph target influences the specified node or not
    bool MorphTargetStandard::Influences(uint32 nodeIndex) const
    {
        // check if there is a deform data object, which works on the specified node
        for (const DeformData* deformData : mDeformDatas)
        {
            if (deformData->mNodeIndex == nodeIndex)
            {
                return true;
            }
        }

        // check all transforms
        const uint32 numTransforms = mTransforms.GetLength();
        for (uint32 i = 0; i < numTransforms; ++i)
        {
            if (mTransforms[i].mNodeIndex == nodeIndex)
            {
                return true;
            }
        }

        // this morph target doesn't influence the given node
        return false;
    }


    // apply the deformations to a given actor
    void MorphTargetStandard::Apply(ActorInstance* actorInstance, float weight)
    {
        // calculate the normalized weight (in range of 0..1)
        const float newWeight = MCore::Clamp<float>(weight, mRangeMin, mRangeMax); // make sure its within the range
        const float normalizedWeight = CalcNormalizedWeight(newWeight); // convert in range of 0..1

        TransformData* transformData = actorInstance->GetTransformData();
        Transform newTransform;

        // calculate the new transformations for all nodes of this morph target
        const uint32 numTransforms = mTransforms.GetLength();
        for (uint32 i = 0; i < numTransforms; ++i)
        {
            // try to find the node
            const uint32 nodeIndex = mTransforms[i].mNodeIndex;

            // init the transform data
            newTransform = transformData->GetCurrentPose()->GetLocalSpaceTransform(nodeIndex);

            // calc new position and scale (delta based targetTransform)
            newTransform.mPosition += mTransforms[i].mPosition * newWeight;

            EMFX_SCALECODE
            (
                newTransform.mScale += mTransforms[i].mScale * newWeight;
                // newTransform.mScaleRotation.Identity();
            )

            // rotate additively
            const AZ::Quaternion& orgRot = transformData->GetBindPose()->GetLocalSpaceTransform(nodeIndex).mRotation;
            const AZ::Quaternion rot = orgRot.NLerp(mTransforms[i].mRotation, normalizedWeight);
            newTransform.mRotation = newTransform.mRotation * (orgRot.GetInverseFull() * rot);
            newTransform.mRotation.Normalize();
            /*
                    // scale rotate additively
                    orgRot = actorInstance->GetTransformData()->GetOrgScaleRot( nodeIndex );
                    a = orgRot;
                    b = mTransforms[i].mScaleRotation;
                    rot = a.Lerp(b, normalizedWeight);
                    rot.Normalize();
                    newTransform.mScaleRotation = newTransform.mScaleRotation * (orgRot.Inversed() * rot);
                    newTransform.mScaleRotation.Normalize();
            */
            // set the new transformation
            transformData->GetCurrentPose()->SetLocalSpaceTransform(nodeIndex, newTransform);
        }
    }

    uint32 MorphTargetStandard::GetNumDeformDatas() const
    {
        return static_cast<uint32>(mDeformDatas.size());
    }

    MorphTargetStandard::DeformData* MorphTargetStandard::GetDeformData(uint32 nr) const
    {
        return mDeformDatas[nr];
    }

    void MorphTargetStandard::AddDeformData(DeformData* data)
    {
        mDeformDatas.emplace_back(data);
    }

    void MorphTargetStandard::AddTransformation(const Transformation& transform)
    {
        mTransforms.Add(transform);
    }

    uint32 MorphTargetStandard::GetNumTransformations() const
    {
        return mTransforms.GetLength();
    }

    MorphTargetStandard::Transformation& MorphTargetStandard::GetTransformation(uint32 nr)
    {
        return mTransforms[nr];
    }


    // clone this morph target
    MorphTarget* MorphTargetStandard::Clone()
    {
        // create the clone and copy its base class values
        MorphTargetStandard* clone = aznew MorphTargetStandard(""); // use an empty dummy name, as we will copy over the ID generated from it anyway
        CopyBaseClassMemberValues(clone);

        // now copy over the standard morph target related values
        // first start with the transforms
        clone->mTransforms = mTransforms;

        // now clone the deform datas
        clone->mDeformDatas.resize(mDeformDatas.size());
        for (size_t i = 0; i < mDeformDatas.size(); ++i)
        {
            clone->mDeformDatas[i] = mDeformDatas[i]->Clone();
        }

        // return the clone
        return clone;
    }



    //---------------------------------------------------
    // DeformData
    //---------------------------------------------------

    // constructor
    MorphTargetStandard::DeformData::DeformData(uint32 nodeIndex, uint32 numVerts)
    {
        mNodeIndex  = nodeIndex;
        mNumVerts   = numVerts;
        mDeltas     = (VertexDelta*)MCore::Allocate(numVerts * sizeof(VertexDelta), EMFX_MEMCATEGORY_GEOMETRY_PMORPHTARGETS, MorphTargetStandard::MEMORYBLOCK_ID);
        mMinValue   = -10.0f;
        mMaxValue   = +10.0f;
    }


    // destructor
    MorphTargetStandard::DeformData::~DeformData()
    {
        MCore::Free(mDeltas);
    }


    // create
    MorphTargetStandard::DeformData* MorphTargetStandard::DeformData::Create(uint32 nodeIndex, uint32 numVerts)
    {
        return aznew MorphTargetStandard::DeformData(nodeIndex, numVerts);
    }


    // clone a morph target
    MorphTargetStandard::DeformData* MorphTargetStandard::DeformData::Clone()
    {
        MorphTargetStandard::DeformData* clone = aznew MorphTargetStandard::DeformData(mNodeIndex, mNumVerts);

        // copy the data
        clone->mMinValue = mMinValue;
        clone->mMaxValue = mMaxValue;
        MCore::MemCopy((uint8*)clone->mDeltas, (uint8*)mDeltas, mNumVerts * sizeof(VertexDelta));

        return clone;
    }

    void MorphTargetStandard::RemoveAllDeformDatas()
    {
        for (DeformData* deformData : mDeformDatas)
        {
            deformData->Destroy();
        }

        mDeformDatas.clear();
    }

    void MorphTargetStandard::RemoveAllDeformDatasFor(Node* joint)
    {
        mDeformDatas.erase(
            AZStd::remove_if(mDeformDatas.begin(), mDeformDatas.end(),
                [=](const DeformData* deformData)
                {
                    return deformData->mNodeIndex == joint->GetNodeIndex();
                }),
            mDeformDatas.end());
    }

    // pre-alloc memory for the deform datas
    void MorphTargetStandard::ReserveDeformDatas(uint32 numDeformDatas)
    {
        mDeformDatas.reserve(numDeformDatas);
    }

    // pre-allocate memory for the transformations
    void MorphTargetStandard::ReserveTransformations(uint32 numTransforms)
    {
        mTransforms.Reserve(numTransforms);
    }

    void MorphTargetStandard::RemoveDeformData(uint32 index, bool delFromMem)
    {
        if (delFromMem)
        {
            delete mDeformDatas[index];
        }
        mDeformDatas.erase(mDeformDatas.begin() + index);
    }


    void MorphTargetStandard::RemoveTransformation(uint32 index)
    {
        mTransforms.Remove(index);
    }


    // scale the morph data
    void MorphTargetStandard::Scale(float scaleFactor)
    {
        // if we don't need to adjust the scale, do nothing
        if (MCore::Math::IsFloatEqual(scaleFactor, 1.0f))
        {
            return;
        }

        // scale the transformations
        const uint32 numTransformations = mTransforms.GetLength();
        for (uint32 i = 0; i < numTransformations; ++i)
        {
            Transformation& transform = mTransforms[i];
            transform.mPosition *= scaleFactor;
        }

        // scale the deform datas (packed per vertex morph deltas)
        for (DeformData* deformData : mDeformDatas)
        {
            DeformData::VertexDelta* deltas = deformData->mDeltas;

            float newMinValue = deformData->mMinValue * scaleFactor;
            float newMaxValue = deformData->mMaxValue * scaleFactor;

            // make sure the values won't be too small
            if (newMaxValue - newMinValue < 1.0f)
            {
                if (newMinValue < 0.0f && newMinValue > -1.0f)
                {
                    newMinValue = -1.0f;
                }

                if (newMaxValue > 0.0f && newMaxValue < 1.0f)
                {
                    newMaxValue = 1.0f;
                }
            }


            // iterate over the deltas (per vertex values)
            const uint32 numVerts = deformData->mNumVerts;
            for (uint32 v = 0; v < numVerts; ++v)
            {
                // decompress
                AZ::Vector3 decompressed = deltas[v].mPosition.ToVector3(deformData->mMinValue, deformData->mMaxValue);

                // scale
                decompressed *= scaleFactor;

                // compress again
                deltas[v].mPosition.FromVector3(decompressed, newMinValue, newMaxValue);
            }

            deformData->mMinValue = newMinValue;
            deformData->mMaxValue = newMaxValue;
        }
    }
} // namespace EMotionFX
