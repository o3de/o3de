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
#include "DualQuatSkinDeformer.h"
#include "Mesh.h"
#include "Node.h"
#include "SubMesh.h"
#include "Actor.h"
#include "SkinningInfoVertexAttributeLayer.h"
#include "TransformData.h"
#include "ActorInstance.h"
#include <EMotionFX/Source/Allocators.h>
#include <MCore/Source/LogManager.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(DualQuatSkinDeformer, DeformerAllocator, 0)

    // constructor
    DualQuatSkinDeformer::DualQuatSkinDeformer(Mesh* mesh)
        : MeshDeformer(mesh)
    {
        mBones.SetMemoryCategory(EMFX_MEMCATEGORY_GEOMETRY_DEFORMERS);
    }


    // destructor
    DualQuatSkinDeformer::~DualQuatSkinDeformer()
    {
        mBones.Clear();
    }


    // creation
    DualQuatSkinDeformer* DualQuatSkinDeformer::Create(Mesh* mesh)
    {
        return aznew DualQuatSkinDeformer(mesh);
    }


    // get the type id
    uint32 DualQuatSkinDeformer::GetType() const
    {
        return TYPE_ID;
    }


    // get the subtype id
    uint32 DualQuatSkinDeformer::GetSubType() const
    {
        return SUBTYPE_ID;
    }


    // clone this class
    MeshDeformer* DualQuatSkinDeformer::Clone(Mesh* mesh)
    {
        // create the new cloned deformer
        DualQuatSkinDeformer* result = aznew DualQuatSkinDeformer(mesh);

        // copy the bone info (for precalc/optimization reasons)
        result->mBones = mBones;

        // return the result
        return result;
    }


    // the main method where all calculations are done
    void DualQuatSkinDeformer::Update(ActorInstance* actorInstance, Node* node, float timeDelta)
    {
        AZ_UNUSED(node);
        MCORE_UNUSED(timeDelta);

        const Actor* actor = actorInstance->GetActor();
        const Pose* pose = actorInstance->GetTransformData()->GetCurrentPose();

        AZ::Vector3  newPos, newNormal, newTangent, newBitangent;
        AZ::Vector3  vtxPos, normal, tangent, bitangent;
        AZ::Vector3* positions   = static_cast<AZ::Vector3*>(mMesh->FindVertexData(Mesh::ATTRIB_POSITIONS));
        AZ::Vector3* normals     = static_cast<AZ::Vector3*>(mMesh->FindVertexData(Mesh::ATTRIB_NORMALS));
        AZ::Vector4* tangents    = static_cast<AZ::Vector4*>(mMesh->FindVertexData(Mesh::ATTRIB_TANGENTS));
        AZ::Vector3* bitangents  = static_cast<AZ::Vector3*>(mMesh->FindVertexData(Mesh::ATTRIB_BITANGENTS));
        AZ::u32*     orgVerts    = static_cast<AZ::u32*>(mMesh->FindVertexData(Mesh::ATTRIB_ORGVTXNUMBERS));

        // precalc the skinning matrices
        const uint32 numBones = mBones.GetLength();
        for (uint32 i = 0; i < numBones; i++)
        {
            const uint32 nodeIndex = mBones[i].mNodeNr;
            Transform skinTransform = actor->GetInverseBindPoseTransform(nodeIndex);
            skinTransform.Multiply(pose->GetModelSpaceTransform(nodeIndex));
            mBones[i].mDualQuat.FromRotationTranslation(skinTransform.mRotation, skinTransform.mPosition);
        }

        // find the skinning layer
        SkinningInfoVertexAttributeLayer* layer = (SkinningInfoVertexAttributeLayer*)mMesh->FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);
        MCORE_ASSERT(layer);

        // if there are tangents and bitangents to skin
        if (tangents && bitangents)
        {
            const uint32 numVertices = mMesh->GetNumVertices();
            uint32 v = 0;
            uint32 orgVertex;

            SkinInfluence*  influence;
            BoneInfo*       boneInfo;
            float           weight;
            for (v = 0; v < numVertices; ++v)
            {
                // get the original vertex number
                orgVertex = *(orgVerts++);

                // reset the skinned position
                newPos = AZ::Vector3::CreateZero();
                newNormal = AZ::Vector3::CreateZero();
                newTangent = AZ::Vector3::CreateZero();
                newBitangent = AZ::Vector3::CreateZero();

                const float tangentW = tangents->GetW();
                vtxPos.Set  (positions->GetX(),  positions->GetY(),   positions->GetZ());
                normal.Set  (normals->GetX(),    normals->GetY(),     normals->GetZ());
                tangent.Set (tangents->GetX(),   tangents->GetY(),    tangents->GetZ());
                bitangent.Set(bitangents->GetX(), bitangents->GetY(), bitangents->GetZ());

                // process the skin influences for this vertex
                const uint32 numInfluences = layer->GetNumInfluences(orgVertex);
                if (numInfluences > 0)
                {
                    // get the pivot quat, used for the dot product check
                    const MCore::DualQuaternion& pivotQuat = mBones[ layer->GetInfluence(orgVertex, 0)->GetBoneNr() ].mDualQuat;

                    // our skinning dual quaternion
                    MCore::DualQuaternion skinQuat(AZ::Quaternion(0, 0, 0, 0), AZ::Quaternion(0, 0, 0, 0));

                    for (uint32 i = 0; i < numInfluences; ++i)
                    {
                        // get the influence
                        influence   = layer->GetInfluence(orgVertex, i);
                        boneInfo    = &mBones[ influence->GetBoneNr() ];
                        weight      = influence->GetWeight();

                        // check if we need to invert the dual quat
                        MCore::DualQuaternion& influenceQuat = mBones[ influence->GetBoneNr() ].mDualQuat;
                        if (influenceQuat.mReal.Dot(pivotQuat.mReal) < 0.0f)
                        {
                            influenceQuat *= -1.0f;
                        }

                        // weighted sum
                        skinQuat += influenceQuat * weight;
                    }

                    // normalize the dual quaternion
                    skinQuat.Normalize();

                    // perform skinning
                    newPos      = skinQuat.TransformPoint(vtxPos);
                    newNormal   = skinQuat.TransformVector(normal);
                    newTangent  = skinQuat.TransformVector(tangent);
                    newBitangent = skinQuat.TransformVector(bitangent);
                }
                else
                {
                    // perform the skinning
                    newPos      = vtxPos;
                    newNormal   = normal;
                    newTangent  = tangent;
                    newBitangent = bitangent;
                }

                // output the skinned values
                positions->Set  (newPos.GetX(),      newPos.GetY(),       newPos.GetZ());
                positions++;
                normals->Set    (newNormal.GetX(),   newNormal.GetY(),    newNormal.GetZ());
                normals++;
                tangents->Set   (newTangent.GetX(),  newTangent.GetY(),   newTangent.GetZ(), tangentW);
                tangents++;
                bitangents->Set (newBitangent.GetX(), newBitangent.GetY(), newBitangent.GetZ());
                bitangents++;
            }
        }
        else if (tangents && !bitangents) // tangents but no bitangents
        {
            const uint32 numVertices = mMesh->GetNumVertices();
            uint32 v = 0;
            uint32 orgVertex;

            SkinInfluence*  influence;
            BoneInfo*       boneInfo;
            float           weight;
            for (v = 0; v < numVertices; ++v)
            {
                // get the original vertex number
                orgVertex = *(orgVerts++);

                // reset the skinned position
                newPos = AZ::Vector3::CreateZero();
                newNormal = AZ::Vector3::CreateZero();
                newTangent = AZ::Vector3::CreateZero();

                const float tangentW = tangents->GetW();
                vtxPos.Set  (positions->GetX(),  positions->GetY(),   positions->GetZ());
                normal.Set  (normals->GetX(),    normals->GetY(),     normals->GetZ());
                tangent.Set (tangents->GetX(),  tangents->GetY(),   tangents->GetZ());

                // process the skin influences for this vertex
                const uint32 numInfluences = layer->GetNumInfluences(orgVertex);
                if (numInfluences > 0)
                {
                    // get the pivot quat, used for the dot product check
                    const MCore::DualQuaternion& pivotQuat = mBones[ layer->GetInfluence(orgVertex, 0)->GetBoneNr() ].mDualQuat;

                    // our skinning dual quaternion
                    MCore::DualQuaternion skinQuat(AZ::Quaternion(0, 0, 0, 0), AZ::Quaternion(0, 0, 0, 0));

                    for (uint32 i = 0; i < numInfluences; ++i)
                    {
                        // get the influence
                        influence   = layer->GetInfluence(orgVertex, i);
                        boneInfo    = &mBones[ influence->GetBoneNr() ];
                        weight      = influence->GetWeight();

                        // check if we need to invert the dual quat
                        MCore::DualQuaternion& influenceQuat = mBones[ influence->GetBoneNr() ].mDualQuat;
                        if (influenceQuat.mReal.Dot(pivotQuat.mReal) < 0.0f)
                        {
                            influenceQuat *= -1.0f;
                        }

                        // weighted sum
                        skinQuat += influenceQuat * weight;
                    }

                    // normalize the dual quaternion
                    skinQuat.Normalize();

                    // perform skinning
                    newPos      = skinQuat.TransformPoint(vtxPos);
                    newNormal   = skinQuat.TransformVector(normal);
                    newTangent  = skinQuat.TransformVector(tangent);
                }
                else
                {
                    // perform the skinning
                    newPos      = vtxPos;
                    newNormal   = normal;
                    newTangent  = tangent;
                }

                // output the skinned values
                positions->Set  (newPos.GetX(),      newPos.GetY(),       newPos.GetZ());
                positions++;
                normals->Set    (newNormal.GetX(),   newNormal.GetY(),    newNormal.GetZ());
                normals++;
                tangents->Set   (newTangent.GetX(),  newTangent.GetY(),   newTangent.GetZ(), tangentW);
                tangents++;
            }
        }
        else // there are no tangents and bitangents to skin
        {
            const uint32 numVertices = mMesh->GetNumVertices();
            uint32 v = 0;
            uint32 orgVertex;

            SkinInfluence*  influence;
            BoneInfo*       boneInfo;
            float           weight;

            for (v = 0; v < numVertices; ++v)
            {
                // get the original vertex number
                orgVertex = *(orgVerts++);

                // reset the skinned position
                newPos = AZ::Vector3::CreateZero();
                newNormal = AZ::Vector3::CreateZero();

                vtxPos.Set(positions->GetX(), positions->GetY(), positions->GetZ());
                normal.Set(normals->GetX(), normals->GetY(), normals->GetZ());

                // process the skin influences for this vertex
                const uint32 numInfluences = layer->GetNumInfluences(orgVertex);
                if (numInfluences > 0)
                {
                    // get the pivot quat, used for the dot product check
                    const MCore::DualQuaternion& pivotQuat = mBones[ layer->GetInfluence(orgVertex, 0)->GetBoneNr() ].mDualQuat;

                    // our skinning dual quaternion
                    MCore::DualQuaternion skinQuat(AZ::Quaternion(0, 0, 0, 0), AZ::Quaternion(0, 0, 0, 0));

                    for (uint32 i = 0; i < numInfluences; ++i)
                    {
                        // get the influence
                        influence   = layer->GetInfluence(orgVertex, i);
                        boneInfo    = &mBones[ influence->GetBoneNr() ];
                        weight      = influence->GetWeight();

                        // check if we need to invert the dual quat
                        MCore::DualQuaternion& influenceQuat = mBones[ influence->GetBoneNr() ].mDualQuat;
                        if (influenceQuat.mReal.Dot(pivotQuat.mReal) < 0.0f)
                        {
                            influenceQuat *= -1.0f;
                        }

                        // weighted sum
                        skinQuat += influenceQuat * weight;
                    }

                    // normalize the dual quaternion
                    skinQuat.Normalize();

                    // perform skinning
                    newPos      = skinQuat.TransformPoint(vtxPos);
                    newNormal   = skinQuat.TransformVector(normal);
                }
                else
                {
                    // perform the skinning
                    newPos      = vtxPos;
                    newNormal   = normal;
                }

                // output the skinned values
                positions->Set(newPos.GetX(), newPos.GetY(), newPos.GetZ());
                positions++;
                normals->Set(newNormal.GetX(), newNormal.GetY(), newNormal.GetZ());
                normals++;
            }
        }
    }


    // initialize the mesh deformer
    void DualQuatSkinDeformer::Reinitialize(Actor* actor, Node* node, uint32 lodLevel)
    {
        MCORE_UNUSED(actor);
        MCORE_UNUSED(node);
        MCORE_UNUSED(lodLevel);

        // clear the bone information array, but don't free the currently allocated/reserved memory
        mBones.Clear(false);

        // if there is no mesh
        if (mMesh == nullptr)
        {
            return;
        }

        // get the attribute number
        SkinningInfoVertexAttributeLayer* skinningLayer = (SkinningInfoVertexAttributeLayer*)mMesh->FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);
        MCORE_ASSERT(skinningLayer);

        // reserve space for the bone array
        //mBones.Reserve( actor->GetNumNodes() );

        // find out what bones this mesh uses
        const uint32 numOrgVerts = mMesh->GetNumOrgVertices();
        for (uint32 i = 0; i < numOrgVerts; i++)
        {
            // now we have located the skinning information for this vertex, we can see if our bones array
            // already contains the bone it uses by traversing all influences for this vertex, and checking
            // if the bone of that influence already is in the array with used bones
            const uint32 numInfluences = skinningLayer->GetNumInfluences(i);
            for (uint32 a = 0; a < numInfluences; ++a)
            {
                SkinInfluence* influence = skinningLayer->GetInfluence(i, a);

                // get the bone index in the array
                uint32 boneIndex = FindLocalBoneIndex(influence->GetNodeNr());

                // if the bone is not found in our array
                if (boneIndex == MCORE_INVALIDINDEX32)
                {
                    // add the bone to the array of bones in this deformer
                    mBones.AddEmptyExact();
                    BoneInfo& lastBone = mBones.GetLast();
                    lastBone.mNodeNr = influence->GetNodeNr();
                    lastBone.mDualQuat.Identity();
                    boneIndex = mBones.GetLength() - 1;
                }

                // set the bone number in the influence
                influence->SetBoneNr(static_cast<uint16>(boneIndex));
            }
        }

        // get rid of all items in the used bones array
        //  mBones.Shrink();
    }
} // namespace EMotionFX
