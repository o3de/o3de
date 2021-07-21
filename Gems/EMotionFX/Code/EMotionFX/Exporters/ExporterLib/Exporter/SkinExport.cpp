/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Exporter.h"
#include <EMotionFX/Source/SkinningInfoVertexAttributeLayer.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Importer/ActorFileFormat.h>
#include <MCore/Source/LogManager.h>


namespace ExporterLib
{
    // save the given skin for the given LOD level
    void SaveSkin(MCore::Stream* file, EMotionFX::Mesh* mesh, uint32 nodeIndex, bool isCollisionMesh, uint32 lodLevel, MCore::Endian::EEndianType targetEndianType)
    {
        MCORE_ASSERT(mesh);

        const uint32 numLayers = mesh->GetNumSharedVertexAttributeLayers();
        for (uint32 layerNr = 0; layerNr < numLayers; ++layerNr)
        {
            EMotionFX::VertexAttributeLayer* vertexAttributeLayer = mesh->GetSharedVertexAttributeLayer(layerNr);

            if (vertexAttributeLayer->GetType() != EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID)
            {
                continue;
            }

            EMotionFX::SkinningInfoVertexAttributeLayer* skinLayer = static_cast<EMotionFX::SkinningInfoVertexAttributeLayer*>(vertexAttributeLayer);

            // get the number of original vertices
            const uint32 numOrgVerts = skinLayer->GetNumAttributes();

            // get the number of total influences
            uint32 numTotalInfluences = 0;
            uint32 v;
            for (v = 0; v < numOrgVerts; ++v)
            {
                numTotalInfluences += aznumeric_cast<uint32>(skinLayer->GetNumInfluences(v));
            }

            // skip meshes which don't contain any influences
            if (numOrgVerts <= 0)
            {
                continue;
            }

            if (numOrgVerts != mesh->GetNumOrgVertices())
            {
                MCore::LogWarning("More/Less skinning influences (%i) found than the mesh actually has original vertices (%i).", numOrgVerts, mesh->GetNumOrgVertices());
            }

            // chunk header
            EMotionFX::FileFormat::FileChunk chunkHeader;

            // calculate the total size and write the chunk header
            uint32 totalSize = sizeof(EMotionFX::FileFormat::Actor_SkinningInfo);
            totalSize += numTotalInfluences * sizeof(EMotionFX::FileFormat::Actor_SkinInfluence);
            totalSize += numOrgVerts * sizeof(EMotionFX::FileFormat::Actor_SkinningInfoTableEntry);

            chunkHeader.mChunkID        = EMotionFX::FileFormat::ACTOR_CHUNK_SKINNINGINFO;
            chunkHeader.mSizeInBytes    = totalSize;
            chunkHeader.mVersion        = 1;

            // endian conversion
            ConvertFileChunk(&chunkHeader, targetEndianType);

            // write header and influence
            file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

            if (nodeIndex == MCORE_INVALIDINDEX32)
            {
                MCore::LogError("Skin (Nr=%i) is not connected to a valid transform node.", nodeIndex);
            }

            MCore::LogDetailedInfo(" - Skinning Info (NodeNr=%i):", nodeIndex);
            MCore::LogDetailedInfo("    + Total data size:      %d kB", totalSize / 1024);
            MCore::LogDetailedInfo("    + Num org vertices:     %d", numOrgVerts);
            MCore::LogDetailedInfo("    + Num total influences: %d", numTotalInfluences);

            EMotionFX::FileFormat::Actor_SkinningInfo skinningInfoChunk;
            memset(&skinningInfoChunk, 0, sizeof(EMotionFX::FileFormat::Actor_SkinningInfo));
            skinningInfoChunk.mIsForCollisionMesh   = isCollisionMesh ? 1 : 0;
            skinningInfoChunk.mNodeIndex            = nodeIndex;
            skinningInfoChunk.mLOD                  = lodLevel;
            skinningInfoChunk.mNumTotalInfluences   = numTotalInfluences;

            AZStd::set<AZ::u32> localJointIndices = skinLayer->CalcLocalJointIndices(numOrgVerts);
            skinningInfoChunk.mNumLocalBones = static_cast<AZ::u32>(localJointIndices.size());

            ConvertUnsignedInt(&skinningInfoChunk.mNodeIndex, targetEndianType);
            ConvertUnsignedInt(&skinningInfoChunk.mLOD, targetEndianType);
            ConvertUnsignedInt(&skinningInfoChunk.mNumTotalInfluences, targetEndianType);
            ConvertUnsignedInt(&skinningInfoChunk.mNumLocalBones, targetEndianType);

            file->Write(&skinningInfoChunk, sizeof(EMotionFX::FileFormat::Actor_SkinningInfo));

            for (v = 0; v < numOrgVerts; ++v)
            {
                const uint32 weightCount = aznumeric_cast<uint32>(skinLayer->GetNumInfluences(v));

                //LogDebug(" - Vertex#%i: NumWeights='%i'", v, weightCount);

                for (uint32 w = 0; w < weightCount; ++w)
                {
                    EMotionFX::FileFormat::Actor_SkinInfluence skinInfluence;
                    memset(&skinInfluence, 0, sizeof(EMotionFX::FileFormat::Actor_SkinInfluence));
                    skinInfluence.mNodeNr = skinLayer->GetInfluence(v, w)->GetNodeNr();
                    skinInfluence.mWeight = skinLayer->GetInfluence(v, w)->GetWeight();

                    //LogDebug("    + SkingInfluence#%i: NodeNr='%i', Weight='%f'", w, skinInfluence.mNodeNr, skinInfluence.mWeight);

                    ConvertUnsignedShort(&skinInfluence.mNodeNr, targetEndianType);
                    ConvertFloat(&skinInfluence.mWeight, targetEndianType);

                    file->Write(&skinInfluence, sizeof(EMotionFX::FileFormat::Actor_SkinInfluence));
                }
            }

            uint32 currentInfluence = 0;
            for (v = 0; v < numOrgVerts; ++v)
            {
                const uint32 weightCount = aznumeric_cast<uint32>(skinLayer->GetNumInfluences(v));

                EMotionFX::FileFormat::Actor_SkinningInfoTableEntry skinningTableEntryChunk;
                skinningTableEntryChunk.mNumElements    = weightCount;
                skinningTableEntryChunk.mStartIndex     = currentInfluence;

                ConvertUnsignedInt(&skinningTableEntryChunk.mNumElements, targetEndianType);
                ConvertUnsignedInt(&skinningTableEntryChunk.mStartIndex, targetEndianType);

                file->Write(&skinningTableEntryChunk, sizeof(EMotionFX::FileFormat::Actor_SkinningInfoTableEntry));

                currentInfluence += weightCount;
            }
        }
    }


