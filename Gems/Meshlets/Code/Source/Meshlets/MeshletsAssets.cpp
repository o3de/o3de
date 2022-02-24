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
        uint32_t MeshletModel::s_modelNumber = 0;

        //----------------------------------------------------------------------
        // Meshlets Generation
        //----------------------------------------------------------------------
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

        uint32_t MeshletModel::CreateMeshlest(GeneratorMesh& mesh)
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
                return 0;
            }

            // this is an example of how to trim the vertex/triangle arrays when copying data out to GPU storage
            const meshopt_Meshlet& last = meshlets.back();
            meshlet_vertices.resize(last.vertex_offset + last.vertex_count);
            meshlet_triangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
            uint32_t meshletsAmount = (uint32_t) meshlets.size();
            //----------------------------------------------------------------

            // [Adi] - add this to display meshlet separation - debug purpose only
            static bool markTextureCoordinates = true;
            if (markTextureCoordinates)
            {
                debugMarkMeshletsUVs(mesh, meshlets, meshlet_vertices, meshlet_triangles);
            }

            AZ_Warning("Meshlets", false, "Successfully generated [%d] meshlets\n", meshletsAmount);
            return meshletsAmount;
        }

        //----------------------------------------------------------------------
        // Enhanced (Meshlet) Model Creation 
        //----------------------------------------------------------------------
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

        bool MeshletModel::ProcessBuffersData(float* position, uint32_t vtxNum)
        {
            for (uint32_t vtx = 0; vtx < vtxNum; ++vtx, position += 3)
            {
                Vector3* positionV3 = (Vector3*)position;

                float length = positionV3->GetLength();
                const float maxVertexSize = 999.0f;
                if (length < maxVertexSize)
                {
                    m_aabb.AddPoint(*positionV3);
                }
                else
                {   
                    AZ_Warning("Meshlets", false, "Warning -- vertex [%d:%d] out of bound (%.2f, %.2f, %.2f) in model [%s]",
                        vtx, vtxNum, positionV3->GetX(), positionV3->GetY(), positionV3->GetZ(), m_name.c_str());
                }
            }

            AZ_Error("Meshlets", m_aabb.IsValid(), "Error --- Model [%s] AABB is invalid - all [%d] vertices are corrupted",
                m_name.c_str(), vtxNum);
            return m_aabb.IsValid() ? true : false;
        }

        uint32_t MeshletModel::CreateMeshletModel(const RPI::ModelLodAsset::Mesh& meshAsset)
        {
            //-------------------------------------------
            // The following section is creating a new model, however, this can be moved to
            // the outside calling method and have the new model contain several Lods and
            // not have 1:1 relation as it is currently
            RPI::ModelAssetCreator modelAssetCreator;
            Uuid modelId = Uuid::CreateRandom();
            modelAssetCreator.Begin(Uuid::CreateRandom());
            modelAssetCreator.SetName(m_name);
            //-------------------------------------------

            // setup a stream layout and shader input contract for the vertex streams
            RHI::Format IndexStreamFormat;// = RHI::Format::R32_UINT;
            RHI::Format PositionStreamFormat;// = RHI::Format::R32G32B32_FLOAT;
            RHI::Format NormalStreamFormat;// = RHI::Format::R32G32B32_FLOAT;
            RHI::Format TangentStreamFormat;// = RHI::Format::R32G32B32A32_FLOAT;
            RHI::Format BitangentStreamFormat;// = RHI::Format::R32G32B32_FLOAT;
            RHI::Format UVStreamFormat;// = RHI::Format::R32G32_FLOAT;
            RHI::BufferViewDescriptor bufferDescriptors[10];

            uint32_t meshletsAmount = 0;

            // Index buffer
            uint32_t indexCount = 0;
            const RPI::BufferAssetView* bufferView = &meshAsset.GetIndexBufferAssetView();
            uint8_t* indices = RetrieveBufferData(bufferView, IndexStreamFormat, 0, indexCount, bufferDescriptors[0]);
            const auto indicesAsset = CreateBufferAsset(IndicesSemanticName.GetStringView(), bufferDescriptors[0], indices);

            // Vertex streams
            uint32_t vertexCount = 0;
            bufferView = meshAsset.GetSemanticBufferAssetView(PositionSemanticName);
            float* positions =
                (float*)RetrieveBufferData(bufferView, PositionStreamFormat, 0, vertexCount, bufferDescriptors[1]);
            const auto positionsAsset = CreateBufferAsset(PositionSemanticName.GetStringView(), bufferDescriptors[1], positions);

            bufferView = meshAsset.GetSemanticBufferAssetView(NormalSemanticName);
            float* normals = positions ?
                (float*)RetrieveBufferData(bufferView, NormalStreamFormat, vertexCount, vertexCount, bufferDescriptors[2]) :
                nullptr;
            const auto normalsAsset = CreateBufferAsset(NormalSemanticName.GetStringView(), bufferDescriptors[2], normals);

            bufferView = meshAsset.GetSemanticBufferAssetView(UVSemanticName);
            float* texCoords = normals ?
                (float*)RetrieveBufferData(bufferView, UVStreamFormat, vertexCount, vertexCount, bufferDescriptors[3]) :
                nullptr;

            bufferView = meshAsset.GetSemanticBufferAssetView(TangentSemanticName);
            [[maybe_unused]] float* tangents = normals ?
                (float*)RetrieveBufferData(bufferView, TangentStreamFormat, vertexCount, vertexCount, bufferDescriptors[4]) :
                nullptr;
            const auto tangentsAsset = CreateBufferAsset(TangentSemanticName.GetStringView(), bufferDescriptors[4], tangents);

            bufferView = meshAsset.GetSemanticBufferAssetView(BiTangentSemanticName);
            [[maybe_unused]] float* bitangents = normals ?
                (float*)RetrieveBufferData(bufferView, BitangentStreamFormat, vertexCount, vertexCount, bufferDescriptors[5]) :
                nullptr;
            const auto bitangentsAsset = CreateBufferAsset(BiTangentSemanticName.GetStringView(), bufferDescriptors[5], bitangents);

            // At this point we create the meshlets and mark the UVs based on meshlet index
            meshletsAmount += CreateMeshlets(positions, normals, texCoords, vertexCount,
                (uint16_t*)indices, indexCount, IndexStreamFormat);

            // This is done here since we update the data to indicate meshlets
            const auto texCoordsAsset = CreateBufferAsset(UVSemanticName.GetStringView(), bufferDescriptors[3], texCoords);

            // Model LOD Creation
            {
                if (!positionsAsset || !normalsAsset || !tangentsAsset || !texCoordsAsset || !indicesAsset)
                {
                    AZ_Error("Meshlets", false, "Failed creating model [%s] - buffer assets were not created successfully", m_name.c_str());
                    return false;
                }

                //--------------------------------------------
                RPI::ModelLodAssetCreator modelLodAssetCreator;
                modelLodAssetCreator.Begin(Uuid::CreateRandom());

                modelLodAssetCreator.BeginMesh();
                {
                    {
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

                        // The following is crucial for the AABB generation.
                        ProcessBuffersData(positions, vertexCount);

                        modelLodAssetCreator.SetMeshAabb(AZStd::move(m_aabb));
                        modelLodAssetCreator.SetMeshName(Name{ m_name.c_str() });
                    }
                }
                modelLodAssetCreator.EndMesh();

                // Create the model LOD based on the model LOD asset we created
                Data::Asset<RPI::ModelLodAsset> modelLodAsset;
                if (!modelLodAssetCreator.End(modelLodAsset))
                {
                    AZ_Error("Meshlets", false, "Error -- creating model [%s] - ModelLoadAssetCreator.End() failed",
                        m_name.c_str());
                    return false;
                }

                // Add the LOD model asset created to the model asset.
                modelAssetCreator.AddLodAsset(AZStd::move(modelLodAsset));
            }

            //-------------------------------------------
            // Final stage - create the model based on the created assets
            Data::Asset<RPI::ModelAsset> modelAsset;

            if (!modelAssetCreator.End(modelAsset))
            {
                AZ_Error("Meshlets", false, "Error -- creating model [%s] - model asset was not created", m_name.c_str());
                return false;
            }

            m_atomModel = RPI::Model::FindOrCreate(modelAsset);
            //-------------------------------------------

            if (!m_atomModel)
            {
                AZ_Error("Meshlets", false, "Error -- creating model [%s] - model could not be found or created", m_name.c_str());
                return false;
            }

            return true;
        }


        //----------------------------------------------------------------------
        // Model Traversal and Data Copy for Creation 
        //----------------------------------------------------------------------
        uint8_t* MeshletModel::RetrieveBufferData(
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
//          uint32_t elementSize = bufferDesc.m_elementSize;
            format = bufferDesc.m_elementFormat;
            AZStd::span<const uint8_t> bufferData = bufferAsset->GetBuffer();
            return (uint8_t *) bufferData.data();
        }

        uint32_t MeshletModel::CreateMeshlets(
            float* positions, float* normals, float* texCoords, uint32_t vtxNum,
            uint16_t* indices, uint32_t idxNum, RHI::Format IndexStreamFormat)
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

            // The meshlets creation - meshlet data is not stored for now.
            uint32_t meshletsAmount = CreateMeshlest(mesh);

            // Copy back the altered UVs for visual verification
            for (uint32_t vtx = 0, uvIdx = 0; vtx < vtxNum; ++vtx, uvIdx += 2)
            {
                GeneratorVertex& genVtx = mesh.vertices[vtx];

                texCoords[uvIdx] = genVtx.tx;
                texCoords[uvIdx + 1] = genVtx.ty;
            }

            return meshletsAmount;
        }

        uint32_t MeshletModel::CreateMeshletsFromModelAsset()
        {
            uint32_t meshletsAmount = 0;

            for (const Data::Asset<RPI::ModelLodAsset>& lodAsset : m_atomModelAsset->GetLodAssets())
            {
                for (const RPI::ModelLodAsset::Mesh& meshAsset : lodAsset->GetMeshes())
                {
                    meshletsAmount += CreateMeshletModel(meshAsset);
                }
            }
            return meshletsAmount;
        }

        uint32_t MeshletModel::CreateMeshletsFromModel()
        {                     
            for (uint32_t lodIdx = 0; lodIdx < m_atomModel->GetLodCount(); ++lodIdx)
            {
                const Data::Instance<RPI::ModelLod>& currentLod = m_atomModel->GetLods()[lodIdx];

                for (uint32_t meshIdx = 0; meshIdx < currentLod->GetMeshes().size(); ++meshIdx)
                {
//                    const RPI::ModelLod::Mesh& mesh = currentLod->GetMeshes()[meshIdx];
                    // The next is TBD - harder to get than from the reflected Asset part
                }
            }
            return 0;
        }

        MeshletModel::MeshletModel()
        {
            IndicesSemanticName = Name{ "INDICES" };
            PositionSemanticName = Name{ "POSITION" };
            NormalSemanticName = Name{ "NORMAL" };
            TangentSemanticName = Name{ "TANGENT" };
            BiTangentSemanticName = Name{ "BITANGENT" };
            UVSemanticName = Name{ "UV" };

            m_name = "Model_" + AZStd::to_string(s_modelNumber++);
            m_aabb.CreateNull();
        }

        MeshletModel::~MeshletModel()
        {
        }

    } // namespace Meshlets
} // namespace AZ

#pragma optimize("", on)
