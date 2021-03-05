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


    // basic constructor
    MorphTargetStandard::MorphTargetStandard(const char* name)
        : MorphTarget(name)
    {
        mDeformDatas.SetMemoryCategory(EMFX_MEMCATEGORY_GEOMETRY_PMORPHTARGETS);
    }


    // extended constructor
    MorphTargetStandard::MorphTargetStandard(bool captureTransforms, bool captureMeshDeforms, Actor* neutralPose, Actor* targetPose, const char* name)
        : MorphTarget(name)
    {
        InitFromPose(captureTransforms, captureMeshDeforms, neutralPose, targetPose);
    }


    // destructor
    MorphTargetStandard::~MorphTargetStandard()
    {
        RemoveAllDeformDatas();
    }


    // create
    MorphTargetStandard* MorphTargetStandard::Create(const char* name)
    {
        return aznew MorphTargetStandard(name);
    }


    // extended create
    MorphTargetStandard* MorphTargetStandard::Create(bool captureTransforms, bool captureMeshDeforms, Actor* neutralPose, Actor* targetPose, const char* name)
    {
        return aznew MorphTargetStandard(captureTransforms, captureMeshDeforms, neutralPose, targetPose, name);
    }


    // get the expressionpart type
    uint32 MorphTargetStandard::GetType() const
    {
        return TYPE_ID;
    }


    void MorphTargetStandard::BuildWorkMesh(Mesh* mesh, MCore::Array< MCore::Array<uint32> >& output)
    {
        // resize the array
        const uint32 numOrgVerts = mesh->GetNumOrgVertices();
        output.Resize(numOrgVerts);

        // get the original vertex numbers
        const uint32* orgVerts = (uint32*)mesh->FindOriginalVertexData(Mesh::ATTRIB_ORGVTXNUMBERS);

        // iterate over all vertices and add them to the worker mesh
        const uint32 numVerts = mesh->GetNumVertices();
        for (uint32 v = 0; v < numVerts; ++v)
        {
            const uint32 orgVertex = orgVerts[v];
            output[orgVertex].Add(v);
        }
    }


    // initialize the morph target from a given actor pose
    void MorphTargetStandard::InitFromPose(bool captureTransforms, bool captureMeshDeforms, Actor* neutralPose, Actor* targetPose)
    {
        MCORE_ASSERT(neutralPose);
        MCORE_ASSERT(targetPose);

        // filter all changed meshes
        if (captureMeshDeforms)
        {
            Skeleton* targetSkeleton    = targetPose->GetSkeleton();
            Skeleton* neutralSkeleton   = neutralPose->GetSkeleton();
            const uint32 numPoseNodes = targetSkeleton->GetNumNodes();
            for (uint32 i = 0; i < numPoseNodes; ++i)
            {
                // find if the pose node also exists in the actor where we apply this to
                Node* targetNode  = targetSkeleton->GetNode(i);
                Node* neutralNode = neutralSkeleton->FindNodeByID(targetNode->GetID());

                // skip this node if it doesn't exist in the actor we apply this to
                if (!neutralNode)
                {
                    continue;
                }

                // get the meshes
                Mesh* neutralMesh = neutralPose->GetMesh(0, neutralNode->GetNodeIndex());
                Mesh* targetMesh  = targetPose->GetMesh(0, targetNode->GetNodeIndex());

                // if one of the nodes has no mesh, we can skip this node
                if (!neutralMesh || !targetMesh)
                {
                    continue;
                }

                // both nodes have a mesh, lets check if they have the same number of vertices as well
                const uint32 numNeutralVerts = neutralMesh->GetNumVertices();
                const uint32 numTargetVerts  = targetMesh->GetNumVertices();

                if (neutralMesh->GetNumOrgVertices() != targetMesh->GetNumOrgVertices())
                {
                    AZ_Warning("EMotionFX", false, "The number of source original vertices (%d) and target original vertices (%d) are different, cannot build a morph target for this mesh (name = '%s'). Skipping morph target.", neutralMesh->GetNumOrgVertices(), targetMesh->GetNumOrgVertices(), targetNode->GetName());
                    continue;
                }

                if (numNeutralVerts != numTargetVerts)
                {
                    AZ_Warning("EMotionFX", false, "The number of source vertices (%d) and target vertices (%d) are different, cannot build a morph target for this mesh (name = '%s'). Skipping morph target.", numNeutralVerts, numTargetVerts, targetNode->GetName());
                    continue;
                }

                // check if the mesh has differences
                uint32 numDifferent = 0;
                const AZ::Vector3*   neutralPositions = static_cast<const AZ::Vector3*>(neutralMesh->FindOriginalVertexData(Mesh::ATTRIB_POSITIONS));
                const AZ::Vector3*   neutralNormals   = static_cast<const AZ::Vector3*>(neutralMesh->FindOriginalVertexData(Mesh::ATTRIB_NORMALS));
                const AZ::u32*       neutralOrgVerts  = static_cast<const AZ::u32*>(neutralMesh->FindOriginalVertexData(Mesh::ATTRIB_ORGVTXNUMBERS));
                const AZ::Vector4*   neutralTangents  = static_cast<const AZ::Vector4*>(neutralMesh->FindOriginalVertexData(Mesh::ATTRIB_TANGENTS));
                const AZ::Vector3*   neutralBitangents= static_cast<const AZ::Vector3*>(neutralMesh->FindOriginalVertexData(Mesh::ATTRIB_BITANGENTS));
                const AZ::u32*       targetOrgVerts   = static_cast<const AZ::u32*>(targetMesh->FindOriginalVertexData(Mesh::ATTRIB_ORGVTXNUMBERS));
                const AZ::Vector3*   targetPositions  = static_cast<const AZ::Vector3*>(targetMesh->FindOriginalVertexData(Mesh::ATTRIB_POSITIONS));
                const AZ::Vector3*   targetNormals    = static_cast<const AZ::Vector3*>(targetMesh->FindOriginalVertexData(Mesh::ATTRIB_NORMALS));
                const AZ::Vector4*   targetTangents   = static_cast<const AZ::Vector4*>(targetMesh->FindOriginalVertexData(Mesh::ATTRIB_TANGENTS));
                const AZ::Vector3*   targetBitangents = static_cast<const AZ::Vector3*>(targetMesh->FindOriginalVertexData(Mesh::ATTRIB_BITANGENTS));

                // Do some simplified check to see if the mesh topology is different.
                bool differentTopology = false;
                for (uint32 v = 0; v < numNeutralVerts; ++v)
                {
                    if (neutralOrgVerts[v] != targetOrgVerts[v])
                    {
                        differentTopology = true;
                    }
                }

                if (differentTopology)
                {
                    AZ_Warning("EMotionFX", false, "The morph target's mesh ('%s') is not the same topology as the base mesh ('%s'). The vertex order changed. Morph target might not look correct.", targetNode->GetName(), neutralNode->GetName());
                }

                // first calculate the neutral and target bounds
                MCore::AABB neutralAABB;
                MCore::AABB targetAABB;
                const AZ::Transform identity = AZ::Transform::CreateIdentity();
                neutralMesh->CalcAABB(&neutralAABB, identity, 1);
                targetMesh->CalcAABB(&targetAABB, identity, 1);
                neutralAABB.Encapsulate(targetAABB); // join the two boxes, so that we have the box around those two boxes

                // now get the extent length
                float epsilon = neutralAABB.CalcRadius() * 0.0025f; // TODO: tweak this parameter
                if (epsilon < MCore::Math::epsilon)
                {
                    epsilon = MCore::Math::epsilon;
                }

                //------------------------------------------------
                // build the worker meshes
                MCore::Array< MCore::Array<uint32> > neutralVerts;
                MCore::Array< MCore::Array<uint32> > morphVerts;
                BuildWorkMesh(neutralMesh, neutralVerts);
                BuildWorkMesh(targetMesh, morphVerts);

                // calculate the bounding box
                MCore::AABB box;
                uint32 v;
                for (v = 0; v < numNeutralVerts; ++v)
                {
                    const uint32 orgVertex = neutralOrgVerts[v];

                    // calculate the delta vector between the two positions
                    const AZ::Vector3 deltaVec = targetPositions[ morphVerts[orgVertex][0] ] - neutralPositions[v];

                    // check if the vertex positions are different, so if there are mesh changes
                    if (MCore::SafeLength(deltaVec) > /*0.0001f*/ epsilon)
                    {
                        box.Encapsulate(deltaVec);
                        numDifferent++;
                    }
                }
                //------------------------------------------------

                // go to the next node and mesh if there is no difference
                if (numDifferent == 0)
                {
                    continue;
                }

                // calculate the minimum and maximum value
                const AZ::Vector3& boxMin = box.GetMin();
                const AZ::Vector3& boxMax = box.GetMax();
                float minValue = MCore::Min3<float>(boxMin.GetX(), boxMin.GetY(), boxMin.GetZ());
                float maxValue = MCore::Max3<float>(boxMax.GetX(), boxMax.GetY(), boxMax.GetZ());

                // make sure the values won't be too small
                if (maxValue - minValue < 1.0f)
                {
                    minValue -= 0.5f;
                    maxValue += 0.5f;
                }

                // check if we have tangents
                const bool hasTangents = (neutralTangents && targetTangents);
                const bool hasBitangents = (neutralBitangents && targetBitangents);

                // create the deformation data
                DeformData* deformData = DeformData::Create(neutralNode->GetNodeIndex(), numDifferent);
                deformData->mMinValue = minValue;
                deformData->mMaxValue = maxValue;

                // set all deltas for all vertices
                uint32 curVertex = 0;
                for (v = 0; v < numNeutralVerts; ++v)
                {
                    const uint32 orgVertex = neutralOrgVerts[v];

                    // calculate the delta vector between the two positions
                    const AZ::Vector3 deltaVec = targetPositions[morphVerts[orgVertex][0]] - neutralPositions[v];

                    // check if the vertex positions are different, so if there are mesh changes
                    if (MCore::SafeLength(deltaVec) > epsilon)
                    {
                        uint32 duplicateIndex = 0;
                        uint32 targetIndex = 0;

                        // if the number of duplicates is equal
                        if (neutralVerts[orgVertex].GetLength() == morphVerts[orgVertex].GetLength())
                        {
                            duplicateIndex = neutralVerts[orgVertex].Find(v);
                        }

                        // get the target vertex index
                        targetIndex = morphVerts[orgVertex][duplicateIndex];

                        AZ::Vector3 deltaTangent(0.0f, 0.0f, 0.0f);
                        if (hasTangents)
                        {
                            const AZ::Vector3 neutralTangent = neutralTangents[v].GetAsVector3().GetNormalizedSafe();
                            const AZ::Vector3 targetTangent  = targetTangents[v].GetAsVector3().GetNormalizedSafe();
                            deltaTangent = targetTangent - neutralTangent;
                        }

                        AZ::Vector3 deltaBitangent(0.0f, 0.0f, 0.0f);
                        if (hasBitangents)
                        {
                            const AZ::Vector3 neutralBitangent = neutralBitangents[v].GetNormalizedSafe();
                            const AZ::Vector3 targetBitangent  = targetBitangents[v].GetNormalizedSafe();
                            deltaBitangent = targetBitangent - neutralBitangent;
                        }

                        // setup the deform data for this vertex
                        const AZ::Vector3 neutralNormal = neutralNormals[v].GetNormalizedSafe();
                        const AZ::Vector3 targetNormal  = targetNormals[v].GetNormalizedSafe();
                        const AZ::Vector3 deltaNormal   = targetNormal - neutralNormal;
                        deformData->mDeltas[curVertex].mPosition.FromVector3(deltaVec, minValue, maxValue);
                        deformData->mDeltas[curVertex].mNormal.FromVector3(deltaNormal, -2.0f, 2.0f);
                        deformData->mDeltas[curVertex].mTangent.FromVector3(deltaTangent, -2.0f, 2.0f);
                        deformData->mDeltas[curVertex].mBitangent.FromVector3(deltaBitangent, -2.0f, 2.0f);
                        deformData->mDeltas[curVertex].mVertexNr = v;
                        curVertex++;
                    }
                }

                //LogInfo("Mesh deform data for node '%s' contains %d vertices", neutralNode->GetName(), numDifferent);

                // add the deform data
                mDeformDatas.Add(deformData);

                //-------------------------------
                // create the mesh deformer
                MeshDeformerStack* stack = neutralPose->GetMeshDeformerStack(0, neutralNode->GetNodeIndex());

                // create the stack if it doesn't yet exist
                if (stack == nullptr)
                {
                    stack = MeshDeformerStack::Create(neutralMesh);
                    neutralPose->SetMeshDeformerStack(0, neutralNode->GetNodeIndex(), stack);
                }

                // try to see if there is already some  morph deformer
                MorphMeshDeformer* deformer = nullptr;
                MeshDeformerStack* stackPtr = stack;
                deformer = (MorphMeshDeformer*)stackPtr->FindDeformerByType(MorphMeshDeformer::TYPE_ID);
                if (deformer == nullptr) // there isn't one, so create and add one
                {
                    deformer = MorphMeshDeformer::Create(neutralMesh);
                    stack->InsertDeformer(0, deformer);
                }

                // add the deform pass to the mesh deformer
                MorphMeshDeformer::DeformPass deformPass;
                deformPass.mDeformDataNr = mDeformDatas.GetLength() - 1;
                deformPass.mMorphTarget  = this;
                deformer->AddDeformPass(deformPass);

                //-------------------------------

                // make sure we end up with the same number of different vertices, otherwise the two loops
                // have different detection on if a vertex has changed or not
                MCORE_ASSERT(curVertex == numDifferent);
            }
        }

        //--------------------------------------------------------------------------------------------------

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
        uint32 i;

        // check if there is a deform data object, which works on the specified node
        const uint32 numDatas = mDeformDatas.GetLength();
        for (i = 0; i < numDatas; ++i)
        {
            if (mDeformDatas[i]->mNodeIndex == nodeIndex)
            {
                return true;
            }
        }

        // check all transforms
        const uint32 numTransforms = mTransforms.GetLength();
        for (i = 0; i < numTransforms; ++i)
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
        return mDeformDatas.GetLength();
    }


    MorphTargetStandard::DeformData* MorphTargetStandard::GetDeformData(uint32 nr) const
    {
        return mDeformDatas[nr];
    }


    void MorphTargetStandard::AddDeformData(DeformData* data)
    {
        mDeformDatas.Add(data);
    }


    // add transformation to the morph target
    void MorphTargetStandard::AddTransformation(const Transformation& transform)
    {
        mTransforms.Add(transform);
    }


    // get the number of transformations in this morph target
    uint32 MorphTargetStandard::GetNumTransformations() const
    {
        return mTransforms.GetLength();
    }


    // get the given transformation
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
        clone->mDeformDatas.Resize(mDeformDatas.GetLength());
        for (uint32 i = 0; i < mDeformDatas.GetLength(); ++i)
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


    // delete all deform datas
    void MorphTargetStandard::RemoveAllDeformDatas()
    {
        // get rid of all deform datas
        const uint32 numDeformDatas = mDeformDatas.GetLength();
        for (uint32 i = 0; i < numDeformDatas; ++i)
        {
            mDeformDatas[i]->Destroy();
        }

        mDeformDatas.Clear();
    }


    // pre-alloc memory for the deform datas
    void MorphTargetStandard::ReserveDeformDatas(uint32 numDeformDatas)
    {
        mDeformDatas.Reserve(numDeformDatas);
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
        mDeformDatas.Remove(index);
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
        const uint32 numDeformDatas = mDeformDatas.GetLength();
        for (uint32 i = 0; i < numDeformDatas; ++i)
        {
            DeformData* deformData = mDeformDatas[i];
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
