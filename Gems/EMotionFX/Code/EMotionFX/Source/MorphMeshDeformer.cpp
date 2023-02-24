/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    AZ_CLASS_ALLOCATOR_IMPL(MorphMeshDeformer, DeformerAllocator)

    // constructor
    MorphMeshDeformer::MorphMeshDeformer(Mesh* mesh)
        : MeshDeformer(mesh)
    {
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
    MeshDeformer* MorphMeshDeformer::Clone(Mesh* mesh) const
    {
        // create the new cloned deformer
        MorphMeshDeformer* result = aznew MorphMeshDeformer(mesh);

        // copy the deform passes
        result->m_deformPasses.resize(m_deformPasses.size());
        for (size_t i = 0; i < m_deformPasses.size(); ++i)
        {
            DeformPass& pass = result->m_deformPasses[i];
            pass.m_deformDataNr = m_deformPasses[i].m_deformDataNr;
            pass.m_morphTarget  = m_deformPasses[i].m_morphTarget;
            pass.m_lastNearZero = false;
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
        const size_t lodLevel = actorInstance->GetLODLevel();

        // apply all deform passes
        for (DeformPass& deformPass : m_deformPasses)
        {
            // find the morph target
            MorphTargetStandard* morphTarget = (MorphTargetStandard*)actor->GetMorphSetup(lodLevel)->FindMorphTargetByID(deformPass.m_morphTarget->GetID());
            if (morphTarget == nullptr)
            {
                continue;
            }

            // get the deform data and number of vertices to deform
            MorphTargetStandard::DeformData* deformData = morphTarget->GetDeformData(deformPass.m_deformDataNr);
            const uint32 numDeformVerts = deformData->m_numVerts;

            // this mesh deformer can't work on this mesh, because the deformdata number of vertices is bigger than the
            // number of vertices inside this mesh!
            // and that would make it crash, which isn't what we want
            if (numDeformVerts > m_mesh->GetNumVertices())
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
            if (nearZero && deformPass.m_lastNearZero)
            {
                continue;
            }

            // update the flag
            if (nearZero)
            {
                deformPass.m_lastNearZero = true;
            }
            else
            {
                deformPass.m_lastNearZero = false; // we moved away from zero influence
            }

            // output data
            AZ::Vector3* positions   = static_cast<AZ::Vector3*>(m_mesh->FindVertexData(Mesh::ATTRIB_POSITIONS));
            AZ::Vector3* normals     = static_cast<AZ::Vector3*>(m_mesh->FindVertexData(Mesh::ATTRIB_NORMALS));
            AZ::Vector4* tangents    = static_cast<AZ::Vector4*>(m_mesh->FindVertexData(Mesh::ATTRIB_TANGENTS));
            AZ::Vector3* bitangents  = static_cast<AZ::Vector3*>(m_mesh->FindVertexData(Mesh::ATTRIB_BITANGENTS));

            // input data
            const MorphTargetStandard::DeformData::VertexDelta* deltas = deformData->m_deltas;
            const float minValue = deformData->m_minValue;
            const float maxValue = deformData->m_maxValue;

            if (tangents && bitangents)
            {
                // process all vertices that we need to deform
                for (uint32 v = 0; v < numDeformVerts; ++v)
                {
                    uint32 vtxNr = deltas[v].m_vertexNr;

                    positions [vtxNr] = positions[vtxNr] + deltas[v].m_position.ToVector3(minValue, maxValue) * weight;
                    normals   [vtxNr] = normals[vtxNr] + deltas[v].m_normal.ToVector3(-2.0f, 2.0f) * weight;
                    bitangents[vtxNr] = bitangents[vtxNr] + deltas[v].m_bitangent.ToVector3(-2.0f, 2.0f) * weight;

                    const AZ::Vector3 tangentDirVector = deltas[v].m_tangent.ToVector3(-2.0f, 2.0f);
                    tangents[vtxNr] += AZ::Vector4(tangentDirVector.GetX()*weight, tangentDirVector.GetY()*weight, tangentDirVector.GetZ()*weight, 0.0f);
                }
            }
            else if (tangents && !bitangents) // tangents but no bitangents
            {
                for (uint32 v = 0; v < numDeformVerts; ++v)
                {
                    uint32 vtxNr = deltas[v].m_vertexNr;

                    positions[vtxNr] = positions[vtxNr] + deltas[v].m_position.ToVector3(minValue, maxValue) * weight;
                    normals  [vtxNr] = normals[vtxNr] + deltas[v].m_normal.ToVector3(-2.0f, 2.0f) * weight;

                    const AZ::Vector3 tangentDirVector = deltas[v].m_tangent.ToVector3(-2.0f, 2.0f);
                    tangents[vtxNr] += AZ::Vector4(tangentDirVector.GetX()*weight, tangentDirVector.GetY()*weight, tangentDirVector.GetZ()*weight, 0.0f);
                }
            }
            else // no tangents
            {
                // process all vertices that we need to deform
                for (uint32 v = 0; v < numDeformVerts; ++v)
                {
                    uint32 vtxNr = deltas[v].m_vertexNr;

                    positions[vtxNr] = positions[vtxNr] + deltas[v].m_position.ToVector3(minValue, maxValue) * weight;
                    normals[vtxNr]   = normals[vtxNr] + deltas[v].m_normal.ToVector3(-2.0f, 2.0f) * weight;
                }
            }
        }
    }


    // initialize the mesh deformer
    void MorphMeshDeformer::Reinitialize(Actor* actor, Node* node, size_t lodLevel, uint16 highestJointIndex)
    {
        MCORE_UNUSED(highestJointIndex);
        // clear the deform passes, but don't free the currently allocated/reserved memory
        m_deformPasses.clear();

        // get the morph setup
        MorphSetup* morphSetup = actor->GetMorphSetup(lodLevel);

        // get the number of morph targets and iterate through them
        const size_t numMorphTargets = morphSetup->GetNumMorphTargets();
        for (size_t i = 0; i < numMorphTargets; ++i)
        {
            // get the morph target
            MorphTargetStandard* morphTarget = static_cast<MorphTargetStandard*>(morphSetup->GetMorphTarget(i));

            // get the number of deform datas and add one deform pass per deform data
            const size_t numDeformDatas = morphTarget->GetNumDeformDatas();
            for (size_t j = 0; j < numDeformDatas; ++j)
            {
                // get the deform data and only add it to our deformer in case it belongs to our mesh
                MorphTargetStandard::DeformData* deformData = morphTarget->GetDeformData(j);
                if (deformData->m_nodeIndex == node->GetNodeIndex())
                {
                    // add an empty deform pass and fill it afterwards
                    m_deformPasses.emplace_back();
                    const size_t deformPassIndex = m_deformPasses.size() - 1;
                    m_deformPasses[deformPassIndex].m_deformDataNr = j;
                    m_deformPasses[deformPassIndex].m_morphTarget  = morphTarget;
                }
            }
        }
    }


    void MorphMeshDeformer::AddDeformPass(const DeformPass& deformPass)
    {
        m_deformPasses.emplace_back(deformPass);
    }


    size_t MorphMeshDeformer::GetNumDeformPasses() const
    {
        return m_deformPasses.size();
    }


    void MorphMeshDeformer::ReserveDeformPasses(size_t numPasses)
    {
        m_deformPasses.reserve(numPasses);
    }
} // namespace EMotionFX
