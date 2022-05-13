#pragma once

#include <AzCore/Math/Aabb.h>

#include <Atom/RHI/Factory.h>

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

#include <MeshletsFeatureProcessor.h>
#include <MeshletsUtilities.h>
#include <MeshletsRenderObject.h>

#pragma optimize("", off)

namespace AZ
{
    namespace Meshlets
    {
        //======================================================================
        //                        MeshletsRenderObject
        //======================================================================
        uint32_t MeshletsRenderObject::s_modelNumber = 0;

        bool MeshletsRenderObject::SetShaders()
        {
            {
                m_computeShader = m_featureProcessor->GetComputeShader();
                if (!m_computeShader)
                {
                    AZ_Error("Meshlets", false, "Failed to get Compute shader");
                    return false;
                }

                m_renderShader = m_featureProcessor->GetRenderShader();
                if (!m_renderShader)
                {
                    AZ_Error("Meshlets", false, "Failed to get Render shader");
                    return false;
                }
            }
            return true;
        }

        //! This method will generate the meshlets and store their data in 'm_meshletsData' 
        uint32_t MeshletsRenderObject::CreateMeshlets(GeneratorMesh& mesh)
        {
            const size_t max_vertices = Meshlets::maxVerticesPerMeshlet;    // matching wave/warp groups size multiplier
            const size_t max_triangles = Meshlets::maxTrianglesPerMeshlet;  // NVidia-recommended 126, rounded down to a multiple of 4
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
            uint32_t meshletsCount = (uint32_t) m_meshletsData.meshlets.size();
            //----------------------------------------------------------------
            */

            ////////////////////////////
            std::vector<meshopt_Meshlet> meshlets(max_meshlets);
            std::vector<unsigned int> meshlet_vertices(max_meshlets * max_vertices);		// Vertex Index indirection map
            std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);	// Meshlet triangles into the vertex index indirection - local to meshlet.

            // NO scan seems to return more localized meshlets
            meshlets.resize(meshopt_buildMeshlets(
                &meshlets[0], &meshlet_vertices[0], &meshlet_triangles[0], &mesh.indices[0], mesh.indices.size(),
                &mesh.vertices[0].px, mesh.vertices.size(), sizeof(GeneratorVertex), max_vertices, max_triangles, cone_weight));

            // this is an example of how to trim the vertex/triangle arrays when copying data out to GPU storage
            const meshopt_Meshlet& last = meshlets.back();
            meshlet_vertices.resize(last.vertex_offset + last.vertex_count);
            meshlet_triangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
            uint32_t meshletsCount = (uint32_t) meshlets.size();

            m_meshletsData.Descriptors = meshlets;
            m_meshletsData.IndicesIndirection = meshlet_vertices;
            m_meshletsData.EncodeTrianglesData(meshlet_triangles);
            m_meshletsData.ValidateData((uint32_t)meshlet_vertices.size());
            ////////////////////////////

            AZ_Warning("Meshlets", false, "Successfully generated [%d] meshlets\n", meshletsCount);
            return meshletsCount;
        }

        uint32_t MeshletsRenderObject::CreateMeshlets(
            float* positions, float* normals, float* texCoords, uint32_t vtxNum,
            uint16_t* indices, uint32_t idxNum, RHI::Format IndexStreamFormat
        )
        {
            GeneratorMesh mesh;

            // Filling the mesh data for the meshlet library
            mesh.vertices.resize(vtxNum);
            for (uint32_t vtx = 0, posIdx = 0, uvIdx = 0; vtx < vtxNum; ++vtx, posIdx += 3, uvIdx += 2)
            {
                GeneratorVertex& genVtx = mesh.vertices[vtx];
                genVtx.px = positions[posIdx];
                genVtx.py = positions[posIdx + 1];
                genVtx.pz = positions[posIdx + 2];
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
            uint32_t meshletsCount = 0;
            if (createMeshlets)
            {
                meshletsCount = CreateMeshlets(mesh);
            }

            return meshletsCount;
        }

        bool MeshletsRenderObject::ProcessBuffersData(float* position, uint32_t vtxNum)
        {
            for (uint32_t vtx = 0; vtx < vtxNum; ++vtx, position += 3)
            {
                Vector3* positionV3 = (Vector3*)position;

                float length = positionV3->GetLength();
                const float maxVertexSize = 99.0f;
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

        void MeshletsRenderObject::PrepareComputeSrgDescriptors(
            MeshRenderData &meshRenderData, 
            uint32_t vertexCount, uint32_t indexCount)
        {
            if (meshRenderData.m_computeBuffersDescriptors.size())
            {
                return;
            }

            meshRenderData.m_computeBuffersDescriptors.resize(uint8_t(ComputeStreamsSemantics::NumBufferStreams));

            meshRenderData.m_computeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::Indices)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite,
                    RHI::Format::R32_UINT,
                    RHI::BufferBindFlags::Indirect | RHI::BufferBindFlags::ShaderReadWrite,
                    sizeof(uint32_t), indexCount,
                    Name{ "INDICES" }, Name{ "m_Indices" }, 0, 0
                );

            meshRenderData.m_computeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::UVs)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite,
                    RHI::Format::R32G32_UINT,
                    RHI::BufferBindFlags::Indirect | RHI::BufferBindFlags::ShaderReadWrite,
                    sizeof(uint32_t) * 2, vertexCount,
                    Name{ "UV" }, Name{ "m_TextureCoords" }, 1, 0
                );

