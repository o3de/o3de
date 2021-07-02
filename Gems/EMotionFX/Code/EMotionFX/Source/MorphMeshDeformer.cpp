/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "MorphMeshDeformer.h"
#include "MorphTargetStandard.h"
#include "MorphSetup.h"
#include "MorphSetupInstance.h"
#include "Mesh.h"
#include "Node.h"
#include "Actor.h"
#include "ActorInstance.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MorphMeshDeformer, DeformerAllocator, 0)

    // constructor
    MorphMeshDeformer::MorphMeshDeformer(Mesh* mesh)
        : MeshDeformer(mesh)
    {
        mDeformPasses.SetMemoryCategory(EMFX_MEMCATEGORY_GEOMETRY_DEFORMERS);
    }


    // destructor
    MorphMeshDeformer::~MorphMeshDeformer()
    {
    }


    // create
    MorphMeshDeformer* MorphMeshDeformer::Create(Mesh* mesh)
    {
        return aznew MorphMeshDeformer(mesh);
    }


    // get the unique type id
    uint32 MorphMeshDeformer::GetType() const
    {
        return MorphMeshDeformer::TYPE_ID;
    }


    // get the subtype id
    uint32 MorphMeshDeformer::GetSubType() const
    {
        return MorphMeshDeformer::SUBTYPE_ID;
    }


    // clone this class
    MeshDeformer* MorphMeshDeformer::Clone(Mesh* mesh)
    {
        // create the new cloned deformer
        MorphMeshDeformer* result = aznew MorphMeshDeformer(mesh);

        // copy the deform passes
        result->mDeformPasses.Resize(mDeformPasses.GetLength());
        for (uint32 i = 0; i < mDeformPasses.GetLength(); ++i)
        {
            DeformPass& pass = result->mDeformPasses[i];
            pass.mDeformDataNr = mDeformPasses[i].mDeformDataNr;
            pass.mMorphTarget  = mDeformPasses[i].mMorphTarget;
            pass.mLastNearZero = false;
        }

        // return the result
        return result;
    }


    // the main method where all calculations are done
    void MorphMeshDeformer::Update(ActorInstance* actorInstance, Node* node, float timeDelta)
    {
        MCORE_UNUSED(node);
        MCORE_UNUSED(timeDelta);

        // get the actor instance and its LOD level
        Actor* actor = actorInstance->GetActor();
        const uint32 lodLevel = actorInstance->GetLODLevel();

        // apply all deform passes
        const uint32 numPasses = mDeformPasses.GetLength();
        for (uint32 i = 0; i < numPasses; ++i)
        {
            // find the morph target
            MorphTargetStandard* morphTarget = (MorphTargetStandard*)actor->GetMorphSetup(lodLevel)->FindMorphTargetByID(mDeformPasses[i].mMorphTarget->GetID());
            if (morphTarget == nullptr)
            {
                continue;
            }

            // get the deform data and number of vertices to deform
            MorphTargetStandard::DeformData* deformData = morphTarget->GetDeformData(mDeformPasses[i].mDeformDataNr);
            const uint32 numDeformVerts = deformData->mNumVerts;

            // this mesh deformer can't work on this mesh, because the deformdata number of vertices is bigger than the
            // number of vertices inside this mesh!
            // and that would make it crash, which isn't what we want
            if (numDeformVerts > mMesh->GetNumVertices())
            {
                continue;
            }

            // get the weight value and convert it to a range based weight value
            MorphSetupInstance::MorphTarget* morphTargetInstance = actorInstance->GetMorphSetupInstance()->FindMorphTargetByID(morphTarget->GetID());
            float weight = morphTargetInstance->GetWeight();

            // clamp the weight
            weight = MCore::Clamp<float>(weight, morphTarget->GetRangeMin(), morphTarget->GetRangeMax());

            // nothing to do when the weight is too small
            const bool nearZero = (MCore::Math::Abs(weight) < 0.0001f);

            // we are near zero, and the previous frame as well, so we can return
            if (nearZero && mDeformPasses[i].mLastNearZero)
            {
                continue;
            }

            // update the flag
            if (nearZero)
            {
                mDeformPasses[i].mLastNearZero = true;
            }
            else
            {
                mDeformPasses[i].mLastNearZero = false; // we moved away from zero influence
            }

            // output data
            AZ::Vector3* positions   = static_cast<AZ::Vector3*>(mMesh->FindVertexData(Mesh::ATTRIB_POSITIONS));
            AZ::Vector3* normals     = static_cast<AZ::Vector3*>(mMesh->FindVertexData(Mesh::ATTRIB_NORMALS));
            AZ::Vector4* tangents    = static_cast<AZ::Vector4*>(mMesh->FindVertexData(Mesh::ATTRIB_TANGENTS));
            AZ::Vector3* bitangents  = static_cast<AZ::Vector3*>(mMesh->FindVertexData(Mesh::ATTRIB_BITANGENTS));

            // input data
            const MorphTargetStandard::DeformData::VertexDelta* deltas = deformData->mDeltas;
            const float minValue = deformData->mMinValue;
            const float maxValue = deformData->mMaxValue;

            if (tangents && bitangents)
            {
                // process all vertices that we need to deform
                uint32 vtxNr;
                for (uint32 v = 0; v < numDeformVerts; ++v)
                {
                    vtxNr = deltas[v].mVertexNr;

                    positions [vtxNr] = positions[vtxNr] + deltas[v].mPosition.ToVector3(minValue, maxValue) * weight;
                    normals   [vtxNr] = normals[vtxNr] + deltas[v].mNormal.ToVector3(-2.0f, 2.0f) * weight;
                    bitangents[vtxNr] = bitangents[vtxNr] + deltas[v].mBitangent.ToVector3(-2.0f, 2.0f) * weight;

                    const AZ::Vector3 tangentDirVector = deltas[v].mTangent.ToVector3(-2.0f, 2.0f);
                    tangents[vtxNr] += AZ::Vector4(tangentDirVector.GetX()*weight, tangentDirVector.GetY()*weight, tangentDirVector.GetZ()*weight, 0.0f);
                }
            }
            else if (tangents && !bitangents) // tangents but no bitangents
            {
                uint32 vtxNr;
                for (uint32 v = 0; v < numDeformVerts; ++v)
                {
                    vtxNr = deltas[v].mVertexNr;

                    positions[vtxNr] = positions[vtxNr] + deltas[v].mPosition.ToVector3(minValue, maxValue) * weight;
                    normals  [vtxNr] = normals[vtxNr] + deltas[v].mNormal.ToVector3(-2.0f, 2.0f) * weight;

                    const AZ::Vector3 tangentDirVector = deltas[v].mTangent.ToVector3(-2.0f, 2.0f);
                    tangents[vtxNr] += AZ::Vector4(tangentDirVector.GetX()*weight, tangentDirVector.GetY()*weight, tangentDirVector.GetZ()*weight, 0.0f);
                }
            }
            else // no tangents
            {
                // process all vertices that we need to deform
                uint32 vtxNr;
                for (uint32 v = 0; v < numDeformVerts; ++v)
                {
                    vtxNr = deltas[v].mVertexNr;

                    positions[vtxNr] = positions[vtxNr] + deltas[v].mPosition.ToVector3(minValue, maxValue) * weight;
                    normals[vtxNr]   = normals[vtxNr] + deltas[v].mNormal.ToVector3(-2.0f, 2.0f) * weight;
                }
            }
        }
    }


    // initialize the mesh deformer
    void MorphMeshDeformer::Reinitialize(Actor* actor, Node* node, uint32 lodLevel)
    {
        // clear the deform passes, but don't free the currently allocated/reserved memory
        mDeformPasses.Clear(false);

        // get the morph setup
        MorphSetup* morphSetup = actor->GetMorphSetup(lodLevel);

        // get the number of morph targets and iterate through them
        const uint32 numMorphTargets = morphSetup->GetNumMorphTargets();
        for (uint32 i = 0; i < numMorphTargets; ++i)
        {
            // get the morph target
            MorphTargetStandard* morphTarget = static_cast<MorphTargetStandard*>(morphSetup->GetMorphTarget(i));

            // get the number of deform datas and add one deform pass per deform data
            const uint32 numDeformDatas = morphTarget->GetNumDeformDatas();
            for (uint32 j = 0; j < numDeformDatas; ++j)
            {
                // get the deform data and only add it to our deformer in case it belongs to our mesh
                MorphTargetStandard::DeformData* deformData = morphTarget->GetDeformData(j);
                if (deformData->mNodeIndex == node->GetNodeIndex())
                {
                    // add an empty deform pass and fill it afterwards
                    mDeformPasses.AddEmpty();
                    const uint32 deformPassIndex = mDeformPasses.GetLength() - 1;
                    mDeformPasses[deformPassIndex].mDeformDataNr = j;
                    mDeformPasses[deformPassIndex].mMorphTarget  = morphTarget;
                }
            }
        }
    }


    void MorphMeshDeformer::AddDeformPass(const DeformPass& deformPass)
    {
        mDeformPasses.Add(deformPass);
    }


    uint32 MorphMeshDeformer::GetNumDeformPasses() const
    {
        return mDeformPasses.GetLength();
    }


    void MorphMeshDeformer::ReserveDeformPasses(uint32 numPasses)
    {
        mDeformPasses.Reserve(numPasses);
    }
} // namespace EMotionFX
