/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/


#include <AzCore/Math/Aabb.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>

#include <vector>

#include "MeshletsAssets.h"

namespace AZ
{
    namespace Meshlets
    {
        //======================================================================
        //                            MeshletsModel
        //======================================================================
        uint32_t MeshletsModel::s_modelNumber = 0;

        //----------------------------------------------------------------------
        // Meshlets Generation.
        // Resulted operation will alter the mesh
        //----------------------------------------------------------------------
        void MeshletsModel::debugMarkMeshletsUVs(GeneratorMesh& mesh)
        {
            for (uint32_t meshletId = 0; meshletId < m_meshletsData.Descriptors.size(); ++meshletId)
            {
                meshopt_Meshlet& meshlet = m_meshletsData.Descriptors[meshletId];
                float textureCoordU = (meshletId % 3) * 0.5f;
                float textureCoordV = ((meshletId / 3) % 3) * 0.5f;
                for (uint32_t triIdx = 0 ; triIdx < meshlet.triangle_count; ++triIdx)
                {
                    uint32_t encodedTri = m_meshletsData.EncodedTriangles[meshlet.triangle_offset + triIdx];
                    // Next bring decode the uint32_t and separate into three elements.
                    uint8_t vtxIndirectIndex[3] = {
                        uint8_t((encodedTri >> 0) & 0xff),
                        uint8_t((encodedTri >> 8) & 0xff),
                        uint8_t((encodedTri >> 16) & 0xff)
                    };

                    for (uint32_t vtx = 0; vtx < 3; ++vtx)
                    {
                        uint32_t vtxIndex = m_meshletsData.IndicesIndirection[meshlet.vertex_offset + vtxIndirectIndex[vtx]];

                        mesh.vertices[vtxIndex].tx = textureCoordU;
                        mesh.vertices[vtxIndex].ty = textureCoordV;
                    }
                }
            }
        }

        // The results of using this function will alter the mesh UVs
        void debugMarkVertexUVs(GeneratorMesh& mesh)
        {
            for (uint32_t vtxIndex = 0; vtxIndex < mesh.vertices.size(); ++vtxIndex)
            {
                float tx = (vtxIndex % 5) * 0.25f;
                float ty = ((vtxIndex / 5) % 5) * 0.25f;
                mesh.vertices[vtxIndex].tx = tx;
                mesh.vertices[vtxIndex].ty = ty;
            }
        }