            meshRenderData.m_computeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::MeshletsData)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite,
//                    RHI::Format::R32G32B32A32_UINT,
                    RHI::Format::Unknown,   // Mark is as Unknown since it represents StructuredBuffer
                    RHI::BufferBindFlags::Indirect | RHI::BufferBindFlags::ShaderRead,
//                    sizeof(uint32_t) * 4, meshletsCount,
                    sizeof(MeshletDescriptor), (uint32_t)m_meshletsData.Descriptors.size(), 
                    Name{ "MESHLETS" }, Name{ "m_MeshletsDescriptors" }, 2, 0,
                    (uint8_t*)m_meshletsData.Descriptors.data()
                );

            meshRenderData.m_computeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::MehsletsTriangles)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite,
                    RHI::Format::R32_UINT,
                    RHI::BufferBindFlags::Indirect | RHI::BufferBindFlags::ShaderRead,
                    sizeof(uint32_t), (uint32_t)m_meshletsData.EncodedTriangles.size(),
                    Name{ "MESHLETS_TRIANGLES" }, Name{ "m_MeshletsTriangles" }, 3, 0,
                    (uint8_t*)m_meshletsData.EncodedTriangles.data()
                );

            meshRenderData.m_computeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::MeshletsIndicesIndirection)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite,
                    RHI::Format::R32_UINT,
                    RHI::BufferBindFlags::Indirect | RHI::BufferBindFlags::ShaderRead,
                    sizeof(uint32_t), (uint32_t)m_meshletsData.IndicesIndirection.size(),
                    Name{ "MESHLETS_LOOKUP" }, Name{ "m_MeshletsIndicesLookup" }, 4, 0,
                    (uint8_t*)m_meshletsData.IndicesIndirection.data()
                );
        }

        void MeshletsRenderObject::PrepareRenderSrgDescriptors(
            MeshRenderData &meshRenderData, uint32_t vertexCount, uint32_t indicesCount)
        {
            if (meshRenderData.m_renderBuffersDescriptors.size())
            {
                return;
            }

            meshRenderData.m_renderBuffersDescriptors.resize(uint8_t(RenderStreamsSemantics::NumBufferStreams));

            meshRenderData.m_renderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Indices)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite,
                    RHI::Format::R32_UINT,
                    RHI::BufferBindFlags::InputAssembly, // RHI::BufferBindFlags::Indirect | RHI::BufferBindFlags::ShaderRead,
                    sizeof(uint32_t), indicesCount,
                    Name{ "INDICES" }, Name{ "m_Indices" }, 3, 0
                );

            meshRenderData.m_renderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Positions)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::StaticInputAssembly,
                    RHI::Format::R32G32B32_FLOAT,
                    RHI::BufferBindFlags::InputAssembly, // RHI::BufferBindFlags::Indirect | RHI::BufferBindFlags::ShaderRead,
                    sizeof(float) * 3, vertexCount,
                    Name{ "POSITION" }, Name{ "m_Positions" }, 0, 0
                );

            meshRenderData.m_renderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Normals)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::StaticInputAssembly,
                    RHI::Format::R32G32B32_FLOAT,
                    RHI::BufferBindFlags::InputAssembly, // RHI::BufferBindFlags::Indirect | RHI::BufferBindFlags::ShaderRead,
                    sizeof(float) * 3, vertexCount,
                    Name{ "NORMAL" }, Name{ "m_Normals" }, 1, 0
                );

            meshRenderData.m_renderBuffersDescriptors[uint8_t(RenderStreamsSemantics::UVs)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite,
                    RHI::Format::R32G32_FLOAT,
                    RHI::BufferBindFlags::InputAssembly, // RHI::BufferBindFlags::Indirect | RHI::BufferBindFlags::ShaderRead,
                    sizeof(uint32_t) * 2, vertexCount,
                    Name{ "UV" }, Name{ "m_TextureCoords" }, 2, 0
                );

            meshRenderData.m_renderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Tangents)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::StaticInputAssembly,
                    RHI::Format::R32G32B32A32_FLOAT,
                    RHI::BufferBindFlags::InputAssembly, // RHI::BufferBindFlags::Indirect | RHI::BufferBindFlags::ShaderRead,
                    sizeof(float) * 4, vertexCount,
                    Name{ "TANGENT" }, Name{ "m_Tangents" }, 3, 0
                );

            meshRenderData.m_renderBuffersDescriptors[uint8_t(RenderStreamsSemantics::BiTangents)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::StaticInputAssembly,
                    RHI::Format::R32G32B32_FLOAT,
                    RHI::BufferBindFlags::InputAssembly, // RHI::BufferBindFlags::Indirect | RHI::BufferBindFlags::ShaderRead,
                    sizeof(float) * 3, vertexCount,
                    Name{ "BITANGENT" }, Name{ "m_BiTangents" }, 4, 0
                );
        }

        //==============================================================================
        // The following two methods are currently not used. The srgs are created prior
        // to the buffers creation and they are bound to after the creation, finishing
        // with Srg compilation.
        // 
        //! There are two methods to use the shared buffer:
        //! 1. Use a the same buffer with pass sync point and use offset to the data
        //!     structures inside. The problem there is offset overhead + complex conversions
        //! 2. Use buffer views into the original shared buffer and treat them as buffers
        //!     with the desired data type. In this case Atom still requires single shared buffer
        //!     usage within the shader in order to support the sync point. 
        // In Atom the usage of BufferView is what permits the usage of different 'buffers'
        //  allocated from within the originally bound single buffer.
        // This allows us to have a single sync point (barrier) between passes only for this
        //  buffer, while indirectly it is used as multiple buffers used by multiple objects in
        //  this pass. 
        bool MeshletsRenderObject::CreateAndBindComputeSrg(MeshRenderData &meshRenderData)
        {
            meshRenderData.m_computeSrg = RPI::ShaderResourceGroup::Create(m_computeShader->GetAsset(), AZ::Name{ "MeshletsDataSrg" });
            if (!meshRenderData.m_computeSrg)
            {
                AZ_Error("Meshlets", false, "Failed to create the Compute Srg");
                return false;
            }

            for (uint8_t stream = 0; stream < uint8_t(ComputeStreamsSemantics::NumBufferStreams); ++stream)
            {
                SrgBufferDescriptor& bufferDesc = meshRenderData.m_computeBuffersDescriptors[stream];
                RHI::ShaderInputBufferIndex indexHandle = meshRenderData.m_computeSrg->FindShaderInputBufferIndex(bufferDesc.m_paramNameInSrg);
                bufferDesc.m_resourceShaderIndex = indexHandle.GetIndex();

                if (!meshRenderData.m_computeSrg->SetBufferView(indexHandle, meshRenderData.m_computeBuffersViews[stream].get()))
                {
                    AZ_Error("Meshlets", false, "Failed to bind compute buffer view for %s", bufferDesc.m_bufferName.GetCStr());
//                    return false;
                }
            }

            // Now that all data was created and bound, we can compile the Srg 
            meshRenderData.m_computeSrg->Compile();

            return true;
        }

        //==================================================================
        //                              NOT USED
        // No Render Pass Srgs require the generation and attachments to buffers
        // currently as we use them as an assembly streams for the vertex shader.
        // This should be changed if they are used as buffers and in this case
        // the Srg should be constructed as per object Srg.
        //==================================================================
        bool MeshletsRenderObject::CreateAndBindPerObjectRenderSrg(MeshRenderData &meshRenderData)
        {
            AZ_Assert(false, "render Srg should not be created as buffers are used directly as streams in the shader. ");

            if (!m_renderShader)
            {
                AZ_Error("Meshlets", false, "Render shader was not set yet");
                return false;
            }

            meshRenderData.m_renderObjectSrg = RPI::ShaderResourceGroup::Create(m_renderShader->GetAsset(), AZ::Name{ "RenderSrg" });
            if (!meshRenderData.m_renderObjectSrg)
            {
                AZ_Error("Meshlets", false, "Failed to create the render Srg");
                return false;
            }

            for (uint8_t stream = 0; stream < uint8_t(RenderStreamsSemantics::NumBufferStreams); ++stream)
            {
                if (stream == uint8_t(RenderStreamsSemantics::Indices))
                {   // Skip the index buffer - it is not passed as a buffer
                    continue;
                }

                SrgBufferDescriptor& bufferDesc = meshRenderData.m_renderBuffersDescriptors[stream];
                RHI::ShaderInputBufferIndex indexHandle = meshRenderData.m_renderObjectSrg->FindShaderInputBufferIndex(bufferDesc.m_paramNameInSrg);
                bufferDesc.m_resourceShaderIndex = indexHandle.GetIndex();

                if (!meshRenderData.m_renderObjectSrg->SetBufferView(indexHandle, meshRenderData.m_renderBuffersViews[stream].get()))
                {
                    AZ_Error("Meshlets", false, "Failed to bind render buffer view for %s", bufferDesc.m_bufferName.GetCStr());
                    return false;
                }
            }

            // Final stage - compile the Srg
            meshRenderData.m_renderObjectSrg->Compile();

            return true;
        }
        //==============================================================================

        bool MeshletsRenderObject::CreateRenderBuffers(MeshRenderData &meshRenderData)
        {
            // Notice that in the mapping here there is strong assumption that the order of entries
            // is according to the order in ComputeStreamsSemantics.
            uint8_t mappingToCompute[2] = {
                uint8_t(ComputeStreamsSemantics::Indices),
                uint8_t(ComputeStreamsSemantics::UVs)
            };

            // Create the Srg first - required for the buffers generation
            if (m_renderShader)
            {
                meshRenderData.m_renderObjectSrg = RPI::ShaderResourceGroup::Create(m_renderShader->GetAsset(), AZ::Name{ "MeshletsRenderSrg" });
            }
            if (!meshRenderData.m_renderObjectSrg)
            {
                AZ_Error("Meshlets", false, "Failed to create the Render Srg");
                // [Adi] - for now don't return if no srg
//                return false;
            }

            bool success = true;
            uint32_t streamsNum = (uint32_t) meshRenderData.m_renderBuffersDescriptors.size();
//            meshRenderData.m_renderBuffersAllocators.resize(streamsNum);
            meshRenderData.m_renderBuffersViews.resize(streamsNum);   // resize for a vector, no need for an array

            // Unlike the compute method - the render method actually creates all buffers including
            // the shared indices and UVs buffers that are used by the Compute prior to rendering.
            for (uint32_t stream = 0; stream < streamsNum ; ++stream)
            {
                SrgBufferDescriptor& bufferDesc = meshRenderData.m_renderBuffersDescriptors[stream];

                if ((stream == uint8_t(RenderStreamsSemantics::UVs)) ||
                    (stream == uint8_t(RenderStreamsSemantics::Indices))) 
                {   
                    // Shared Buffer Views: allocate views from the shared buffer since all buffers will
                    // share the same state and be shader read/write - in this case the buffers were created
                    // by CreateAndBindRenderBuffers so we need to copy the data from there.
                    uint8_t mappedIdx = mappingToCompute[stream];
//                    meshRenderData.m_renderBuffersAllocators[stream] = meshRenderData.m_computeBuffersAllocators[mappedIdx];

                    if (stream == uint8_t(RenderStreamsSemantics::UVs))
                    {
                        meshRenderData.m_renderBuffersViews[stream] = meshRenderData.m_computeBuffersViews[mappedIdx];
                    }
                    else if (stream == uint8_t(RenderStreamsSemantics::Indices))
                    {
                        meshRenderData.m_indexBufferView = RHI::IndexBufferView(
                            meshRenderData.m_computeBuffersViews[mappedIdx]->GetBuffer(),
                            bufferDesc.m_viewOffsetInBytes,
                            (uint64_t)bufferDesc.m_elementCount * bufferDesc.m_elementSize,
                            (bufferDesc.m_elementFormat == RHI::Format::R32_UINT) ? RHI::IndexFormat::Uint32 : RHI::IndexFormat::Uint16);
                    }
                }
                else
                {   // Regular buffers creation - since they are read only, no need to use the shared buffer
                    // No need to fill in the buffer views as the Srg was already bound via the buffer
                    meshRenderData.m_renderBuffersViews[stream] = Data::Instance<RHI::BufferView>();
                    meshRenderData.m_renderBuffers[stream] =
                        UtilityClass::CreateBuffer("Meshlets", bufferDesc, nullptr);    // buffers are not tied to srg

                    success &= (meshRenderData.m_renderBuffers[stream] ? true : false);
                }
            }

            // Copy the original mesh data into the buffers or delete the allocators if failed
            if (success)
            {
                for (uint32_t stream = 0; stream < streamsNum && success ; ++stream)
                {   // upload the original streams data - indices copy is not required
                    if ((stream == uint8_t(ComputeStreamsSemantics::UVs)))// ||
//                        (stream == uint8_t(ComputeStreamsSemantics::Indices)))
                    {
                        continue;
                    }

                    Data::Instance<RPI::Buffer> buffer = meshRenderData.m_renderBuffers[stream] ?
                        meshRenderData.m_renderBuffers[stream] : 
                        Meshlets::SharedBufferInterface::Get()->GetBuffer();

                    SrgBufferDescriptor& bufferDesc = meshRenderData.m_renderBuffersDescriptors[stream];
                    size_t requiredSize = (uint64_t)bufferDesc.m_elementCount * bufferDesc.m_elementSize;
                    bool upload = buffer->UpdateData( bufferDesc.m_bufferData, requiredSize, bufferDesc.m_viewOffsetInBytes);
                    AZ_Error("Meshlets", upload, "Data could not be uploaded to Render buffer [%s]", bufferDesc.m_bufferName.GetCStr());
                    success &= upload;
                }
            }

            return success;
        }

        bool MeshletsRenderObject::CreateAndBindComputeBuffers(MeshRenderData &meshRenderData)
        {
            // Start with the Srg creation - it will be required for the buffers generation
            meshRenderData.m_computeSrg = RPI::ShaderResourceGroup::Create(m_computeShader->GetAsset(), AZ::Name{ "MeshletsDataSrg" });
            if (!meshRenderData.m_computeSrg)
            {
                AZ_Error("Meshlets", false, "Failed to create the Compute Srg");
                return false;
            }

            bool success = true;
            uint32_t streamsNum = (uint32_t)meshRenderData.m_computeBuffersDescriptors.size();
            meshRenderData.m_computeBuffersAllocators.resize(streamsNum);
            meshRenderData.m_computeBuffersViews.resize(streamsNum);
            meshRenderData.m_computeBuffers.resize(streamsNum);

            for (uint32_t stream = 0; stream < streamsNum ; ++stream)
            {
                SrgBufferDescriptor& bufferDesc = meshRenderData.m_computeBuffersDescriptors[stream];
                
                if ((stream == uint8_t(ComputeStreamsSemantics::UVs)) ||
                    (stream == uint8_t(ComputeStreamsSemantics::Indices)))
                {   // Shared Buffer Views: allocate views from the shared buffer since Index and UV buffers will
                    // share the same state and be shader read/write.
                    meshRenderData.m_computeBuffersViews[stream] = UtilityClass::CreateSharedBufferViewAndBindToSrg(
                        "Meshlets", bufferDesc, meshRenderData.m_computeBuffersAllocators[stream], meshRenderData.m_computeSrg);
                }
                else
                {   // Regular buffers: since these buffers are read only and will not be altered there is no need to
                    // use the shared buffer. This also means that we bind using buffers instead of buffers views.
                    meshRenderData.m_computeBuffersViews[stream] = Data::Instance<RHI::BufferView>();
                    meshRenderData.m_computeBuffers[stream] = UtilityClass::CreateBufferAndBindToSrg(
                        "Meshlets", bufferDesc, meshRenderData.m_computeSrg);

                    success &= (meshRenderData.m_computeBuffers[stream] ? true : false);
                }
            }

            // Copy the original mesh data into the buffers or delete the allocators if failed
            if (success)
            {
                for (uint32_t stream = 0; stream < streamsNum ; ++stream)
                {   // upload the original streams data.
                    // Avoid this for indices and UVs in order to test the compute stage output
                    if ((stream == uint8_t(ComputeStreamsSemantics::UVs)) ||
                        (stream == uint8_t(ComputeStreamsSemantics::Indices)))
                    {
                        continue;
                    }

                    SrgBufferDescriptor& bufferDesc = meshRenderData.m_computeBuffersDescriptors[stream];
                    size_t requiredSize = (uint64_t)bufferDesc.m_elementCount * bufferDesc.m_elementSize;
                    bool upload = meshRenderData.m_computeBuffers[stream]->UpdateData(bufferDesc.m_bufferData, requiredSize, bufferDesc.m_viewOffsetInBytes);
                    AZ_Error("Meshlets", success == upload, "Data could not be uploaded to Compute buffer [%s]", bufferDesc.m_bufferName.GetCStr());
                    success &= upload;
                }
            }

            // Now that all data was created and bound, we can compile the Srg 
            meshRenderData.m_computeSrg->Compile();

            return success;
        }

        bool MeshletsRenderObject::RetrieveSourceMeshData(
            const RPI::ModelLodAsset::Mesh& meshAsset,
            MeshRenderData& meshRenderData, 
            uint32_t vertexCount, uint32_t indexCount)
        {
            RHI::BufferViewDescriptor bufferDescriptor;
            RHI::Format streamFormat;

            // Take care of the indices since it's stored differently than the rest of the streams
            SrgBufferDescriptor* descriptor = &meshRenderData.m_renderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Indices)];
            const RPI::BufferAssetView* bufferAssetView = &meshAsset.GetIndexBufferAssetView();
            descriptor->m_bufferData = RetrieveBufferData(bufferAssetView, streamFormat, 0, indexCount, bufferDescriptor);

            bool success = (streamFormat == descriptor->m_elementFormat);
            AZ_Error("Meshlets", success, "Error - index buffer with different format [%d]", streamFormat);

            // Now take care of the rest of the streams
            for (uint8_t stream = 0; stream < uint8_t(RenderStreamsSemantics::NumBufferStreams); ++stream)
            {
                if (stream == uint8_t(RenderStreamsSemantics::Indices))
                {
                    continue;
                }

                descriptor = &meshRenderData.m_renderBuffersDescriptors[stream];
                bufferAssetView = meshAsset.GetSemanticBufferAssetView(descriptor->m_bufferName);
                descriptor->m_bufferData = RetrieveBufferData(bufferAssetView, streamFormat, vertexCount, vertexCount, bufferDescriptor);

                if (streamFormat != descriptor->m_elementFormat)
                {
                    AZ_Error("Meshlets", false,
                        "Error - buffer %s with different format [%d]", descriptor->m_bufferName.GetCStr(), streamFormat);
                    success = false;
                }

                if (!descriptor->m_bufferData)
                {
                    AZ_Error("Meshlets", false, "Failed to create meshlet model [%s] - buffer [%s] data could not be retrieved",
                        descriptor->m_bufferName.GetCStr());
                    return false;
                }
            }

            // AABB generation - can also be used for vertices scaling / creating transform.
            success &= ProcessBuffersData((float*)meshRenderData.m_renderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Positions)].m_bufferData, vertexCount);

            return success;
        }

        //----------------------------------------------------------------------
        // The following method populates the new meshlets model from the given meshLodAsset.
        // Moving the creator outside calling method and have the new model contain several
        // Lods did not work as it needs to be local to the block!
        uint32_t MeshletsRenderObject::CreateMeshletsRenderObject(
            const RPI::ModelLodAsset::Mesh& meshAsset,
            MeshRenderData &meshRenderData)
        {
            uint32_t indexCount = meshAsset.GetIndexCount();
            uint32_t vertexCount = meshAsset.GetVertexCount();

            // Prepare the rendering descriptor required next
            PrepareRenderSrgDescriptors(meshRenderData, vertexCount, indexCount);

            if (!RetrieveSourceMeshData(meshAsset, meshRenderData, vertexCount, indexCount))
            {
                return 0;
            }

            // Now we start generating the meshlets data.
            uint32_t meshletsCount = CreateMeshlets(
                (float*)meshRenderData.m_renderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Positions)].m_bufferData,
                (float*)meshRenderData.m_renderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Normals)].m_bufferData,
                (float*)meshRenderData.m_renderBuffersDescriptors[uint8_t(RenderStreamsSemantics::UVs)].m_bufferData,
                vertexCount,
                (uint16_t*) meshRenderData.m_renderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Indices)].m_bufferData,
                indexCount,
                meshRenderData.m_renderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Indices)].m_elementFormat
                );

            if (!meshletsCount)
            {
                AZ_Error("Meshlets", false, "Failed to create meshlet model [%s] - the meshlet creation process failed", m_name.c_str());
                return 0;
            }

            //----------------------------------------------------------------
            // Prepare the Compute buffers, views and Srg for the Compute pass.
            PrepareComputeSrgDescriptors(meshRenderData, vertexCount, indexCount);
            if (!CreateAndBindComputeBuffers(meshRenderData))
            {
                 return 0;
            }

            meshRenderData.m_meshletsAmount = meshletsCount;
            meshRenderData.m_dispatchItem.InitDispatch(
                m_computeShader.get(), meshRenderData.m_computeSrg, meshRenderData.m_meshletsAmount);

            //----------------------------------------------------------------
            // Create the render streams and bind the the render Srg for the Render pass.
            if (!CreateRenderBuffers(meshRenderData) ||
                !CreateAndBindPerObjectRenderSrg(meshRenderData))
            {
                return 0;
            }

            return meshletsCount;
        }


        //----------------------------------------------------------------------
        // Model Traversal and Data Copy for Creation 
        //----------------------------------------------------------------------
        uint8_t* MeshletsRenderObject::RetrieveBufferData(
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

        // [Adi] - currently we create only the first mesh of the first Lod to be able to
        // get to a fully working POC.
        // Enhancing this by doing a double pass, gathering all data and creating meshlets groups
        // by Lod level should not be a problem but a design of meshlet model structure should be
        // put in place before doing that.
        uint32_t MeshletsRenderObject::CreateMeshletsFromModelAsset(Data::Asset<RPI::ModelAsset> sourceModelAsset)
        {
            uint32_t meshletsCount = 0;

            m_modelRenderData.resize(sourceModelAsset->GetLodAssets().size());
            uint32_t curLod = 0;
            uint32_t lodAssetsAmount = uint32_t(sourceModelAsset->GetLodAssets().size());

//            for (const Data::Asset<RPI::ModelLodAsset>& lodAsset : sourceModelAsset->GetLodAssets())
            for (const Data::Asset<RPI::ModelLodAsset>& lodAsset = sourceModelAsset->GetLodAssets()[curLod] ;
                curLod < lodAssetsAmount ; ++curLod )
            {
                ModelLodDataArray* lodRenderData = &m_modelRenderData[curLod];
                uint32_t meshCount = uint32_t(lodAsset->GetMeshes().size());
                lodRenderData->resize(meshCount);

//                for (const RPI::ModelLodAsset::Mesh& meshAsset : lodAsset->GetMeshes())
                for (uint32_t meshIdx=0 ; meshIdx<meshCount ; ++meshIdx)
                {
                    MeshRenderData* meshRenderData = new MeshRenderData;
                    meshletsCount += CreateMeshletsRenderObject(lodAsset->GetMeshes()[meshIdx], *meshRenderData);

                    (*lodRenderData)[meshIdx] = meshRenderData;
                    // check for validity! better option is to allocate and pass by ptr.
                    if (meshIdx == 0)
                        break;   
                }
            }

            AZ_Warning("Meshlets", false, "Meshlet model [%s] was created", m_name.c_str());
            return meshletsCount;
        }

        MeshletsRenderObject::MeshletsRenderObject(Data::Asset<RPI::ModelAsset> sourceModelAsset,
            MeshletsFeatureProcessor* meshletsFeatureProcessor)
        {
            m_featureProcessor = meshletsFeatureProcessor;
            SetShaders();
            m_name = "Model_" + AZStd::to_string(s_modelNumber++);
            m_aabb.CreateNull();

            if (!Meshlets::SharedBufferInterface::Get())
            {
                AZ_Error("Meshlets", false, "Shared buffer was NOT created - meshlets model will not be created.");
                return;
            }
            m_meshletsCount = CreateMeshletsFromModelAsset(sourceModelAsset);
        }

        MeshletsRenderObject::~MeshletsRenderObject()
        {
            for (auto modelLodDataArray : m_modelRenderData)
            {
                for (auto lodData : modelLodDataArray)
                {
                    delete lodData;
                }
            }
        }

    } // namespace Meshlets
} // namespace AZ

#pragma optimize("", on)
