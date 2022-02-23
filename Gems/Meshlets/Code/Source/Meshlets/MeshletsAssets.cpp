#pragma once

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

#include <MeshletsAssets.h>
#include <vector>

#pragma optimize("", off)

namespace AZ
{
    namespace Meshlets
    {
        //======================================================================
        //                            MeshletModel
        //======================================================================
        void debugMarkMeshletsUVs(
            GeneratorMesh& mesh,	// Results will alter the mesh here
            std::vector<meshopt_Meshlet>& meshlets,
            std::vector<unsigned int> meshlet_vertices,
            std::vector<unsigned char> meshlet_triangles
        )
        {
            for (uint32_t meshletId = 0; meshletId < meshlets.size(); ++meshletId)
            {
                meshopt_Meshlet& meshlet = meshlets[meshletId];
                float tx = (meshletId % 3) * 0.5f;
                float ty = ((meshletId / 3) % 3) * 0.5f;
                for (uint32_t triIdx = 0, triOffset = meshlet.triangle_offset; triIdx < meshlet.triangle_count; ++triIdx, triOffset += 3)
                {
                    for (uint32_t vtx = 0; vtx < 3; ++vtx)
                    {
                        unsigned char vtxIndirectIndex = meshlet_triangles[triOffset + vtx];
                        unsigned int vtxIndex = meshlet_vertices[meshlet.vertex_offset + vtxIndirectIndex];

                        mesh.vertices[vtxIndex].tx = tx;
                        mesh.vertices[vtxIndex].ty = ty;
                    }
                }
            }
        }

        void MeshletModel::CreateMeshlest(GeneratorMesh& mesh)
        {
            const size_t max_vertices = 64;
            const size_t max_triangles = 124; // NVidia-recommended 126, rounded down to a multiple of 4
            const float cone_weight = 0.5f;   // note: should be set to 0 unless cone culling is used at runtime!

            //----------------------------------------------------------------
            size_t max_meshlets = meshopt_buildMeshletsBound(mesh.indices.size(), max_vertices, max_triangles);
            std::vector<meshopt_Meshlet> meshlets(max_meshlets);
            std::vector<unsigned int> meshlet_vertices(max_meshlets * max_vertices);		// Vertex Index indirection map
            std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);	// Meshlet triangles into the vertex index indirection - local to meshlet.

            // NO scan seems to return more localized meshlets
            meshlets.resize(meshopt_buildMeshlets(
                &meshlets[0], &meshlet_vertices[0], &meshlet_triangles[0], &mesh.indices[0], mesh.indices.size(),
                &mesh.vertices[0].px, mesh.vertices.size(), sizeof(GeneratorVertex), max_vertices, max_triangles, cone_weight));

            if (!meshlets.size())
            {
                printf("Error generating meshlets - no meshlets were built\n");
                return;
            }

            // this is an example of how to trim the vertex/triangle arrays when copying data out to GPU storage
            const meshopt_Meshlet& last = meshlets.back();
            meshlet_vertices.resize(last.vertex_offset + last.vertex_count);
            meshlet_triangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
            //----------------------------------------------------------------

            // [Adi] - add this to display meshlet separation - debug purpose only
            static bool markTextureCoordinates = true;
            if (markTextureCoordinates)
            {
                debugMarkMeshletsUVs(mesh, meshlets, meshlet_vertices, meshlet_triangles);
            }

            AZ_Warning("Meshlets", false, "Successfully generated [%d] meshlets\n", (int)meshlets.size());
        }

