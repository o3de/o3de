/*
* Modifications Copyright (c) Contributors to the Open 3D Engine Project. 
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
* 
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

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
            uint32_t meshletsCount = (uint32_t) meshlets.size();

            m_meshletsData.Descriptors = meshlets;
            m_meshletsData.IndicesIndirection = meshlet_vertices;
            m_meshletsData.EncodeTrianglesData(meshlet_triangles);

            // Some validation
            m_meshletsData.ValidateData((uint32_t)meshlet_vertices.size());
            std::vector<uint32_t> decodedIndices(meshlet_triangles.size());
            m_meshletsData.GenerateDecodedIndices(decodedIndices);
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
            uint32_t badVertices = 0;
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
                    ++badVertices;
                    AZ_Warning("Meshlets", false, "Warning -- vertex [%d:%d] out of bound (%.2f, %.2f, %.2f) in model [%s]",
                        vtx, vtxNum, positionV3.GetX(), positionV3.GetY(), positionV3.GetZ(), m_name.c_str());
                }
            }

            AZ_Error("Meshlets", (badVertices == 0),
                "[%d] Bad Vertices in Model [%s]", badVertices, m_name.c_str());
            AZ_Error("Meshlets", m_aabb.IsValid(), "Error --- Model [%s] AABB is invalid - all [%d] vertices are corrupted",
                m_name.c_str(), vtxNum);
            return m_aabb.IsValid() ? true : false;
        }

        void MeshletsRenderObject::PrepareComputeSrgDescriptors(
            MeshRenderData &meshRenderData, 
            uint32_t vertexCount, uint32_t indexCount)
        {
            if (meshRenderData.ComputeBuffersDescriptors.size())
            {
                return;
            }

            meshRenderData.ComputeBuffersDescriptors.resize(uint8_t(ComputeStreamsSemantics::NumBufferStreams));

            // Allocated using regular buffers
            meshRenderData.ComputeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::MeshletsData)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::Unknown,   // Mark is as Unknown since it represents StructuredBuffer
//                    RHI::BufferBindFlags::Indirect |  [To Do] - add this when moving to GPU driven render pipeline
                    RHI::BufferBindFlags::ShaderRead,
                    sizeof(MeshletDescriptor), (uint32_t)m_meshletsData.Descriptors.size(), 
                    Name{ "MESHLETS" }, Name{ "m_meshletsDescriptors" }, 0, 0,
                    (uint8_t*)m_meshletsData.Descriptors.data()
                );

            meshRenderData.ComputeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::MehsletsTriangles)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32_UINT,
//                    RHI::BufferBindFlags::Indirect |  [To Do] - add this when moving to GPU driven render pipeline
                    RHI::BufferBindFlags::ShaderRead,
                    sizeof(uint32_t), (uint32_t)m_meshletsData.EncodedTriangles.size(),
                    Name{ "MESHLETS_TRIANGLES" }, Name{ "m_meshletsTriangles" }, 1, 0,
                    (uint8_t*)m_meshletsData.EncodedTriangles.data()
                );

            meshRenderData.ComputeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::MeshletsIndicesIndirection)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32_UINT,
//                    RHI::BufferBindFlags::Indirect |  [To Do] - add this when moving to GPU driven render pipeline
                    RHI::BufferBindFlags::ShaderRead,
                    sizeof(uint32_t), (uint32_t)m_meshletsData.IndicesIndirection.size(),
                    Name{ "MESHLETS_LOOKUP" }, Name{ "m_meshletsIndicesLookup" }, 2, 0,
                    (uint8_t*)m_meshletsData.IndicesIndirection.data()
                );

            // Allocated using view into shared buffer to allow for a barrier before the render pass
            // [To Do] - including the InputAssembly flag will fail the validation.
            // This requires change in Atom since the pool flags and the buffer flags are not
            // properly correlated!
            meshRenderData.ComputeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::UVs)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite,
                    RHI::Format::R32G32_FLOAT,
//                    RHI::BufferBindFlags::Indirect |  [To Do] - add this when moving to GPU driven render pipeline
                    RHI::BufferBindFlags::ShaderReadWrite, // | RHI::BufferBindFlags::InputAssembly,
                    sizeof(float) * 2, vertexCount,
                    Name{ "UV" }, Name{ "m_uvs" }, 3, 0
                );

            meshRenderData.ComputeBuffersDescriptors[uint8_t(ComputeStreamsSemantics::Indices)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite,
                    RHI::Format::R32_UINT,
//                    RHI::BufferBindFlags::Indirect |  [To Do] - add this when moving to GPU driven render pipeline
                    RHI::BufferBindFlags::ShaderReadWrite, // | RHI::BufferBindFlags::InputAssembly,
                    sizeof(uint32_t), indexCount,
                    Name{ "INDICES" }, Name{ "m_indices" }, 4, 0
                );
        }

        void MeshletsRenderObject::PrepareRenderSrgDescriptors(
            MeshRenderData &meshRenderData, uint32_t vertexCount, uint32_t indicesCount)
        {
            if (meshRenderData.RenderBuffersDescriptors.size())
            {
                return;
            }

            meshRenderData.RenderBuffersDescriptors.resize(uint8_t(RenderStreamsSemantics::NumBufferStreams));

            meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Positions)] =
                SrgBufferDescriptor(
//                    RPI::CommonBufferPoolType::StaticInputAssembly,
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32_FLOAT,
//                    RHI::Format::R32G32B32_FLOAT,
//                    RHI::BufferBindFlags::Indirect |  [To Do] - add this when moving to GPU driven render pipeline
//                    RHI::BufferBindFlags::InputAssembly, 
                    RHI::BufferBindFlags::ShaderRead,
                    sizeof(float),
                    3 * vertexCount,    // The amount of elements   
                    Name{ "POSITION" }, Name{ "m_positions" }, 0, 0
                );

            // The following should be unknown structure type to represent StructuredBuffer.
            // This is done in order to avoid misalignment due to elements that are not
            // 16 bytes aligned.
            meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Normals)] =
                SrgBufferDescriptor(
//                    RPI::CommonBufferPoolType::StaticInputAssembly,
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32_FLOAT,
//                    RHI::Format::R32G32B32_FLOAT,
//                    RHI::BufferBindFlags::Indirect |  [To Do] - add this when moving to GPU driven render pipeline
//                    RHI::BufferBindFlags::InputAssembly, 
                    RHI::BufferBindFlags::ShaderRead,
                    sizeof(float),
                    3 * vertexCount,    // The amount of elements   
                    Name{ "NORMAL" }, Name{ "m_normals" }, 1, 0
                );

            meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Tangents)] =
                SrgBufferDescriptor(
//                    RPI::CommonBufferPoolType::StaticInputAssembly,
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32G32B32A32_FLOAT,
//                    RHI::BufferBindFlags::Indirect |  [To Do] - add this when moving to GPU driven render pipeline
//                    RHI::BufferBindFlags::InputAssembly, 
                    RHI::BufferBindFlags::ShaderRead,
                    sizeof(float) * 4, vertexCount,
                    Name{ "TANGENT" }, Name{ "m_tangents" }, 2, 0
                );

            meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::BiTangents)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32_FLOAT,
//                    RHI::Format::R32G32B32_FLOAT,
//                    RHI::BufferBindFlags::Indirect |  [To Do] - add this when moving to GPU driven render pipeline
//                    RHI::BufferBindFlags::InputAssembly, 
                    RHI::BufferBindFlags::ShaderRead,
                    sizeof(float),
                    3 * vertexCount,    // The amount of elements   
                    Name{ "BITANGENT" }, Name{ "m_bitangents" }, 3, 0
                );

            // For now created as ReadWrite shared buffer - should be ReadOnly in the final product
            meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::UVs)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32G32_FLOAT,
//                    RHI::BufferBindFlags::Indirect |  [To Do] - add this when moving to GPU driven render pipeline
                    RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::InputAssembly,
                    sizeof(float) * 2, vertexCount,
                    Name{ "UV" }, Name{ "m_uvs" }, 4, 0
                );

            // Notice that several of these buffers were already created and so the pool type
            // doesn't really matter since it won't be used.
            meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Indices)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::StaticInputAssembly,  // Not used (by the pool), created using shared buffer
                    RHI::Format::R32_UINT,
//                    RHI::BufferBindFlags::Indirect |  [To Do] - add this when moving to GPU driven render pipeline
                    RHI::BufferBindFlags::ShaderReadWrite | RHI::BufferBindFlags::InputAssembly,
                    sizeof(uint32_t), indicesCount,
                    Name{ "INDICES" }, Name{ "m_indices" }, 5, 0
                );
        }

        // Notice that unlike the Compute buffers, for the render all the buffers are
        // read only and because of this we can create and bind all in one stage.
        bool MeshletsRenderObject::CreateAndBindRenderBuffers(MeshRenderData &meshRenderData)
        {
            // Create the Srg first - required for the buffers generation
            if (m_renderShader)
            {
                meshRenderData.RenderObjectSrg = RPI::ShaderResourceGroup::Create(
                        m_renderShader->GetAsset(), AZ::Name{ "MeshletsObjectRenderSrg" });
            }
            if (!meshRenderData.RenderObjectSrg)
            {
                AZ_Error("Meshlets", false, "Failed to create the Render Srg - Meshlets Mesh load will fail");
                return false;
            }

            bool success = true;
            uint32_t streamsNum = (uint32_t) meshRenderData.RenderBuffersDescriptors.size();
            meshRenderData.RenderBuffersViews.resize(streamsNum);   // resize for a vector, no need for an array
            meshRenderData.RenderBuffers.resize(streamsNum);

            // Unlike the compute method - the render method actually creates all buffers including
            // the shared indices and UVs buffers that are used by the Compute prior to rendering.
            for (uint32_t stream = 0; stream < streamsNum ; ++stream)
            {
                SrgBufferDescriptor& bufferDesc = meshRenderData.RenderBuffersDescriptors[stream];

                if ((stream == uint8_t(RenderStreamsSemantics::UVs)) ||
                    (stream == uint8_t(RenderStreamsSemantics::Indices))) 
                {   
                    // Shared Buffer Views: allocate views from the shared buffer since all buffers will
                    // share the same state and be shader read/write - in this case the buffers were created
                    // by CreateComputeBuffers so we need to copy the data from there including the
                    // descriptors that contains the proper offsets to the shared buffer.

                    if (stream == uint8_t(RenderStreamsSemantics::UVs))
                    {
                        uint8_t mappedIdx = uint8_t(ComputeStreamsSemantics::UVs);
                        bufferDesc.m_viewOffsetInBytes = meshRenderData.ComputeBuffersDescriptors[mappedIdx].m_viewOffsetInBytes;
                        meshRenderData.RenderBuffersViews[stream] = meshRenderData.ComputeBuffersViews[mappedIdx];

                        RHI::ShaderInputBufferIndex indexHandle = meshRenderData.RenderObjectSrg->FindShaderInputBufferIndex(bufferDesc.m_paramNameInSrg);
                        bufferDesc.m_resourceShaderIndex = indexHandle.GetIndex();
                        if (!meshRenderData.RenderObjectSrg->SetBufferView(indexHandle, meshRenderData.RenderBuffersViews[stream]))
                        {
                            AZ_Error("Meshlets", false, "Failed to bind render buffer view for %s", bufferDesc.m_bufferName.GetCStr());
                            return false;
                        }
                    }
                    if (stream == uint8_t(RenderStreamsSemantics::Indices))
                    {
                        uint8_t mappedIdx = uint8_t(ComputeStreamsSemantics::Indices);
                        bufferDesc.m_viewOffsetInBytes = meshRenderData.ComputeBuffersDescriptors[mappedIdx].m_viewOffsetInBytes;

                        meshRenderData.IndexBufferView = RHI::IndexBufferView(
                            *meshRenderData.ComputeBuffersViews[mappedIdx]->GetBuffer(),
                            bufferDesc.m_viewOffsetInBytes,
                            (uint64_t)bufferDesc.m_elementCount * bufferDesc.m_elementSize,
                            (bufferDesc.m_elementFormat == RHI::Format::R32_UINT) ? RHI::IndexFormat::Uint32 : RHI::IndexFormat::Uint16);
                    }
                }
                else
                {   // Regular buffers creation - since they are read only, no need to use the shared buffer
                    meshRenderData.RenderBuffersViews[stream] = Data::Instance<RHI::BufferView>();
                    meshRenderData.RenderBuffers[stream] = UtilityClass::CreateBufferAndBindToSrg(
                        "Meshlets", bufferDesc, meshRenderData.RenderObjectSrg);

                    success &= (meshRenderData.RenderBuffers[stream] ? true : false);
                }
            }

            // Copy the original mesh data into the buffers or delete the allocators if failed
            if (success)
            {
                for (uint32_t stream = 0; stream < streamsNum && success ; ++stream)
                {   // upload the original streams data - skip indices and UVs copy : Compute will set them
                    if ((stream == uint8_t(RenderStreamsSemantics::UVs)) ||
                        (stream == uint8_t(RenderStreamsSemantics::Indices)))
                    {   // Update the data so that we can compare with the Compute output 
//                        continue;
                    }

                    Data::Instance<RPI::Buffer> buffer = meshRenderData.RenderBuffers[stream] ?
                        meshRenderData.RenderBuffers[stream] : 
                        Meshlets::SharedBufferInterface::Get()->GetBuffer();

                    SrgBufferDescriptor& bufferDesc = meshRenderData.RenderBuffersDescriptors[stream];
                    size_t requiredSize = (uint64_t)bufferDesc.m_elementCount * bufferDesc.m_elementSize;
                    bool upload = buffer->UpdateData( bufferDesc.m_bufferData, requiredSize, bufferDesc.m_viewOffsetInBytes);
                    AZ_Error("Meshlets", upload, "Data could not be uploaded to Render buffer [%s]", bufferDesc.m_bufferName.GetCStr());
                    success &= upload;
                }
            }

            // Final stage - compile the Srg
            // Make sure to compile the Srg - when setting all parameter including ObjectId.
//            meshRenderData.RenderObjectSrg->Compile();

            return success;
        }

        // For the Compute, since some of the buffers are RW the RHI will verify that
        // they are attached to the frame scheduler (ValidateSetBufferView) and this
        // might fail if the creation is not times correctly, hence they is a split
        // between the creation and the binding to the Srg.
        bool MeshletsRenderObject::CreateAndBindComputeSrgAndDispatch(
            Data::Instance<RPI::Shader> computeShader, MeshRenderData& meshRenderData)
        {
            // Start with the Srg creation - it will be required for the buffers generation
            if (meshRenderData.ComputeSrg)
            {
                return true;
            }

            meshRenderData.ComputeSrg =
                RPI::ShaderResourceGroup::Create(computeShader->GetAsset(), AZ::Name{ "MeshletsDataSrg" });
            if (!meshRenderData.ComputeSrg)
            {
                AZ_Error("Meshlets", false, "Failed to create the Compute Srg");
                return false;
            }

            uint32_t streamsNum = (uint32_t)meshRenderData.ComputeBuffersDescriptors.size();
            bool success = true;
            for (uint32_t stream = 0; stream < streamsNum ; ++stream)
            {
                SrgBufferDescriptor& bufferDesc = meshRenderData.ComputeBuffersDescriptors[stream];

                if ((stream == uint8_t(ComputeStreamsSemantics::UVs)) ||
                    (stream == uint8_t(ComputeStreamsSemantics::Indices)))
                {   // Shared Buffer Views: allocate views from the shared buffer since Index and UV buffers will
                    // share the same state and be shader read/write.
                    if (!meshRenderData.ComputeBuffersViews[stream])
                    {
                        AZ_Error("Meshlets", false, "Buffer view doesn't exist");
                        success = false;
                        continue;
                    }

                    success &= UtilityClass::BindBufferViewToSrg(
                        "Meshlets", meshRenderData.ComputeBuffersViews[stream], bufferDesc,
                        meshRenderData.ComputeSrg);

                    // And now for the second method - using offsets within the shared buffer
                    AZ::Name constantName = (stream == uint8_t(ComputeStreamsSemantics::UVs)) ?
                        Name("m_texCoordsOffset") : Name("m_indicesOffset");
                    RHI::ShaderInputConstantIndex constantHandle = meshRenderData.ComputeSrg->FindShaderInputConstantIndex(constantName);
                    uint32_t offsetInUint = bufferDesc.m_viewOffsetInBytes / sizeof(uint32_t);
                    if (!meshRenderData.ComputeSrg->SetConstant(constantHandle, offsetInUint))
                    {
                        AZ_Error("Meshlets", false, "Failed to bind Constant [%s]", constantName.GetCStr());
                        return false;
                    }
                }
                else
                {   // Regular buffers: since these buffers are read only and will not be altered there is no need to
                    // use the shared buffer. This also means that we bind using buffers instead of buffers views.
                    if (!meshRenderData.ComputeBuffers[stream])
                    {
                        AZ_Error("Meshlets", false, "Buffer doesn't exist");
                        success = false;
                        continue;
                    }

                    success &= UtilityClass::BindBufferToSrg(
                        "Meshlets", meshRenderData.ComputeBuffers[stream], bufferDesc,
                        meshRenderData.ComputeSrg);
                }
            }

            if (success)
            {
                // Compile the Srg and create the dispatch 
                meshRenderData.ComputeSrg->Compile();

                meshRenderData.MeshDispatchItem.InitDispatch(
                    computeShader.get(), meshRenderData.ComputeSrg, meshRenderData.MeshletsCount);
            }

            return success;
        }

        bool MeshletsRenderObject::CreateComputeBuffers(MeshRenderData &meshRenderData)
        {
            bool success = true;
            uint32_t streamsNum = (uint32_t)meshRenderData.ComputeBuffersDescriptors.size();
            meshRenderData.ComputeBuffersAllocators.resize(streamsNum);
            meshRenderData.ComputeBuffersViews.resize(streamsNum);
            meshRenderData.ComputeBuffers.resize(streamsNum);

            for (uint32_t stream = 0; stream < streamsNum ; ++stream)
            {
                SrgBufferDescriptor& bufferDesc = meshRenderData.ComputeBuffersDescriptors[stream];

                if ((stream == uint8_t(ComputeStreamsSemantics::UVs)) ||
                    (stream == uint8_t(ComputeStreamsSemantics::Indices)))
                {   // Shared Buffer Views: allocate views from the shared buffer since Index and UV buffers will
                    // share the same state and be shader read/write.
                    meshRenderData.ComputeBuffersViews[stream] = UtilityClass::CreateSharedBufferView(
                        "Meshlets", bufferDesc, meshRenderData.ComputeBuffersAllocators[stream]);
                }
                else
                {   // Regular buffers: since these buffers are read only and will not be altered there is no need to
                    // use the shared buffer. This also means that we bind using buffers instead of buffers views.
                    meshRenderData.ComputeBuffersViews[stream] = Data::Instance<RHI::BufferView>();
                    meshRenderData.ComputeBuffers[stream] = UtilityClass::CreateBuffer("Meshlets", bufferDesc);

                    success &= (meshRenderData.ComputeBuffers[stream] ? true : false);
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

                    SrgBufferDescriptor& bufferDesc = meshRenderData.ComputeBuffersDescriptors[stream];
                    size_t requiredSize = (uint64_t)bufferDesc.m_elementCount * bufferDesc.m_elementSize;
                    bool upload = meshRenderData.ComputeBuffers[stream]->UpdateData(bufferDesc.m_bufferData, requiredSize, bufferDesc.m_viewOffsetInBytes);
                    AZ_Error("Meshlets", success == upload, "Data could not be uploaded to Compute buffer [%s]", bufferDesc.m_bufferName.GetCStr());
                    success &= upload;
                }
            }

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
            SrgBufferDescriptor* descriptor = &meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Indices)];
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

                descriptor = &meshRenderData.RenderBuffersDescriptors[stream];
                bufferAssetView = meshAsset.GetSemanticBufferAssetView(descriptor->m_bufferName);
                if (!bufferAssetView)
                {
                    AZ_Error("Meshlets", false,
                        "Error - missing buffer stream [%s]", descriptor->m_bufferName.GetCStr());
                    return false;
                }
                descriptor->m_bufferData = RetrieveBufferData(bufferAssetView, streamFormat, vertexCount, vertexCount, bufferDescriptor);

                if (streamFormat != descriptor->m_elementFormat)
                {
                    AZ_Warning("Meshlets", false,
                        "Error - buffer %s with different format [%d]", descriptor->m_bufferName.GetCStr(), streamFormat);
//                    success = false;
                }

                if (!descriptor->m_bufferData)
                {
                    AZ_Error("Meshlets", false, "Failed to create meshlet model [%s] - buffer [%s] data could not be retrieved",
                        descriptor->m_bufferName.GetCStr());
                    return false;
                }
            }

            // AABB generation - can also be used for vertices scaling / creating transform.
            success &= ProcessBuffersData((float*)meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Positions)].m_bufferData, vertexCount);

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
                (float*)meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Positions)].m_bufferData,
                (float*)meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Normals)].m_bufferData,
                (float*)meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::UVs)].m_bufferData,
                vertexCount,
                (uint16_t*) meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Indices)].m_bufferData,
                indexCount,
                meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Indices)].m_elementFormat
                );

            if (!meshletsCount)
            {
                AZ_Error("Meshlets", false, "Failed to create meshlet model [%s] - the meshlet creation process failed", m_name.c_str());
                return 0;
            }

            meshRenderData.MeshletsCount = meshletsCount;
            meshRenderData.IndexCount = indexCount;

            //----------------------------------------------------------------
            // Prepare the Compute buffers, views and Srg for the Compute pass.
            PrepareComputeSrgDescriptors(meshRenderData, vertexCount, indexCount);
            if (!CreateComputeBuffers(meshRenderData))
            {
                 return 0;
            }

            //----------------------------------------------------------------
            // Create the render streams and bind the the render Srg for the Render pass.
            if (!CreateAndBindRenderBuffers(meshRenderData))
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

            format = bufferDesc.m_elementFormat;
            AZStd::span<const uint8_t> bufferData = bufferAsset->GetBuffer();
            return (uint8_t *) bufferData.data();
        }

        // [To Do] - currently we create only the first mesh of the first Lod to be able to
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
            MeshletsRenderObject::s_textureCoordinatesName = Name("m_texCoordsOffset");
            MeshletsRenderObject::s_indicesName = Name("m_indicesOffset");

            m_featureProcessor = meshletsFeatureProcessor;
            SetShaders();
            m_name = "Model_" + AZStd::to_string(s_modelNumber++);
            m_aabb = Aabb::CreateNull();

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
