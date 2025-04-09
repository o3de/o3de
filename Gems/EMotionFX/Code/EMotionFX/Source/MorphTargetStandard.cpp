/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    AZ_CLASS_ALLOCATOR_IMPL(MorphTargetStandard, DeformerAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(MorphTargetStandard::DeformData, DeformerAllocator)

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
            Skeleton* targetSkeleton    = targetPose->GetSkeleton();
            Skeleton* neutralSkeleton   = neutralPose->GetSkeleton();

            const Pose& neutralBindPose = *neutralPose->GetBindPose();
            const Pose& targetBindPose  = *targetPose->GetBindPose();

            // check for transformation changes
            const size_t numPoseNodes = targetSkeleton->GetNumNodes();
            for (size_t i = 0; i < numPoseNodes; ++i)
            {
                // get a node id (both nodes will have the same id since they represent their names)
                const size_t nodeID = targetSkeleton->GetNode(i)->GetID();

                // try to find the node with the same name inside the neutral pose actor
                Node* neutralNode = neutralSkeleton->FindNodeByID(nodeID);
                if (neutralNode == nullptr)
                {
                    continue;
                }

                // get the node indices of both nodes
                const size_t neutralNodeIndex = neutralNode->GetNodeIndex();
                const size_t targetNodeIndex  = targetSkeleton->GetNode(i)->GetNodeIndex();

                const Transform& neutralTransform   = neutralBindPose.GetLocalSpaceTransform(neutralNodeIndex);
                const Transform& targetTransform    = targetBindPose.GetLocalSpaceTransform(targetNodeIndex);

                AZ::Vector3         neutralPos      = neutralTransform.m_position;
                AZ::Vector3         targetPos       = targetTransform.m_position;
                AZ::Quaternion      neutralRot      = neutralTransform.m_rotation;
                AZ::Quaternion      targetRot       = targetTransform.m_rotation;

                EMFX_SCALECODE
                (
                    AZ::Vector3      neutralScale    = neutralTransform.m_scale;
                    AZ::Vector3      targetScale     = targetTransform.m_scale;
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

                )

                // if this node changed transformation
                if (changed)
                {
                    // create a transform object form the node in the pose
                    Transformation transform;
                    transform.m_position     = targetPos - neutralPos;
                    transform.m_rotation     = targetRot;

                    EMFX_SCALECODE
                    (
                        transform.m_scale    = targetScale - neutralScale;
                    )

                    transform.m_nodeIndex    = neutralNodeIndex;

                    // add the new transform
                    AddTransformation(transform);
                }
            }
        }
    }


    // apply the relative transformation to the specified node
    // store the result in the position, rotation and scale parameters
    void MorphTargetStandard::ApplyTransformation(ActorInstance* actorInstance, size_t nodeIndex, AZ::Vector3& position, AZ::Quaternion& rotation, AZ::Vector3& scale, float weight)
    {
        // calculate the normalized weight (in range of 0..1)
        const float newWeight = MCore::Clamp<float>(weight, m_rangeMin, m_rangeMax); // make sure its within the range
        const float normalizedWeight = CalcNormalizedWeight(newWeight); // convert in range of 0..1

        // calculate the new transformations for all nodes of this morph target
        for (const Transformation& transform : m_transforms)
        {
            // if this is the node that gets modified by this transform
            if (transform.m_nodeIndex != nodeIndex)
            {
                continue;
            }

            position += transform.m_position * newWeight;
            scale += transform.m_scale * newWeight;

            // rotate additively
            const AZ::Quaternion& orgRot = actorInstance->GetTransformData()->GetBindPose()->GetLocalSpaceTransform(nodeIndex).m_rotation;
            const AZ::Quaternion rot = orgRot.NLerp(transform.m_rotation, normalizedWeight);
            rotation = rotation * (orgRot.GetInverseFull() * rot);
            rotation.Normalize();

            // all remaining nodes in the transform won't modify this current node
            break;
        }
    }


    // check if this morph target influences the specified node or not
    bool MorphTargetStandard::Influences(size_t nodeIndex) const
    {
        return
            AZStd::any_of(begin(m_deformDatas), end(m_deformDatas), [nodeIndex](const DeformData* deformData)
            {
                return deformData->m_nodeIndex == nodeIndex;
            })
            ||
            AZStd::any_of(begin(m_transforms), end(m_transforms), [nodeIndex](const Transformation& transform)
            {
                return transform.m_nodeIndex == nodeIndex;
            });
    }


    // apply the deformations to a given actor
    void MorphTargetStandard::Apply(ActorInstance* actorInstance, float weight)
    {
        // calculate the normalized weight (in range of 0..1)
        const float newWeight = MCore::Clamp<float>(weight, m_rangeMin, m_rangeMax); // make sure its within the range
        const float normalizedWeight = CalcNormalizedWeight(newWeight); // convert in range of 0..1

        TransformData* transformData = actorInstance->GetTransformData();
        Transform newTransform;

        // calculate the new transformations for all nodes of this morph target
        for (const Transformation& transform : m_transforms)
        {
            // try to find the node
            const size_t nodeIndex = transform.m_nodeIndex;

            // init the transform data
            newTransform = transformData->GetCurrentPose()->GetLocalSpaceTransform(nodeIndex);

            // calc new position and scale (delta based targetTransform)
            newTransform.m_position += transform.m_position * newWeight;

            EMFX_SCALECODE
            (
                newTransform.m_scale += transform.m_scale * newWeight;
            )

            // rotate additively
            const AZ::Quaternion& orgRot = transformData->GetBindPose()->GetLocalSpaceTransform(nodeIndex).m_rotation;
            const AZ::Quaternion rot = orgRot.NLerp(transform.m_rotation, normalizedWeight);
            newTransform.m_rotation = newTransform.m_rotation * (orgRot.GetInverseFull() * rot);
            newTransform.m_rotation.Normalize();
            // set the new transformation
            transformData->GetCurrentPose()->SetLocalSpaceTransform(nodeIndex, newTransform);
        }
    }

    size_t MorphTargetStandard::GetNumDeformDatas() const
    {
        return m_deformDatas.size();
    }

    MorphTargetStandard::DeformData* MorphTargetStandard::GetDeformData(size_t nr) const
    {
        return m_deformDatas[nr];
    }

    void MorphTargetStandard::AddDeformData(DeformData* data)
    {
        m_deformDatas.emplace_back(data);
    }

    void MorphTargetStandard::AddTransformation(const Transformation& transform)
    {
        m_transforms.emplace_back(transform);
    }

    // get the number of transformations in this morph target
    size_t MorphTargetStandard::GetNumTransformations() const
    {
        return m_transforms.size();
    }

    MorphTargetStandard::Transformation& MorphTargetStandard::GetTransformation(size_t nr)
    {
        return m_transforms[nr];
    }


    // clone this morph target
    MorphTarget* MorphTargetStandard::Clone() const
    {
        // create the clone and copy its base class values
        MorphTargetStandard* clone = aznew MorphTargetStandard(""); // use an empty dummy name, as we will copy over the ID generated from it anyway
        CopyBaseClassMemberValues(clone);

        // now copy over the standard morph target related values
        // first start with the transforms
        clone->m_transforms = m_transforms;

        // now clone the deform datas
        clone->m_deformDatas.resize(m_deformDatas.size());
        for (uint32 i = 0; i < m_deformDatas.size(); ++i)
        {
            clone->m_deformDatas[i] = m_deformDatas[i]->Clone();
        }

        // return the clone
        return clone;
    }



    //---------------------------------------------------
    // DeformData
    //---------------------------------------------------

    // constructor
    MorphTargetStandard::DeformData::DeformData(size_t nodeIndex, uint32 numVerts)
    {
        m_nodeIndex  = nodeIndex;
        m_numVerts   = numVerts;
        m_deltas     = (VertexDelta*)MCore::Allocate(numVerts * sizeof(VertexDelta), EMFX_MEMCATEGORY_GEOMETRY_PMORPHTARGETS, MorphTargetStandard::MEMORYBLOCK_ID);
        m_minValue   = -10.0f;
        m_maxValue   = +10.0f;
    }


    // destructor
    MorphTargetStandard::DeformData::~DeformData()
    {
        MCore::Free(m_deltas);
    }


    // create
    MorphTargetStandard::DeformData* MorphTargetStandard::DeformData::Create(size_t nodeIndex, uint32 numVerts)
    {
        return aznew MorphTargetStandard::DeformData(nodeIndex, numVerts);
    }


    // clone a morph target
    MorphTargetStandard::DeformData* MorphTargetStandard::DeformData::Clone()
    {
        MorphTargetStandard::DeformData* clone = aznew MorphTargetStandard::DeformData(m_nodeIndex, m_numVerts);

        // copy the data
        clone->m_minValue = m_minValue;
        clone->m_maxValue = m_maxValue;
        MCore::MemCopy((uint8*)clone->m_deltas, (uint8*)m_deltas, m_numVerts * sizeof(VertexDelta));

        return clone;
    }

    void MorphTargetStandard::RemoveAllDeformDatas()
    {
        for (DeformData* deformData : m_deformDatas)
        {
            deformData->Destroy();
        }

        m_deformDatas.clear();
    }

    void MorphTargetStandard::RemoveAllDeformDatasFor(Node* joint)
    {
        m_deformDatas.erase(
            AZStd::remove_if(m_deformDatas.begin(), m_deformDatas.end(),
                [=](const DeformData* deformData)
                {
                    return deformData->m_nodeIndex == joint->GetNodeIndex();
                }),
            m_deformDatas.end());
    }

    // pre-alloc memory for the deform datas
    void MorphTargetStandard::ReserveDeformDatas(size_t numDeformDatas)
    {
        m_deformDatas.reserve(numDeformDatas);
    }

    // pre-allocate memory for the transformations
    void MorphTargetStandard::ReserveTransformations(size_t numTransforms)
    {
        m_transforms.reserve(numTransforms);
    }

    void MorphTargetStandard::RemoveDeformData(size_t index, bool delFromMem)
    {
        if (delFromMem)
        {
            delete m_deformDatas[index];
        }
        m_deformDatas.erase(m_deformDatas.begin() + index);
    }


    void MorphTargetStandard::RemoveTransformation(size_t index)
    {
        m_transforms.erase(AZStd::next(begin(m_transforms), index));
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
        for (Transformation& transform : m_transforms)
        {
             transform.m_position *= scaleFactor;
        }

        // scale the deform datas (packed per vertex morph deltas)
        for (DeformData* deformData : m_deformDatas)
        {
            DeformData::VertexDelta* deltas = deformData->m_deltas;

            float newMinValue = deformData->m_minValue * scaleFactor;
            float newMaxValue = deformData->m_maxValue * scaleFactor;

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
            const uint32 numVerts = deformData->m_numVerts;
            for (uint32 v = 0; v < numVerts; ++v)
            {
                // decompress
                AZ::Vector3 decompressed = deltas[v].m_position.ToVector3(deformData->m_minValue, deformData->m_maxValue);

                // scale
                decompressed *= scaleFactor;

                // compress again
                deltas[v].m_position.FromVector3(decompressed, newMinValue, newMaxValue);
            }

            deformData->m_minValue = newMinValue;
            deformData->m_maxValue = newMaxValue;
        }
    }
} // namespace EMotionFX