        Data::Instance<RPI::ShaderResourceGroup> MeshletsModel::CreateShaderResourceGroup(
            Data::Instance<RPI::Shader> shader,
            const char* shaderResourceGroupId,
            [[maybe_unused]] const char* moduleName)
        {
            Data::Instance<RPI::ShaderResourceGroup> srg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), AZ::Name{ shaderResourceGroupId });
            if (!srg)
            {
                AZ_Error(moduleName, false, "Failed to create shader resource group");
                return nullptr;
            }
            return srg;
        }

        uint32_t MeshletsModel::CreateMeshlets(GeneratorMesh& mesh)
        {
            const size_t max_vertices = 64;
            const size_t max_triangles = 64;  // NVidia-recommended 126, rounded down to a multiple of 4 - set to 64 based on GPU and data generated
            const float cone_weight = 0.5f;   // note: should be set to 0 unless cone culling is used at runtime!

            //----------------------------------------------------------------
            size_t max_meshlets = meshopt_buildMeshletsBound(mesh.indices.size(), max_vertices, max_triangles);

/*
            // NO scan seems to return more localized meshlets
            m_meshletsData.meshlets.resize(meshopt_buildMeshlets(
            &m_meshletsData.meshlets[0], &m_meshletsData.meshlet_vertices[0], &m_meshletsData.meshlet_triangles[0],
            &mesh.indices[0], mesh.indices.size(),
            &mesh.vertices[0].px, mesh.vertices.size(), sizeof(GeneratorVertex),
            max_vertices, max_triangles, cone_weight));

            if (!m_meshletsData.meshlets.size())
            {
                printf("Error generating meshlets - no meshlets were built\n");
                return 0;
            }

            // this is an example of how to trim the vertex/triangle arrays when copying data out to GPU storage
            const meshopt_Meshlet& last = m_meshletsData.meshlets.back();
            m_meshletsData.meshlet_vertices.resize(last.vertex_offset + last.vertex_count);
            m_meshletsData.meshlet_triangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
            uint32_t meshletsAmount = (uint32_t) m_meshletsData.meshlets.size();
            //----------------------------------------------------------------
            */

            ////////////////////////////
            std::vector<meshopt_Meshlet> meshlets(max_meshlets);
            std::vector<unsigned int> meshlet_vertices(max_meshlets * max_vertices);        // Vertex Index indirection map
            std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3); // Meshlet triangles into the vertex index indirection - local to meshlet.

            // NO scan seems to return more localized meshlets
            meshlets.resize(meshopt_buildMeshlets(
                &meshlets[0], &meshlet_vertices[0], &meshlet_triangles[0], &mesh.indices[0], mesh.indices.size(),
                &mesh.vertices[0].px, mesh.vertices.size(), sizeof(GeneratorVertex), max_vertices, max_triangles, cone_weight));

            // this is an example of how to trim the vertex/triangle arrays when copying data out to GPU storage
            const meshopt_Meshlet& last = meshlets.back();
            meshlet_vertices.resize(last.vertex_offset + last.vertex_count);
            meshlet_triangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
            uint32_t meshletsAmount = (uint32_t) meshlets.size();

            m_meshletsData.Descriptors = meshlets;
            m_meshletsData.IndicesIndirection = meshlet_vertices;
            m_meshletsData.EncodeTrianglesData(meshlet_triangles);
            m_meshletsData.ValidateData((uint32_t)meshlet_vertices.size());
            ////////////////////////////

            // Add this in order to display meshlet separation - debug purpose only
            static bool markTextureCoordinates = true;
            if (markTextureCoordinates)
            {
                debugMarkMeshletsUVs(mesh);
            }

            AZ_Warning("Meshlets", false, "Successfully generated [%d] meshlets\n", meshletsAmount);
            return meshletsAmount;
        }

        //----------------------------------------------------------------------
        // Enhanced (Meshlet) Model Creation
        //----------------------------------------------------------------------
        Data::Asset<RPI::BufferAsset> MeshletsModel::CreateBufferAsset(
            const AZStd::string& bufferName,
            const RHI::BufferViewDescriptor& bufferViewDescriptor,
            RHI::BufferBindFlags bufferBindFlags,
            const void* data
        )
        {
            RPI::BufferAssetCreator creator;
            creator.Begin(Uuid::CreateRandom());

            RHI::BufferDescriptor bufferDescriptor;
            bufferDescriptor.m_bindFlags = bufferBindFlags;
            bufferDescriptor.m_byteCount = static_cast<uint64_t>(bufferViewDescriptor.m_elementSize) * static_cast<uint64_t>(bufferViewDescriptor.m_elementCount);

            if (data)
            {
                creator.SetBuffer(data, bufferDescriptor.m_byteCount, bufferDescriptor);
            }

            creator.SetBufferViewDescriptor(bufferViewDescriptor);
            creator.SetUseCommonPool(RPI::CommonBufferPoolType::StaticInputAssembly);

            Data::Asset<RPI::BufferAsset> bufferAsset;

            // The next line is the actual buffer asset creation
            [[maybe_unused]] bool creationSuccessful = creator.End(bufferAsset);
            AZ_Error("Meshlets", creationSuccessful, "Error -- creating buffer [%s]", bufferName.c_str());

            return bufferAsset;
        }

        bool MeshletsModel::ProcessBuffersData(float* position, uint32_t vtxNum)
        {
            const float maxVertexSizeSqr = 99.9f * 99.9f;  // under 100 meters
            for (uint32_t vtx = 0; vtx < vtxNum; ++vtx, position += 3)
            {
                Vector3 positionV3 = Vector3(position[0], position[1], position[2]);

                float lengthSq = positionV3.GetLengthSq();
                if (lengthSq < maxVertexSizeSqr)
                {
                    m_aabb.AddPoint(positionV3);
                }
                else
                {
                    AZ_Warning("Meshlets", false, "Warning -- vertex [%d:%d] out of bound (%.2f, %.2f, %.2f) in model [%s]",
                        vtx, vtxNum, positionV3.GetX(), positionV3.GetY(), positionV3.GetZ(), m_name.c_str());
                }
            }

            AZ_Error("Meshlets", m_aabb.IsValid(), "Error --- Model [%s] AABB is invalid - all [%d] vertices are corrupted",
                m_name.c_str(), vtxNum);
            return m_aabb.IsValid() ? true : false;
        }

        // The following method creates a new model out of a given meshLodAsset.
        // Moving the creator outside calling method and have the new model contain several
        // Lods did not work as it needs to be local to the block!
        uint32_t MeshletsModel::CreateMeshletsModel(
            const RPI::ModelLodAsset::Mesh& meshAsset )
        {
            //-------------------------------------------
            // Start model creation
            RPI::ModelAssetCreator modelAssetCreator;
            Uuid modelId = Uuid::CreateRandom();
            modelAssetCreator.Begin(Uuid::CreateRandom());
            modelAssetCreator.SetName(m_name);
            //-------------------------------------------

            // setup a stream layout and shader input contract for the vertex streams
            RHI::Format IndexStreamFormat;
            RHI::Format streamFromat;
            RHI::BufferViewDescriptor bufferDescriptors[10];

            uint32_t meshletsAmount = 0;
            RHI::BufferBindFlags defaultBindFlags = RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::ShaderRead;
//            RHI::BufferBindFlags readWriteBindFlags = RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::ShaderReadWrite;

            // Index buffer
            uint32_t indexCount = 0;
            const RPI::BufferAssetView* bufferAssetView = &meshAsset.GetIndexBufferAssetView();
            uint8_t* indices = RetrieveBufferData(bufferAssetView, IndexStreamFormat, 0, indexCount, bufferDescriptors[0]);

            // With this flag specified we get an error at buffer pool level in ValidateInitRequest
//            const auto indicesAsset = CreateBufferAsset(IndicesSemanticName.GetStringView(), bufferDescriptors[0], readWriteBindFlags, indices);
            const auto indicesAsset = CreateBufferAsset(IndicesSemanticName.GetStringView(), bufferDescriptors[0], defaultBindFlags, indices);

            // Vertex streams
            uint32_t vertexCount = 0;
            bufferAssetView = meshAsset.GetSemanticBufferAssetView(PositionSemanticName);
            float* positions =
                (float*)RetrieveBufferData(bufferAssetView, streamFromat, 0, vertexCount, bufferDescriptors[1]);
            const auto positionsAsset = CreateBufferAsset(PositionSemanticName.GetStringView(), bufferDescriptors[1], defaultBindFlags, positions);

            bufferAssetView = meshAsset.GetSemanticBufferAssetView(NormalSemanticName);
            float* normals = positions ?
                (float*)RetrieveBufferData(bufferAssetView, streamFromat, vertexCount, vertexCount, bufferDescriptors[2]) :
                nullptr;
            const auto normalsAsset = CreateBufferAsset(NormalSemanticName.GetStringView(), bufferDescriptors[2], defaultBindFlags, normals);

            bufferAssetView = meshAsset.GetSemanticBufferAssetView(UVSemanticName);
            float* texCoords = normals && bufferAssetView ?
                (float*)RetrieveBufferData(bufferAssetView, streamFromat, vertexCount, vertexCount, bufferDescriptors[3]) :
                nullptr;

            bufferAssetView = meshAsset.GetSemanticBufferAssetView(TangentSemanticName);
            [[maybe_unused]] float* tangents = normals && bufferAssetView ?
                (float*)RetrieveBufferData(bufferAssetView, streamFromat, vertexCount, vertexCount, bufferDescriptors[4]) :
                nullptr;
            const auto tangentsAsset = CreateBufferAsset(TangentSemanticName.GetStringView(), bufferDescriptors[4], defaultBindFlags, tangents);

            bufferAssetView = meshAsset.GetSemanticBufferAssetView(BiTangentSemanticName);
            [[maybe_unused]] float* bitangents = normals && bufferAssetView ?
                (float*)RetrieveBufferData(bufferAssetView, streamFromat, vertexCount, vertexCount, bufferDescriptors[5]) :
                nullptr;
            const auto bitangentsAsset = CreateBufferAsset(BiTangentSemanticName.GetStringView(), bufferDescriptors[5], defaultBindFlags, bitangents);

            if (!positionsAsset || !normalsAsset || !tangentsAsset || !indicesAsset || !texCoords)
            {
                AZ_Error("Meshlets", false, "Failed to create meshlet model [%s] - buffer assets were not created successfully", m_name.c_str());
                return 0;
            }

            // The following is crucial for the AABB generation - it can be used
            // for scaling the actual vertices or create transform based on it.
            ProcessBuffersData(positions, vertexCount);

            meshletsAmount = CreateMeshlets(positions, normals, texCoords, vertexCount,
                (uint16_t*)indices, indexCount, IndexStreamFormat);

            if (!meshletsAmount)
            {
                AZ_Error("Meshlets", false, "Failed to create meshlet model [%s] - the meshlet creation process failed", m_name.c_str());
//                return 0;
            }

            // Done only here since we update the UV data to represent meshlets coloring
            const auto texCoordsAsset = CreateBufferAsset(UVSemanticName.GetStringView(), bufferDescriptors[3], defaultBindFlags, texCoords);
//            const auto texCoordsAsset = CreateBufferAsset(UVSemanticName.GetStringView(), bufferDescriptors[3], readWriteBindFlags, texCoords);

            // Model LOD Creation
            {
                //--------------------------------------------
                RPI::ModelLodAssetCreator modelLodAssetCreator;
                modelLodAssetCreator.Begin(Uuid::CreateRandom());

                modelLodAssetCreator.BeginMesh();
                {
                    // Original model replication
                    {
                        modelLodAssetCreator.SetMeshAabb(AZStd::move(m_aabb));
                        modelLodAssetCreator.SetMeshName(Name{ m_name.c_str() });

                        modelLodAssetCreator.SetMeshIndexBuffer({ indicesAsset, bufferDescriptors[0] });

                        modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "POSITION" }, PositionSemanticName,
                            RPI::BufferAssetView{ positionsAsset, bufferDescriptors[1] });
                        modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "NORMAL" }, NormalSemanticName,
                            RPI::BufferAssetView{ normalsAsset, bufferDescriptors[2] });
                        modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "UV" }, UVSemanticName,
                            RPI::BufferAssetView{ texCoordsAsset, bufferDescriptors[3] });
                        modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "TANGENT" }, TangentSemanticName,
                            RPI::BufferAssetView{ tangentsAsset, bufferDescriptors[4] });
                        modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "BITANGENT" }, BiTangentSemanticName,
                            RPI::BufferAssetView{ bitangentsAsset, bufferDescriptors[5] });
                    }

                    // Meshlets data creation
                    if (meshletsAmount > 0)
                    {
                        // Meshlets descriptors buffer
                        const auto meshletsDesc = RHI::BufferViewDescriptor::CreateTyped(
                            0, meshletsAmount, RHI::Format::R32G32B32A32_UINT);
                        const auto meshletsAsset = CreateBufferAsset(MeshletsDescriptorsName.GetCStr(), meshletsDesc,
                            defaultBindFlags, (void*)m_meshletsData.Descriptors.data());
                        modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ MeshletsDescriptorsName }, MeshletsDescriptorsName,
                            RPI::BufferAssetView{ meshletsAsset, meshletsDesc });

                        // Meshlets triangles - sending it as uint32 to simplify calculations
                        // The triangles data is encoded - each uint32_t is 3 indices of 8 bits.
                        const auto meshletsTrisDesc = RHI::BufferViewDescriptor::CreateTyped(
                            0, (uint32_t)m_meshletsData.EncodedTriangles.size(), RHI::Format::R32_UINT);
                        const auto meshletsTrisAsset = CreateBufferAsset(MeshletsTrianglesName.GetCStr(), meshletsTrisDesc,
                            defaultBindFlags, (void*)m_meshletsData.EncodedTriangles.data());
                        modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ MeshletsTrianglesName }, MeshletsTrianglesName,
                            RPI::BufferAssetView{ meshletsTrisAsset, meshletsTrisDesc });

                        // Meshlets indirect indices buffer
                        const auto meshletsIndicesDesc = RHI::BufferViewDescriptor::CreateTyped(
                            0, (uint32_t)m_meshletsData.IndicesIndirection.size(), RHI::Format::R32_UINT);
                        const auto meshletsIndicesAsset = CreateBufferAsset(MeshletsIndicesLookupName.GetCStr(), meshletsIndicesDesc,
                            defaultBindFlags, (void*)m_meshletsData.IndicesIndirection.data());
                        modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ MeshletsIndicesLookupName }, MeshletsIndicesLookupName,
                            RPI::BufferAssetView{ meshletsIndicesAsset, meshletsIndicesDesc });
                    }
                }
                modelLodAssetCreator.EndMesh();

                // Create the model LOD based on the model LOD asset we created
                Data::Asset<RPI::ModelLodAsset> modelLodAsset;
                if (!modelLodAssetCreator.End(modelLodAsset))
                {
                    AZ_Error("Meshlets", false, "Error -- creating model [%s] - ModelLoadAssetCreator.End() failed",
                        m_name.c_str());
                    return 0;
                }

                // Add the LOD model asset created to the model asset.
                modelAssetCreator.AddLodAsset(AZStd::move(modelLodAsset));

                //-------------------------------------------
                // Final stage - create the model based on the created assets
                Data::Asset<RPI::ModelAsset> modelAsset;

                if (!modelAssetCreator.End(modelAsset))
                {
                    AZ_Error("Meshlets", false, "Error -- creating model [%s] - model asset was not created", m_name.c_str());
                    return 0;
                }

                m_meshletsModel = RPI::Model::FindOrCreate(modelAsset);
                if (!m_meshletsModel)
                {
                    AZ_Error("Meshlets", false, "Error -- creating model [%s] - model could not be found or created", m_name.c_str());
                    return 0;
                }
                //-------------------------------------------
            }

            return meshletsAmount;
        }


        //----------------------------------------------------------------------
        // Model Traversal and Data Copy for Creation
        //----------------------------------------------------------------------
        uint8_t* MeshletsModel::RetrieveBufferData(
            const RPI::BufferAssetView* bufferView,
            RHI::Format& format,
            uint32_t expectedAmount, uint32_t &existingAmount,
            RHI::BufferViewDescriptor& bufferDesc
        )
        {
            const Data::Asset<RPI::BufferAsset>& bufferAsset = bufferView->GetBufferAsset();
//            const RHI::BufferViewDescriptor& bufferDesc = bufferView->GetBufferViewDescriptor();
            bufferDesc = bufferView->GetBufferViewDescriptor();
            existingAmount = bufferDesc.m_elementCount;

            if ((bufferDesc.m_elementOffset != 0) ||
                ((existingAmount != expectedAmount) && (expectedAmount != 0)))
            {
                AZ_Error("Meshlets", false, "More than a single mesh, or non-matching elements count");
                return nullptr;
            }
            format = bufferDesc.m_elementFormat;
            AZStd::span<const uint8_t> bufferData = bufferAsset->GetBuffer();
            return (uint8_t *) bufferData.data();
        }

        uint32_t MeshletsModel::CreateMeshlets(
            float* positions, float* normals, float* texCoords, uint32_t vtxNum,
            uint16_t* indices, uint32_t idxNum, RHI::Format IndexStreamFormat
        )
        {
            GeneratorMesh mesh;

            // Filling the mesh data for the meshlet library
            mesh.vertices.resize(vtxNum);
            for (uint32_t vtx = 0, posIdx=0, uvIdx=0; vtx < vtxNum; ++vtx, posIdx+=3, uvIdx+=2)
            {
                GeneratorVertex& genVtx = mesh.vertices[vtx];
                genVtx.px = positions[posIdx];
                genVtx.py = positions[posIdx+1];
                genVtx.pz = positions[posIdx+2];
                genVtx.nx = normals[posIdx];
                genVtx.ny = normals[posIdx + 1];
                genVtx.nz = normals[posIdx + 2];
                genVtx.tx = texCoords[uvIdx];
                genVtx.ty = texCoords[uvIdx + 1];
            }

            mesh.indices.resize(idxNum);
            if (IndexStreamFormat == RHI::Format::R16_UINT)
            {   // 16 bits index format 0..64K vertices
                for (uint32_t idx = 0; idx < idxNum; ++idx)
                {
                    mesh.indices[idx] = (unsigned int)indices[idx];
                }
            }
            else
            {   // simple memcpy since elements are of 4 bytes size
                memcpy(mesh.indices.data(), indices, idxNum * 4);
            }

            static bool createMeshlets = true;
            uint32_t meshletsAmount = 0;
            if (createMeshlets)
            {
                meshletsAmount = CreateMeshlets(mesh);
            }
            else
            {
                debugMarkVertexUVs(mesh);
            }

            // Copy back the altered UVs for visual verification
            for (uint32_t vtx = 0, uvIdx = 0; vtx < vtxNum; ++vtx, uvIdx += 2)
            {
                GeneratorVertex& genVtx = mesh.vertices[vtx];

                texCoords[uvIdx] = genVtx.tx;
                texCoords[uvIdx + 1] = genVtx.ty;
            }

            return meshletsAmount;
        }

        uint32_t MeshletsModel::CreateMeshletsFromModelAsset(Data::Asset<RPI::ModelAsset> sourceModelAsset)
        {
            uint32_t meshletsAmount = 0;
            m_sourceModelAsset = sourceModelAsset;

            for (const Data::Asset<RPI::ModelLodAsset>& lodAsset : sourceModelAsset->GetLodAssets())
            {
                for (const RPI::ModelLodAsset::Mesh& meshAsset : lodAsset->GetMeshes())
                {
                    meshletsAmount += CreateMeshletsModel(meshAsset);
                }
            }

            AZ_Warning("Meshlets", false, "Meshlet model [%s] was created", m_name.c_str());

            return meshletsAmount;
        }

        MeshletsModel::MeshletsModel(Data::Asset<RPI::ModelAsset> sourceModelAsset)
        {
            IndicesSemanticName = Name{ "INDICES" };
            PositionSemanticName = Name{ "POSITION" };
            NormalSemanticName = Name{ "NORMAL" };
            TangentSemanticName = Name{ "TANGENT" };
            BiTangentSemanticName = Name{ "BITANGENT" };
            UVSemanticName = Name{ "UV" };


            MeshletsDescriptorsName = Name{ "MESHLETS" };
            MeshletsTrianglesName = Name{ "MESHLETS_TRIANGLES" };
            MeshletsIndicesLookupName = Name{ "MESHLETS_LOOKUP" };

            m_name = "Model_" + AZStd::to_string(s_modelNumber++);
            m_aabb = Aabb::CreateNull();

            m_meshletsAmount = CreateMeshletsFromModelAsset(sourceModelAsset);
        }

        MeshletsModel::~MeshletsModel()
        {
        }

    } // namespace Meshlets
} // namespace AZ