        uint8_t* MeshletModel::RetrieveBufferData(
            const RPI::ModelLodAsset::Mesh& meshAsset,
            const Name& semanticName,
            RHI::Format& format,
            uint32_t expectedAmount, uint32_t &existingAmount )
        {
            const RPI::BufferAssetView* bufferView = nullptr;
            if (semanticName == IndicesSemanticName)
            {
                bufferView = &meshAsset.GetIndexBufferAssetView();
            }
            else
            {
                bufferView = meshAsset.GetSemanticBufferAssetView(semanticName);
            }
            const Data::Asset<RPI::BufferAsset>& bufferAsset = bufferView->GetBufferAsset();
            const RHI::BufferViewDescriptor& bufferDesc = bufferView->GetBufferViewDescriptor();
            existingAmount = bufferDesc.m_elementCount;

            if ((bufferDesc.m_elementOffset != 0) ||
                ((existingAmount != expectedAmount) && (expectedAmount != 0)))
            {
                AZ_Error("Meshlets", false, "More than a single mesh, or non-matching elements count");
                return nullptr;
            }
//          uint32_t elementSize = bufferDesc.m_elementSize;
            format = bufferDesc.m_elementFormat;
            AZStd::span<const uint8_t> bufferData = bufferAsset->GetBuffer();
            return (uint8_t *) bufferData.data();
        }

        bool MeshletModel::CreateMeshlets(
            float* positions, float* normals, float* texCoords, uint32_t vtxNum,
            uint16_t* indices, uint32_t idxNum, RHI::Format IndexStreamFormat)
        {
            GeneratorMesh mesh;

            // Filling the mesh data for the meshlet library
            mesh.vertices.resize(vtxNum);
            for (int vtx = 0, posIdx=0, uvIdx=0; vtx < vtxNum; ++vtx, posIdx+=3, uvIdx+=2)
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

            // The meshlets creation - meshlet data is not stored for now.
            CreateMeshlest(mesh);

            // Copy back the altered UVs for visual verification
            for (int vtx = 0, uvIdx = 0; vtx < vtxNum; ++vtx, uvIdx += 2)
            {
                GeneratorVertex& genVtx = mesh.vertices[vtx];

                texCoords[uvIdx] = genVtx.tx;
                texCoords[uvIdx + 1] = genVtx.ty;
            }
        }

        bool MeshletModel::CreateMeshletsFromModelAsset()
        {
            // setup a stream layout and shader input contract for the vertex streams
            RHI::Format IndexStreamFormat;// = RHI::Format::R32_UINT;
            RHI::Format PositionStreamFormat;// = RHI::Format::R32G32B32_FLOAT;
            RHI::Format NormalStreamFormat;// = RHI::Format::R32G32B32_FLOAT;
            RHI::Format TangentStreamFormat;// = RHI::Format::R32G32B32A32_FLOAT;
            RHI::Format BitangentStreamFormat;// = RHI::Format::R32G32B32_FLOAT;
            RHI::Format UVStreamFormat;// = RHI::Format::R32G32_FLOAT;

            for (const Data::Asset<RPI::ModelLodAsset>& lodAsset : m_atomModelAsset->GetLodAssets())
            {
                for (const RPI::ModelLodAsset::Mesh& meshAsset : lodAsset->GetMeshes())
                {
                    // Index buffer
                    const RPI::BufferAssetView& indexBufferView = meshAsset.GetIndexBufferAssetView();
                    uint32_t indexCount = 0;
                    uint8_t* indices = RetrieveBufferData(meshAsset, IndicesSemanticName, IndexStreamFormat, 0, indexCount);

                    // Vertex streams
                    uint32_t vertexCount = 0;
                    float* positions =
                        (float*) RetrieveBufferData(meshAsset, PositionSemanticName, PositionStreamFormat, 0, vertexCount);
                    float* normals = positions ?
                        (float*) RetrieveBufferData(meshAsset, NormalSemanticName, NormalStreamFormat, vertexCount, vertexCount) :
                        nullptr;
                    [[maybe_unused]] float* tangents = normals ?
                        (float*) RetrieveBufferData(meshAsset, TangentSemanticName, TangentStreamFormat, vertexCount, vertexCount) :
                        nullptr;
                    [[maybe_unused]] float* bitangents = normals ?
                        (float*) RetrieveBufferData(meshAsset, BiTangentSemanticName, BitangentStreamFormat, vertexCount, vertexCount) :
                        nullptr;
                    float* texCoords = normals ?
                        (float*) RetrieveBufferData(meshAsset, UVSemanticName, UVStreamFormat, vertexCount, vertexCount) :
                        nullptr;

                    CreateMeshlets(positions, normals, texCoords, vertexCount,
                        (uint16_t*) indices, indexCount, IndexStreamFormat );


                }
            }
            return true;
        }