    // save skins for all nodes for the given LOD level
    void SaveSkins(MCore::Stream* file, EMotionFX::Actor* actor, uint32 lodLevel, MCore::Endian::EEndianType targetEndianType)
    {
        MCORE_ASSERT(file);

        // get the number of nodes
        const uint32 numNodes = actor->GetNumNodes();

        MCore::LogDetailedInfo("============================================================");
        MCore::LogInfo("Skins (LOD=%d", lodLevel);
        MCore::LogDetailedInfo("============================================================");

        // iterate through all nodes
        for (uint32 i = 0; i < numNodes; i++)
        {
            // get the mesh and save it
            EMotionFX::Mesh* mesh = actor->GetMesh(lodLevel, i);
            if (mesh)
            {
                SaveSkin(file, mesh, i, false, lodLevel, targetEndianType);
            }

            // get the collision mesh and save it
            //EMotionFX::Mesh* collisionMesh    = actor->GetCollisionMesh(lodLevel, i);
            //if (collisionMesh)
            //SaveSkin( file, collisionMesh, i, true, lodLevel, targetEndianType );
        }
    }


    // save all skins for all LOD levels
    void SaveSkins(MCore::Stream* file, EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType)
    {
        // get the number of LOD levels, iterate through them and save all skins
        const uint32 numLODLevels = actor->GetNumLODLevels();
        for (uint32 i = 0; i < numLODLevels; ++i)
        {
            SaveSkins(file, actor, i, targetEndianType);
        }
    }
} // namespace ExporterLib
