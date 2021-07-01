/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Exporter.h"
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/SubMesh.h>
#include <EMotionFX/Source/VertexAttributeLayerAbstractData.h>
#include <EMotionFX/Source/Importer/Importer.h>
#include <EMotionFX/Source/Importer/ActorFileFormat.h>
#include <EMotionFX/Source/Node.h>
#include <MCore/Source/LogManager.h>


namespace ExporterLib
{
    // save the given mesh
    void SaveMesh(MCore::Stream* file, EMotionFX::Mesh* mesh, uint32 nodeIndex, bool isCollisionMesh, uint32 lodLevel, MCore::Endian::EEndianType targetEndianType)
    {
        // convert endian for abstract layers
        for (uint32 i = 0; i < mesh->GetNumVertexAttributeLayers(); ++i)
        {
            EMotionFX::VertexAttributeLayer* layer = mesh->GetVertexAttributeLayer(i);
            if (!layer->GetIsAbstractDataClass())
            {
                continue;
            }

            EMotionFX::VertexAttributeLayerAbstractData* abstractLayer = static_cast<EMotionFX::VertexAttributeLayerAbstractData*>(layer);

            const uint32 type = abstractLayer->GetType();

            EMotionFX::Importer::AbstractLayerConverter layerConvertFunction;
            layerConvertFunction = EMotionFX::Importer::StandardLayerConvert;

            // convert endian and coordinate systems of all data
            if (layerConvertFunction(abstractLayer, targetEndianType) == false)
            {
                MCore::LogError("Don't know how to endian and/or coordinate system convert layer with type %d (%s)", type, EMotionFX::Importer::ActorVertexAttributeLayerTypeToString(type));
            }
        }

        uint32 totalSize = sizeof(EMotionFX::FileFormat::Actor_Mesh);

        // add all layers to the total size
        uint32 numMeshVerts = mesh->GetNumVertices();
        for (uint32 i = 0; i < mesh->GetNumVertexAttributeLayers(); ++i)
        {
            EMotionFX::VertexAttributeLayer* layer = mesh->GetVertexAttributeLayer(i);
            if (!layer->GetIsAbstractDataClass())
            {
                continue;
            }

            EMotionFX::VertexAttributeLayerAbstractData* abstractLayer = static_cast<EMotionFX::VertexAttributeLayerAbstractData*>(layer);

            totalSize += sizeof(EMotionFX::FileFormat::Actor_VertexAttributeLayer);
            totalSize += numMeshVerts * abstractLayer->GetAttributeSizeInBytes();
            totalSize += GetStringChunkSize(layer->GetNameString());
        }

        // add the submeshes
        for (uint32 i = 0; i < mesh->GetNumSubMeshes(); ++i)
        {
            EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(i);
            totalSize += sizeof(EMotionFX::FileFormat::Actor_SubMesh);
            totalSize += sizeof(uint32) * subMesh->GetNumIndices();
            totalSize += sizeof(uint8) * subMesh->GetNumPolygons();
            totalSize += sizeof(uint32) * subMesh->GetNumBones();
        }

        // write the chunk header
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID        = EMotionFX::FileFormat::ACTOR_CHUNK_MESH;
        chunkHeader.mSizeInBytes    = totalSize;
        chunkHeader.mVersion        = 1;
        ConvertFileChunk(&chunkHeader, targetEndianType);
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));

        // write the mesh header
        EMotionFX::FileFormat::Actor_Mesh meshHeader;
        memset(&meshHeader, 0, sizeof(EMotionFX::FileFormat::Actor_Mesh));
        meshHeader.mIsCollisionMesh = isCollisionMesh ? 1 : 0;
        meshHeader.mNodeIndex       = nodeIndex;
        meshHeader.mNumLayers       = mesh->GetNumVertexAttributeLayers();
        meshHeader.mNumOrgVerts     = mesh->GetNumOrgVertices();
        meshHeader.mNumSubMeshes    = mesh->GetNumSubMeshes();
        meshHeader.mNumPolygons     = mesh->GetNumPolygons();
        meshHeader.mTotalIndices    = mesh->GetNumIndices();
        meshHeader.mLOD             = lodLevel;
        meshHeader.mTotalVerts      = (mesh->GetNumVertexAttributeLayers() > 0) ? mesh->GetNumVertices() : 0;
        meshHeader.mIsTriangleMesh  = mesh->CheckIfIsTriangleMesh();

        MCore::LogDetailedInfo("- Mesh for node with node number %d:", meshHeader.mNodeIndex);
        MCore::LogDetailedInfo("  + LOD:                   %d", meshHeader.mLOD);
        MCore::LogDetailedInfo("  + Num original vertices: %d", meshHeader.mNumOrgVerts);
        MCore::LogDetailedInfo("  + Total vertices:        %d", meshHeader.mTotalVerts);
        MCore::LogDetailedInfo("  + Total polygons:        %d", meshHeader.mNumPolygons);
        MCore::LogDetailedInfo("  + Total indices:         %d", meshHeader.mTotalIndices);
        MCore::LogDetailedInfo("  + Num submeshes:         %d", meshHeader.mNumSubMeshes);
        MCore::LogDetailedInfo("  + Num attribute layers:  %d", meshHeader.mNumLayers);
        MCore::LogDetailedInfo("  + Is collision mesh:     %s", meshHeader.mIsCollisionMesh ? "Yes" : "No");
        MCore::LogDetailedInfo("  + Is triangle mesh:      %s", meshHeader.mIsTriangleMesh ? "Yes" : "No");

        //for (uint32 i=0; i<mesh->GetNumPolygons(); ++i)
        //MCore::LogInfo("poly %d = %d verts", i, mesh->GetPolygonVertexCounts()[i] );

        // convert endian
        ConvertUnsignedInt(&meshHeader.mNodeIndex, targetEndianType);
        ConvertUnsignedInt(&meshHeader.mNumLayers, targetEndianType);
        ConvertUnsignedInt(&meshHeader.mNumSubMeshes, targetEndianType);
        ConvertUnsignedInt(&meshHeader.mNumPolygons, targetEndianType);
        ConvertUnsignedInt(&meshHeader.mTotalIndices, targetEndianType);
        ConvertUnsignedInt(&meshHeader.mTotalVerts, targetEndianType);
        ConvertUnsignedInt(&meshHeader.mNumOrgVerts, targetEndianType);
        ConvertUnsignedInt(&meshHeader.mLOD, targetEndianType);

        // write to file
        file->Write(&meshHeader, sizeof(EMotionFX::FileFormat::Actor_Mesh));

        // now save all layers
        const uint32 numLayers = mesh->GetNumVertexAttributeLayers();
        for (uint32 layerNr = 0; layerNr < numLayers; ++layerNr)
        {
            EMotionFX::VertexAttributeLayer* layer = mesh->GetVertexAttributeLayer(layerNr);
            if (!layer->GetIsAbstractDataClass())
            {
                continue;
            }

            EMotionFX::VertexAttributeLayerAbstractData* abstractLayer = static_cast<EMotionFX::VertexAttributeLayerAbstractData*>(layer);

            EMotionFX::FileFormat::Actor_VertexAttributeLayer fileLayer;
            memset(&fileLayer, 0, sizeof(EMotionFX::FileFormat::Actor_VertexAttributeLayer));
            fileLayer.mLayerTypeID          = layer->GetType();
            fileLayer.mAttribSizeInBytes    = abstractLayer->GetAttributeSizeInBytes();
            fileLayer.mEnableDeformations   = layer->GetKeepOriginals() ? 1 : 0;
            fileLayer.mIsScale              = 0;// TODO: not used

            MCore::LogDetailedInfo("  - Layer #%d (%s):", layerNr, EMotionFX::Importer::ActorVertexAttributeLayerTypeToString(fileLayer.mLayerTypeID));
            MCore::LogDetailedInfo("    + Type ID:          %d", fileLayer.mLayerTypeID);
            MCore::LogDetailedInfo("    + Attrib size:      %d bytes", fileLayer.mAttribSizeInBytes);
            MCore::LogDetailedInfo("    + Enable deforms:   %s", fileLayer.mEnableDeformations ? "Yes" : "No");
            MCore::LogDetailedInfo("    + Name:             %s", layer->GetName());

            // convert endian
            ConvertUnsignedInt(&fileLayer.mAttribSizeInBytes, targetEndianType);
            ConvertUnsignedInt(&fileLayer.mLayerTypeID, targetEndianType);

            // write the layer header
            file->Write(&fileLayer, sizeof(EMotionFX::FileFormat::Actor_VertexAttributeLayer));

            // write the name
            SaveString(layer->GetNameString(), file, targetEndianType);

            // write the layer
            file->Write((uint8*)abstractLayer->GetOriginalData(), abstractLayer->CalcTotalDataSizeInBytes(false));
        }


        // and finally save all submeshes
        const uint32 numSubMeshes = mesh->GetNumSubMeshes();
        for (uint32 s = 0; s < numSubMeshes; ++s)
        {
            EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(s);

            EMotionFX::FileFormat::Actor_SubMesh fileSubMesh;
            fileSubMesh.mMaterialIndex  = subMesh->GetMaterial();
            fileSubMesh.mNumBones       = subMesh->GetNumBones();
            fileSubMesh.mNumIndices     = subMesh->GetNumIndices();
            fileSubMesh.mNumVerts       = subMesh->GetNumVertices();
            fileSubMesh.mNumPolygons    = subMesh->GetNumPolygons();

            MCore::LogDetailedInfo("  - SubMesh #%d:", s);
            MCore::LogDetailedInfo("    + Material:       %d", fileSubMesh.mMaterialIndex);
            MCore::LogDetailedInfo("    + Num vertices:   %d", fileSubMesh.mNumVerts);
            MCore::LogDetailedInfo("    + Num indices:    %d (%d polygons)", fileSubMesh.mNumIndices, fileSubMesh.mNumPolygons);
            MCore::LogDetailedInfo("    + Num bones:      %d", fileSubMesh.mNumBones);

            // convert endian
            ConvertUnsignedInt(&fileSubMesh.mMaterialIndex, targetEndianType);
            ConvertUnsignedInt(&fileSubMesh.mNumBones, targetEndianType);
            ConvertUnsignedInt(&fileSubMesh.mNumIndices, targetEndianType);
            ConvertUnsignedInt(&fileSubMesh.mNumPolygons, targetEndianType);
            ConvertUnsignedInt(&fileSubMesh.mNumVerts, targetEndianType);

            // write the submesh header
            file->Write(&fileSubMesh, sizeof(EMotionFX::FileFormat::Actor_SubMesh));

            // write the index data
            const uint32    numIndices  = subMesh->GetNumIndices();
            const uint32    numPolygons = subMesh->GetNumPolygons();
            const uint32    startVertex = subMesh->GetStartVertex();
            uint32*         indices     = subMesh->GetIndices();
            uint8*          polyVertCounts = subMesh->GetPolygonVertexCounts();

            for (uint32 i = 0; i < numIndices; ++i)
            {
                uint32 index = indices[i] - startVertex;
                ConvertUnsignedInt(&index, targetEndianType);
                file->Write(&index, sizeof(uint32));
            }

            for (uint32 i = 0; i < numPolygons; ++i)
            {
                uint8 numPolyVerts = polyVertCounts[i];
                file->Write(&numPolyVerts, sizeof(uint8));
            }

            // write the bone numbers
            const uint32 numBones = subMesh->GetNumBones();
            for (uint32 i = 0; i < numBones; ++i)
            {
                uint32 value = subMesh->GetBone(i);
                ConvertUnsignedInt(&value, targetEndianType);
                file->Write(&value, sizeof(uint32));
            }
        }
    }


    // save meshes for all nodes for a given LOD level
    void SaveMeshes(MCore::Stream* file, EMotionFX::Actor* actor, uint32 lodLevel, MCore::Endian::EEndianType targetEndianType)
    {
        MCORE_ASSERT(file);
        MCORE_ASSERT(actor);

        MCore::LogDetailedInfo("============================================================");
        MCore::LogInfo("Meshes (LOD=%i", lodLevel);
        MCore::LogDetailedInfo("============================================================");

        // get the number of nodes
        const uint32 numNodes = actor->GetNumNodes();

        // iterate through all nodes
        for (uint32 i = 0; i < numNodes; i++)
        {
            // get the node from the actor
            //EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);

            // get the mesh and save it
            EMotionFX::Mesh* mesh = actor->GetMesh(lodLevel, i);
            if (mesh)
            {
                SaveMesh(file, mesh, i, mesh->GetIsCollisionMesh(), lodLevel, targetEndianType);
            }

            // get the collision mesh and save it
            /*      EMotionFX::Mesh* collisionMesh  = actor->GetCollisionMesh(lodLevel, i);
                    if (collisionMesh)
                        SaveMesh( file, collisionMesh, i, true, lodLevel, targetEndianType );*/
        }
    }


    // save all meshes for all nodes and all LOD levels
    void SaveMeshes(MCore::Stream* file, EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType)
    {
        // get the number of LOD levels, iterate through them and save all meshes
        const uint32 numLODLevels = actor->GetNumLODLevels();
        for (uint32 i = 0; i < numLODLevels; ++i)
        {
            SaveMeshes(file, actor, i, targetEndianType);
        }
    }
} // namespace ExporterLib
