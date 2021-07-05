/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "EMotionFXConfig.h"
#include "SoftSkinDeformer.h"
#include "Mesh.h"
#include "Node.h"
#include "SubMesh.h"
#include "Actor.h"
#include "SkinningInfoVertexAttributeLayer.h"
#include "TransformData.h"
#include "ActorInstance.h"
#include <EMotionFX/Source/Allocators.h>
#include <MCore/Source/AzCoreConversions.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(SoftSkinDeformer, DeformerAllocator, 0)

    // constructor
    SoftSkinDeformer::SoftSkinDeformer(Mesh* mesh)
        : MeshDeformer(mesh)
    {
    }


    // destructor
    SoftSkinDeformer::~SoftSkinDeformer()
    {
        mNodeNumbers.clear();
        mBoneMatrices.clear();
    }


    // create
    SoftSkinDeformer* SoftSkinDeformer::Create(Mesh* mesh)
    {
        return aznew SoftSkinDeformer(mesh);
    }


    // get the type id
    uint32 SoftSkinDeformer::GetType() const
    {
        return TYPE_ID;
    }


    // get the subtype id
    uint32 SoftSkinDeformer::GetSubType() const
    {
        return SUBTYPE_ID;
    }


    // clone this class
    MeshDeformer* SoftSkinDeformer::Clone(Mesh* mesh)
    {
        // create the new cloned deformer
        SoftSkinDeformer* result = aznew SoftSkinDeformer(mesh);

        // copy the bone info (for precalc/optimization reasons)
        result->mNodeNumbers    = mNodeNumbers;
        result->mBoneMatrices   = mBoneMatrices;

        // return the result
        return result;
    }


    // the main method where all calculations are done
    void SoftSkinDeformer::Update(ActorInstance* actorInstance, Node* node, float timeDelta)
    {
        AZ_UNUSED(node);
        MCORE_UNUSED(timeDelta);

        // get some vars
        const TransformData* transformData = actorInstance->GetTransformData();
        const AZ::Matrix3x4* skinningMatrices = transformData->GetSkinningMatrices();

        // precalc the skinning matrices
        const size_t numBones = mBoneMatrices.size();
        for (size_t i = 0; i < numBones; i++)
        {
            const uint32 nodeIndex = mNodeNumbers[i];
            mBoneMatrices[i] = skinningMatrices[nodeIndex];
        }

        // find the skinning layer
        SkinningInfoVertexAttributeLayer* layer = (SkinningInfoVertexAttributeLayer*)mMesh->FindSharedVertexAttributeLayer(SkinningInfoVertexAttributeLayer::TYPE_ID);
        AZ_Assert(layer, "Cannot find skinning info");

        // Perform the skinning.
        AZ::Vector3* __restrict positions    = static_cast<AZ::Vector3*>(mMesh->FindVertexData(Mesh::ATTRIB_POSITIONS));
        AZ::Vector3* __restrict normals      = static_cast<AZ::Vector3*>(mMesh->FindVertexData(Mesh::ATTRIB_NORMALS));
        AZ::Vector4* __restrict tangents     = static_cast<AZ::Vector4*>(mMesh->FindVertexData(Mesh::ATTRIB_TANGENTS));
        AZ::Vector3* __restrict bitangents   = static_cast<AZ::Vector3*>(mMesh->FindVertexData(Mesh::ATTRIB_BITANGENTS));
        AZ::u32*     __restrict orgVerts     = static_cast<AZ::u32*>(mMesh->FindVertexData(Mesh::ATTRIB_ORGVTXNUMBERS));
        SkinVertexRange(0, mMesh->GetNumVertices(), positions, normals, tangents, bitangents, orgVerts, layer);
    }


    void SoftSkinDeformer::SkinVertexRange(uint32 startVertex, uint32 endVertex, AZ::Vector3* positions, AZ::Vector3* normals, AZ::Vector4* tangents, AZ::Vector3* bitangents, uint32* orgVerts, SkinningInfoVertexAttributeLayer* layer)
    {
        AZ::Vector3 newPos, newNormal;
        AZ::Vector3 vtxPos, normal;
        AZ::Vector4 tangent, newTangent;
        AZ::Vector3 bitangent, newBitangent;

        // if there are tangents and bitangents to skin
        if (tangents && bitangents)
        {
            for (uint32 v = startVertex; v < endVertex; ++v)
            {
                newPos = AZ::Vector3::CreateZero();
                newNormal = AZ::Vector3::CreateZero();
                newTangent = AZ::Vector4::CreateZero();
                newBitangent = AZ::Vector3::CreateZero();

                vtxPos  = positions[v];
                normal  = normals[v];
                tangent = tangents[v];
                bitangent = bitangents[v];

                const uint32 orgVertex = orgVerts[v]; // get the original vertex number
                const size_t numInfluences = layer->GetNumInfluences(orgVertex);
                for (size_t i = 0; i < numInfluences; ++i)
                {
                    const SkinInfluence* influence = layer->GetInfluence(orgVertex, i);
                    MCore::Skin(mBoneMatrices[influence->GetBoneNr()], &vtxPos, &normal, &tangent, &bitangent, &newPos, &newNormal, &newTangent, &newBitangent, influence->GetWeight());
                }

                newTangent.SetW(tangents[v].GetW());

                // output the skinned values
                positions[v]    = newPos;
                normals[v]      = newNormal;
                tangents[v]     = newTangent;
                bitangents[v]   = newBitangent;
            }
        }
        else if (tangents) // only tangents but no bitangents
        {
            for (uint32 v = startVertex; v < endVertex; ++v)
            {
                newPos = AZ::Vector3::CreateZero();
                newNormal = AZ::Vector3::CreateZero();
                newTangent = AZ::Vector4::CreateZero();

                vtxPos  = positions[v];
                normal  = normals[v];
                tangent = tangents[v];

                const uint32 orgVertex = orgVerts[v]; // get the original vertex number
                const size_t numInfluences = layer->GetNumInfluences(orgVertex);
                for (size_t i = 0; i < numInfluences; ++i)
                {
                    const SkinInfluence* influence = layer->GetInfluence(orgVertex, i);
                    MCore::Skin(mBoneMatrices[influence->GetBoneNr()], &vtxPos, &normal, &tangent, &newPos, &newNormal, &newTangent, influence->GetWeight());
                }

                newTangent.SetW(tangents[v].GetW());

                // output the skinned values
                positions[v]    = newPos;
                normals[v]      = newNormal;
                tangents[v]     = newTangent;
            }
        }
        else // there are no tangents and bitangents to skin
        {
            for (uint32 v = startVertex; v < endVertex; ++v)
            {
                newPos = AZ::Vector3::CreateZero();
                newNormal = AZ::Vector3::CreateZero();

                vtxPos  = positions[v];
                normal  = normals[v];

                // skin the vertex
                const uint32 orgVertex = orgVerts[v]; // get the original vertex number
                const size_t numInfluences = layer->GetNumInfluences(orgVertex);
                for (size_t i = 0; i < numInfluences; ++i)
                {
                    const SkinInfluence* influence = layer->GetInfluence(orgVertex, i);
                    MCore::Skin(mBoneMatrices[influence->GetBoneNr()], &vtxPos, &normal, &newPos, &newNormal, influence->GetWeight());
                }

                // output the skinned values
                positions[v]    = newPos;
                normals[v]      = newNormal;
            }
        }
    }


    // initialize the mesh deformer
    void SoftSkinDeformer::Reinitialize(Actor* actor, Node* node, uint32 lodLevel)
    {
        MCORE_UNUSED(actor);
        MCORE_UNUSED(node);
        MCORE_UNUSED(lodLevel);

        // clear the bone information array
        mBoneMatrices.clear();
        mNodeNumbers.clear();

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
            const AZ::Matrix3x4 mat = AZ::Matrix3x4::CreateIdentity();

            const size_t numInfluences = skinningLayer->GetNumInfluences(i);
            for (size_t a = 0; a < numInfluences; ++a)
            {
                SkinInfluence* influence = skinningLayer->GetInfluence(i, a);

                // get the bone index in the array
                uint32 boneIndex = FindLocalBoneIndex(influence->GetNodeNr());

                // if the bone is not found in our array
                if (boneIndex == MCORE_INVALIDINDEX32)
                {
                    // add the bone to the array of bones in this deformer
                    mNodeNumbers.emplace_back(influence->GetNodeNr());
                    mBoneMatrices.emplace_back(mat);
                    boneIndex = static_cast<uint32>(mBoneMatrices.size()) - 1;
                }

                // set the bone number in the influence
                influence->SetBoneNr(static_cast<uint16>(boneIndex));
                //MCore::LogInfo("influence %d/%d = %s with weight %f [nodeIndex=%d] [boneIndex=%d]", a+1, numInfluences, actor->GetNode(influence->GetNodeNr())->GetName(), influence->GetWeight(), influence->GetNodeNr(), boneIndex);
            }
        }
        // get rid of all items in the used bones array
        //  mBones.Shrink();
    }
} // namespace EMotionFX