        bool MeshletModel::CreateMeshletsFromModel()
        {
                        /*
            AZStd::span<const Data::Instance<ModelLod>> lods = m_atomModel->GetLods();

            for (Data::Instance<AZ::RPI::ModelLod>& currentLod : lods)
            {
                AZStd::vector<Mesh> meshes = currentLod->GetMeshes();
                for (Mesh& mesh : meshes)
                {
                    mesh.
                }
            }
            */
        }

        /*
        *

        Data::Asset<RPI::BufferAsset> MeshletModel::CreateBufferAsset(
            const AZStd::string& bufferName,
            const RHI::BufferViewDescriptor& bufferViewDescriptor,
            const void* data
        )
        {
            RPI::BufferAssetCreator creator;
            creator.Begin(Uuid::CreateRandom());

            RHI::BufferDescriptor bufferDescriptor;
            bufferDescriptor.m_bindFlags = RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::ShaderRead;
            bufferDescriptor.m_byteCount = static_cast<uint64_t>(bufferViewDescriptor.m_elementSize) * static_cast<uint64_t>(bufferViewDescriptor.m_elementCount);

            if (data)
            {
                creator.SetBuffer(data, bufferDescriptor.m_byteCount, bufferDescriptor);
            }

            creator.SetBufferViewDescriptor(bufferViewDescriptor);
            creator.SetUseCommonPool(RPI::CommonBufferPoolType::StaticInputAssembly);

            Data::Asset<RPI::BufferAsset> bufferAsset;

            // The next line is the actual buffer asset creation
            AZ_Error("Meshlets", creator.End(bufferAsset), "Error -- creating vertex stream %s", bufferName.c_str());

            return bufferAsset;
        }

        bool MeshletModel::CreateAtomModel()
        {
            // Each model gets a unique, random ID, so if the same source model is used for multiple instances, multiple target models will be created.
            RPI::ModelAssetCreator modelAssetCreator;
            Uuid modelId = Uuid::CreateRandom();
            modelAssetCreator.Begin(Uuid::CreateRandom());
            modelAssetCreator.SetName(m_name);

            {
                // Vertex Buffer Streams
                const auto positionDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount, RHI::Format::R32G32B32_FLOAT);
                const auto positionsAsset = CreateBufferAsset(m_vbStreamsDesc[UmbraVertexAttribute_Position].ptr, positionDesc, "UmbraModel_Positions");

                const auto uvDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount, RHI::Format::R32G32_FLOAT);
                const auto uvAsset = CreateBufferAsset(m_vbStreamsDesc[UmbraVertexAttribute_TextureCoordinate].ptr, uvDesc, "UmbraModel_UVs");

                const auto normalDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount, RHI::Format::R32G32B32_FLOAT);
                const auto normalsAsset = CreateBufferAsset(m_vbStreamsDesc[UmbraVertexAttribute_Normal].ptr, normalDesc, "UmbraModel_Normals");

                // Since we needed to add W component to the tangents and calculate the bi-tangents, the original
                // tangents buffer moved to the end and the bi-tangents took its place.
                const auto bitangentDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount, RHI::Format::R32G32B32_FLOAT);
                const auto bitangentsAsset = CreateBufferAsset(m_vbStreamsDesc[UmbraVertexAttribute_Tangent].ptr, bitangentDesc, "UmbraModel_BiTangents");

                // Tangents in Atom have 4 components - the W component represents R / L hand matrix
                const void* tangentsBufferPtr = ((uint8_t*)m_vbStreamsDesc[UmbraVertexAttribute_Tangent].ptr + (m_vertexCount * 3 * sizeof(float)));
                const auto tangentDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount, RHI::Format::R32G32B32A32_FLOAT);
                const auto tangentsAsset = CreateBufferAsset(tangentsBufferPtr, tangentDesc, "UmbraModel_Tangents");


                RHI::Format indicesFormat = (m_indexBytes == 2) ? RHI::Format::R16_UINT : RHI::Format::R32_UINT;
                const auto indexBufferViewDesc = RHI::BufferViewDescriptor::CreateTyped(0, m_indexCount, indicesFormat);
                const auto indicesAsset = CreateBufferAsset(m_ibDesc.ptr, indexBufferViewDesc, "UmbraModel_Indices");

                if (!positionsAsset || !normalsAsset || !tangentsAsset || !uvAsset || !indicesAsset)
                {
                    AZ_Error("AtomSceneStream", false, "Error -- creating model [%s] - buffer assets were not created successfully", m_name.c_str());
                    return false;
                }

                //--------------------------------------------
                // Creating the model LOD asset
                RPI::ModelLodAssetCreator modelLodAssetCreator;
                modelLodAssetCreator.Begin(Uuid::CreateRandom());

                modelLodAssetCreator.BeginMesh();
                {
                    {
                        modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "POSITION" }, PositionName, RPI::BufferAssetView{ positionsAsset, positionDesc });
                        modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "NORMAL" }, NormalName, RPI::BufferAssetView{ normalsAsset, normalDesc });
                        modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "TANGENT" }, TangentName, RPI::BufferAssetView{ tangentsAsset, tangentDesc });
                        modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "BITANGENT" }, BiTangentName, RPI::BufferAssetView{ bitangentsAsset, tangentDesc });
                        modelLodAssetCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ "UV" }, UVName, RPI::BufferAssetView{ uvAsset, uvDesc });
                        modelLodAssetCreator.SetMeshIndexBuffer({ indicesAsset, indexBufferViewDesc });

                        modelLodAssetCreator.SetMeshAabb(AZStd::move(m_aabb));
                        modelLodAssetCreator.SetMeshName(Name{ m_name.c_str() });
                    }
                }
                modelLodAssetCreator.EndMesh();

                // Create the model LOD based on the model LOD asset we created
                Data::Asset<RPI::ModelLodAsset> modelLodAsset;
                if (!modelLodAssetCreator.End(modelLodAsset))
                {
                    AZ_Error("AtomSceneStream", false, "Error -- creating model [%s] - ModelLoadAssetCreator.End() failed", m_name.c_str());
                    return false;
                }

                // Add the LOD model asset created to the model asset.
                modelAssetCreator.AddLodAsset(AZStd::move(modelLodAsset));
            }

            // Final stage - create the model based on the created assets
            Data::Asset<RPI::ModelAsset> modelAsset;

            if (!modelAssetCreator.End(modelAsset))
            {
                AZ_Error("AtomSceneStream", false, "Error -- creating model [%s] - model asset was not created", m_name.c_str());
                return false;
            }

            m_atomModel = RPI::Model::FindOrCreate(modelAsset);

            if (!m_atomModel)
            {
                AZ_Error("AtomSceneStream", false, "Error -- creating model [%s] - model could not be found or created", m_name.c_str());
                return false;
            }

            return true;
        }
        */

        MeshletModel::MeshletModel()
        {
            IndicesSemanticName = Name{ "INDICES" };
            PositionSemanticName = Name{ "POSITION" };
            NormalSemanticName = Name{ "NORMAL" };
            TangentSemanticName = Name{ "TANGENT" };
            BiTangentSemanticName = Name{ "BITANGENT" };
            UVSemanticName = Name{ "UV" };
//            IndicesName = Name{ "Indices" };
        }

        MeshletModel::~MeshletModel()
        {
        }

    } // namespace Meshlets
} // namespace AZ

#pragma optimize("", on)
