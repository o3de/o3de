/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/SkinnedMesh/SkinnedMeshInputBuffers.h>
#include <Atom/Feature/MorphTargets/MorphTargetInputBuffers.h>
#include <SkinnedMesh/SkinnedMeshOutputStreamManager.h>

#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>
#include <Atom/RPI.Reflect/Model/MorphTargetMetaAsset.h>
#include <Atom/RPI.Reflect/Model/MorphTargetDelta.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RHI/Factory.h>

#include <AzCore/std/algorithm.h>
#include <AzCore/Math/PackedVector3.h>

AZ_DECLARE_BUDGET(AzRender);

namespace AZ
{
    namespace Render
    {
        Data::Asset<RPI::BufferAsset> CreateBufferAsset(const void* data, const RHI::BufferViewDescriptor& viewDescriptor, RHI::BufferBindFlags bindFlags, Data::Asset<RPI::ResourcePoolAsset> resourcePoolAsset, const char* bufferName)
        {
            const uint32_t bufferSize = viewDescriptor.m_elementCount * viewDescriptor.m_elementSize;

            Data::Asset<RPI::BufferAsset> asset;
            {
                RHI::BufferDescriptor bufferDescriptor;
                bufferDescriptor.m_bindFlags = bindFlags;

                bufferDescriptor.m_byteCount = bufferSize;
                bufferDescriptor.m_alignment = viewDescriptor.m_elementSize;

                RPI::BufferAssetCreator creator;
                Uuid uuid = Uuid::CreateRandom();
                creator.Begin(uuid);

                creator.SetPoolAsset(resourcePoolAsset);
                creator.SetBuffer(data, bufferDescriptor.m_byteCount, bufferDescriptor);
                // Create a unique buffer name by combining the given, friendly buffer name with the uuid. Use isBrackents=false and isDashes=false to make it look less like some kind of AssetId that has any meaning.
                creator.SetBufferName(AZStd::string::format("%s_%s", bufferName, uuid.ToString<AZStd::string>(false,false).c_str()));
                creator.SetBufferViewDescriptor(viewDescriptor);

                creator.End(asset);
            }

            return asset;
        }

        void SkinnedMeshInputLod::CreateFromModelLodAsset(const Data::Asset<RPI::ModelLodAsset>& modelLodAsset)
        {
            m_modelLodAsset = modelLodAsset;
        }

        Data::Asset<RPI::ModelLodAsset> SkinnedMeshInputLod::GetModelLodAsset() const
        {
            return m_modelLodAsset;
        }

        void SkinnedMeshInputLod::SetIndexCount(uint32_t indexCount)
        {
            m_indexCount = indexCount;
        }

        void SkinnedMeshInputLod::SetVertexCount(uint32_t vertexCount)
        {
            m_vertexCount = vertexCount;
        }

        uint32_t SkinnedMeshInputLod::GetVertexCountForStream(SkinnedMeshOutputVertexStreams outputStream) const
        {
            return m_outputVertexCountsByStream[static_cast<uint8_t>(outputStream)];
        }

        void SkinnedMeshInputLod::SetIndexBufferAsset(const Data::Asset<RPI::BufferAsset> bufferAsset)
        {
            m_indexBufferAsset = bufferAsset;
            m_indexBuffer = RPI::Buffer::FindOrCreate(bufferAsset);
        }

        void SkinnedMeshInputLod::CreateIndexBuffer(const uint32_t* data, const AZStd::string& bufferNamePrefix)
        {
            AZ_Assert(m_indexCount > 0, "SkinnedMeshInputLod::CreateIndexBuffer called with a index count of 0. Make sure SetIndexCount has been called before trying to create any buffers.");

            RHI::BufferViewDescriptor indexBufferViewDescriptor = RHI::BufferViewDescriptor::CreateTyped(0, m_indexCount, AZ::RHI::Format::R32_UINT);

            // Use the user-specified buffer name if it exits, or a default one otherwise.
            const char* bufferName = !bufferNamePrefix.empty() ? bufferNamePrefix.c_str() : "SkinnedMeshStaticIndexBuffer";

            Data::Asset<RPI::BufferAsset> bufferAsset = CreateBufferAsset(data, indexBufferViewDescriptor, RHI::BufferBindFlags::InputAssembly, SkinnedMeshVertexStreamPropertyInterface::Get()->GetStaticStreamResourcePool(), bufferName);
            m_indexBufferAsset = bufferAsset;
            m_indexBuffer = RPI::Buffer::FindOrCreate(bufferAsset);
        }

        void SkinnedMeshInputLod::CreateSkinningInputBuffer(void* data, SkinnedMeshInputVertexStreams inputStream, const AZStd::string& bufferNamePrefix)
        {
            AZ_Assert(m_vertexCount > 0, "SkinnedMeshInputLod::CreateSkinningInputBuffer called with a vertex count of 0. Make sure SetVertexCount has been called before trying to create any buffers.");

            const SkinnedMeshVertexStreamInfo& streamInfo = SkinnedMeshVertexStreamPropertyInterface::Get()->GetInputStreamInfo(inputStream);
            RHI::BufferViewDescriptor viewDescriptor = RHI::BufferViewDescriptor::CreateRaw(0, m_vertexCount * streamInfo.m_elementSize);

            // Use the user-specified buffer name if it exits, or a default one from the streamInfo otherwise.
            const char* bufferName = !bufferNamePrefix.empty() ? bufferNamePrefix.c_str() : streamInfo.m_bufferName.GetCStr();

            Data::Asset<RPI::BufferAsset> bufferAsset = CreateBufferAsset(data, viewDescriptor, RHI::BufferBindFlags::ShaderRead, SkinnedMeshVertexStreamPropertyInterface::Get()->GetInputStreamResourcePool(), bufferName);
            m_inputBufferAssets[static_cast<uint8_t>(inputStream)] = bufferAsset;
            m_inputBuffers[static_cast<uint8_t>(inputStream)] = RPI::Buffer::FindOrCreate(bufferAsset);
        }

        void SkinnedMeshInputLod::SetSkinningInputBufferAsset(const Data::Asset<RPI::BufferAsset> bufferAsset, SkinnedMeshInputVertexStreams inputStream)
        {
            if (inputStream == SkinnedMeshInputVertexStreams::Color)
            {
                AZ_Assert(!m_hasStaticColors, "Attempting to set colors as skinning input (meaning they are dynamic) when they already exist as a static stream");
                m_hasDynamicColors = true;
            }

            m_inputBufferAssets[static_cast<uint8_t>(inputStream)] = bufferAsset;
            Data::Instance<RPI::Buffer> buffer = RPI::Buffer::FindOrCreate(bufferAsset);
            m_inputBuffers[static_cast<uint8_t>(inputStream)] = buffer;

            // Create a buffer view to use as input to the skinning shader
            AZ::RHI::Ptr<AZ::RHI::BufferView> bufferView = RHI::Factory::Get().CreateBufferView();                
            bufferView->SetName(Name{ AZStd::string(buffer->GetBufferView()->GetName().GetStringView()) + "_SkinningInputBufferView" });
            RHI::BufferViewDescriptor bufferViewDescriptor = bufferAsset->GetBufferViewDescriptor();

            // 3-component float buffers are not supported on metal for non-input assembly buffer views, so use a float view instead
            if (bufferViewDescriptor.m_elementFormat == RHI::Format::R32G32B32_FLOAT)
            {
                // Use one float per element, with 3x as many elements
                bufferViewDescriptor = RHI::BufferViewDescriptor::CreateTyped(
                    bufferViewDescriptor.m_elementOffset * 3, bufferViewDescriptor.m_elementCount * 3, RHI::Format::R32_FLOAT);
            }

            [[maybe_unused]] RHI::ResultCode resultCode =
                bufferView->Init(*buffer->GetRHIBuffer(), bufferViewDescriptor);
            AZ_Error(
                "SkinnedMeshInputBuffers", resultCode == RHI::ResultCode::Success,
                "Failed to initialize buffer view for skinned mesh input.");
                
            m_bufferViews[static_cast<uint8_t>(inputStream)] = bufferView;
        }

        void SkinnedMeshInputLod::SetStaticBufferAsset(const Data::Asset<RPI::BufferAsset> bufferAsset, SkinnedMeshStaticVertexStreams staticStream)
        {
            if (staticStream == SkinnedMeshStaticVertexStreams::Color)
            {
                AZ_Assert(!m_hasDynamicColors, "Attempting to set colors as a static stream on a skinned mesh, when they already exist as a dynamic stream");
                m_hasStaticColors = true;
            }

            m_staticBuffers[static_cast<uint8_t>(staticStream)] = RPI::Buffer::FindOrCreate(bufferAsset);
        }

        void SkinnedMeshInputLod::CreateStaticBuffer(void* data, SkinnedMeshStaticVertexStreams staticStream, const AZStd::string& bufferNamePrefix)
        {
            AZ_Assert(m_vertexCount > 0, "SkinnedMeshInputLod::CreateStaticBuffer called with a vertex count of 0. Make sure SetVertexCount has been called before trying to create any buffers.");

            const SkinnedMeshVertexStreamInfo& streamInfo = SkinnedMeshVertexStreamPropertyInterface::Get()->GetStaticStreamInfo(staticStream);
            RHI::BufferViewDescriptor viewDescriptor = RHI::BufferViewDescriptor::CreateTyped(0, m_vertexCount, streamInfo.m_elementFormat);

            // Use the user-specified buffer name if it exits, or a default one from the streamInfo otherwise.
            const char* bufferName = !bufferNamePrefix.empty() ? bufferNamePrefix.c_str() : streamInfo.m_bufferName.GetCStr();

            Data::Asset<RPI::BufferAsset> bufferAsset = CreateBufferAsset(data, viewDescriptor, RHI::BufferBindFlags::InputAssembly, SkinnedMeshVertexStreamPropertyInterface::Get()->GetStaticStreamResourcePool(), bufferName);
            m_staticBuffers[static_cast<uint8_t>(staticStream)] = RPI::Buffer::FindOrCreate(bufferAsset);
        }

        const Data::Asset<RPI::BufferAsset>& SkinnedMeshInputLod::GetSkinningInputBufferAsset(SkinnedMeshInputVertexStreams stream) const
        {
            return m_inputBufferAssets[static_cast<uint8_t>(stream)];
        }

        void SkinnedMeshInputLod::WaitForUpload()
        {
            m_indexBuffer->WaitForUpload();

            for (const Data::Instance<RPI::Buffer>& inputBuffer : m_inputBuffers)
            {
                if (inputBuffer)
                {
                    inputBuffer->WaitForUpload();
                }
            }

            for (const Data::Instance<RPI::Buffer>& staticBuffer : m_staticBuffers)
            {
                if (staticBuffer)
                {
                    staticBuffer->WaitForUpload();
                }
            }
        }

        void SkinnedMeshInputLod::AddMorphTarget(const RPI::MorphTargetMetaAsset::MorphTarget& morphTarget, const Data::Asset<RPI::BufferAsset>& morphBufferAsset, const AZStd::string& bufferNamePrefix, float minWeight = 0.0f, float maxWeight = 1.0f)
        {
            m_morphTargetMetaDatas.push_back(MorphTargetMetaData{ minWeight, maxWeight, morphTarget.m_minPositionDelta, morphTarget.m_maxPositionDelta, morphTarget.m_numVertices, morphTarget.m_startIndex });
            
            // Create a view into the larger per-lod morph buffer for this particular morph
            RHI::BufferViewDescriptor morphView = RHI::BufferViewDescriptor::CreateStructured(morphTarget.m_startIndex, morphTarget.m_numVertices, sizeof(RPI::PackedCompressedMorphTargetDelta));
            RPI::BufferAssetView morphTargetDeltaView{ morphBufferAsset, morphView };

            m_morphTargetInputBuffers.push_back(aznew MorphTargetInputBuffers{ morphTargetDeltaView, bufferNamePrefix });

            // If colors are going to be morphed, the SkinnedMeshInputLod needs to know so that it allocates memory for the dynamically updated colors
            if (morphTarget.m_hasColorDeltas)
            {
                m_hasDynamicColors = true;
            }
        }

        bool SkinnedMeshInputLod::HasDynamicColors() const
        {
            return m_hasDynamicColors;
        }
        
        const AZStd::vector<MorphTargetMetaData>& SkinnedMeshInputLod::GetMorphTargetMetaDatas() const
        {
            return m_morphTargetMetaDatas;
        }

        const AZStd::vector<AZStd::intrusive_ptr<MorphTargetInputBuffers>>& SkinnedMeshInputLod::GetMorphTargetInputBuffers() const
        {
            return m_morphTargetInputBuffers;
        }

        SkinnedMeshInputBuffers::SkinnedMeshInputBuffers() = default;
        SkinnedMeshInputBuffers::~SkinnedMeshInputBuffers() = default;

        void SkinnedMeshInputBuffers::CreateFromModelAsset(const Data::Asset<RPI::ModelAsset>& modelAsset)
        {
            if (!modelAsset.IsReady())
            {
                AZ_Error("SkinnedMeshInputBuffers", false, "Trying to create a skinned mesh from a model '%s' that isn't loaded.", modelAsset.GetHint().c_str());
                return;
            }

            m_modelAsset = modelAsset;
            m_model = RPI::Model::FindOrCreate(m_modelAsset);

            if (m_model)
            {
                m_lods.resize(m_model->GetLodCount());                
                for (size_t lodIndex = 0; lodIndex < m_model->GetLodCount(); ++lodIndex)
                {
                    const Data::Instance<RPI::ModelLod>& modelLod = m_model->GetLods()[lodIndex];
                    Data::Asset<RPI::ModelLodAsset> modelLodAsset = m_modelAsset->GetLodAssets()[lodIndex];

                    // Add a new lod to the SkinnedMeshInputBuffers
                    SkinnedMeshInputLod& skinnedMeshLod = m_lods[lodIndex];
                    skinnedMeshLod.CreateFromModelLodAsset(modelLodAsset);

                    // Collect the vertex count for each output stream
                    skinnedMeshLod.m_outputVertexCountsByStream = SkinnedMeshOutputVertexCounts{ 0 };
                    SkinnedMeshOutputVertexOffsets currentMeshOffsetFromStreamStart = { 0 };

                    skinnedMeshLod.m_meshes.resize(modelLod->GetMeshes().size());
                    for (size_t meshIndex = 0; meshIndex < modelLod->GetMeshes().size(); ++meshIndex)
                    {
                        SkinnedSubMeshProperties& skinnedSubMesh = skinnedMeshLod.m_meshes[meshIndex];
                        skinnedSubMesh.m_vertexOffsetsFromStreamStartInBytes = SkinnedMeshOutputVertexOffsets{ 0 };

                        // Get the source mesh
                        const RPI::ModelLodAsset::Mesh& modelLodAssetMesh = m_modelAsset->GetLodAssets()[lodIndex]->GetMeshes()[meshIndex];
                        skinnedSubMesh.m_vertexCount = modelLodAssetMesh.GetVertexCount();

                        // Get all of the streams potentially used as input to the skinning compute shader
                        RHI::InputStreamLayout inputLayout;
                        RPI::ModelLod::StreamBufferViewList streamBufferViews;
                        modelLod->GetStreamsForMesh(inputLayout, streamBufferViews, nullptr, SkinnedMeshVertexStreamPropertyInterface::Get()->GetComputeShaderInputContract(), meshIndex);

                        AZ_Assert(inputLayout.GetStreamBuffers().size() == streamBufferViews.size(), "Mismatch in size of InputStreamLayout and StreamBufferViewList for model '%s'", m_modelAsset.GetHint().c_str());

                        AZStd::array<bool, static_cast<uint8_t>(SkinnedMeshInputVertexStreams::NumVertexStreams)> meshHasInputStream{ false };

                        // Create a buffer view for each input stream in the current mesh
                        skinnedSubMesh.m_inputBufferViews.reserve(streamBufferViews.size());
                        for (size_t meshStreamIndex = 0; meshStreamIndex < streamBufferViews.size(); ++meshStreamIndex)
                        {
                            const SkinnedMeshVertexStreamInfo* streamInfo = SkinnedMeshVertexStreamPropertyInterface::Get()->GetInputStreamInfo(inputLayout.GetStreamChannels()[meshStreamIndex].m_semantic);

                            if (streamInfo && streamBufferViews[meshStreamIndex].GetByteCount() > 0)
                            {
                                if (streamInfo->m_enum == SkinnedMeshInputVertexStreams::Color && !skinnedMeshLod.m_hasDynamicColors)
                                {
                                    // There is a color stream on the mesh, but it is not modified by morph targets,
                                    // so it does not need to be included as input to the compute shader which applies the morph targets
                                    continue;
                                }

                                skinnedMeshLod.m_inputBufferAssets[static_cast<uint8_t>(streamInfo->m_enum)] = modelLodAssetMesh.GetSemanticBufferAssetView(streamInfo->m_semantic.m_name)->GetBufferAsset();

                                uint32_t elementOffset = streamBufferViews[meshStreamIndex].GetByteOffset() / streamBufferViews[meshStreamIndex].GetByteStride();
                                uint32_t elementCount = streamBufferViews[meshStreamIndex].GetByteCount() / streamBufferViews[meshStreamIndex].GetByteStride();

                                RHI::BufferViewDescriptor descriptor;
                                if (streamInfo->m_enum == SkinnedMeshInputVertexStreams::BlendIndices)
                                {
                                    // Create a descriptor for a raw view from the StreamBufferView
                                    descriptor = RHI::BufferViewDescriptor::CreateRaw(
                                        streamBufferViews[meshStreamIndex].GetByteOffset(),
                                        streamBufferViews[meshStreamIndex].GetByteCount());
                                }
                                else if (streamInfo->m_elementFormat == RHI::Format::R32G32B32_FLOAT)
                                {
                                    // 3-component float buffers are not supported on metal for non-input assembly buffer views,
                                    // so use a float view instead
                                    descriptor =
                                        RHI::BufferViewDescriptor::CreateTyped(elementOffset * 3, elementCount * 3, RHI::Format::R32_FLOAT);
                                }
                                else
                                {
                                    // Create a descriptor for a typed buffer view from the StreamBufferView
                                    descriptor =
                                        RHI::BufferViewDescriptor::CreateTyped(elementOffset, elementCount, streamInfo->m_elementFormat);
                                }

                                AZ::RHI::Ptr<AZ::RHI::BufferView> bufferView = RHI::Factory::Get().CreateBufferView();
                                {
                                    // Initialize the buffer view
                                    AZStd::string bufferViewName = AZStd::string::format("%s_lod%zu_mesh%zu_%s", m_modelAsset->GetName().GetCStr(), lodIndex, meshIndex, streamInfo->m_shaderResourceGroupName.GetCStr());
                                    bufferView->SetName(Name(bufferViewName));
                                    RHI::ResultCode resultCode = bufferView->Init(*streamBufferViews[meshStreamIndex].GetBuffer(), descriptor);

                                    if (resultCode == RHI::ResultCode::Success)
                                    {
                                        // Keep track of which streams exist for the current mesh
                                        meshHasInputStream[static_cast<uint8_t>(streamInfo->m_enum)] = true;
                                    }
                                    else
                                    {
                                        AZ_Error("MorphTargetInputBuffers", false, "Failed to initialize buffer view for morph target.");
                                    }
                                }

                                // Add the buffer view along with the shader resource group name, which will be used to bind it to the srg later
                                skinnedSubMesh.m_inputBufferViews.push_back(AZStd::make_tuple(streamInfo->m_shaderResourceGroupName, bufferView));

                                if (streamInfo->m_enum == SkinnedMeshInputVertexStreams::BlendWeights)
                                {
                                    // ATOM-3247 Support more or less than 4 influences per vertex
                                    skinnedSubMesh.m_skinInfluenceCountPerVertex = elementCount / modelLodAssetMesh.GetVertexCount();
                                    AZ_Assert(skinnedSubMesh.m_skinInfluenceCountPerVertex == 4, "Only 4 influences per vertex are supported at this time");
                                }
                            }
                        }

                        for (uint8_t outputStreamIndex = 0; outputStreamIndex < static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams); ++outputStreamIndex)
                        {
                            const SkinnedMeshOutputVertexStreamInfo& outputStreamInfo = SkinnedMeshVertexStreamPropertyInterface::Get()->GetOutputStreamInfo(static_cast<SkinnedMeshOutputVertexStreams>(outputStreamIndex));

                            // If there is no input to be skinned, then we won't need to bind the output stream
                            if (meshHasInputStream[static_cast<uint8_t>(outputStreamInfo.m_correspondingInputVertexStream)])
                            {
                                // Keep track of the offset for the individual mesh
                                skinnedSubMesh.m_vertexOffsetsFromStreamStartInBytes[outputStreamIndex] = currentMeshOffsetFromStreamStart[outputStreamIndex];
                                currentMeshOffsetFromStreamStart[outputStreamIndex] += modelLodAssetMesh.GetVertexCount() * outputStreamInfo.m_elementSize;
                                // Keep track of the total for the whole lod
                                skinnedMeshLod.m_outputVertexCountsByStream[outputStreamIndex] += modelLodAssetMesh.GetVertexCount();
                            }
                        }

                        for (const RPI::ModelLodAsset::Mesh::StreamBufferInfo& streamBufferInfo : modelLodAssetMesh.GetStreamBufferInfoList())
                        {
                            // If it is not part of the skinning compute shader input or output, then it is a static buffer used for rendering instead of skinning
                            bool isStaticStream = !SkinnedMeshVertexStreamPropertyInterface::Get()->GetInputStreamInfo(streamBufferInfo.m_semantic)
                                && !SkinnedMeshVertexStreamPropertyInterface::Get()->GetOutputStreamInfo(streamBufferInfo.m_semantic);

                            // Also, if it is a color, and colors aren't modified for this mesh, it is static, not dynamic
                            isStaticStream |= streamBufferInfo.m_semantic == RHI::ShaderSemantic{ Name{"COLOR"} } && skinnedMeshLod.m_hasStaticColors;

                            if (isStaticStream)
                            {
                                skinnedMeshLod.m_meshes[meshIndex].m_staticBufferInfo.push_back(streamBufferInfo);

                                // If the buffer asset isn't already tracked by the lod from another mesh, add it here
                                if (AZStd::find(begin(skinnedMeshLod.m_staticBufferAssets), end(skinnedMeshLod.m_staticBufferAssets), streamBufferInfo.m_bufferAssetView.GetBufferAsset())
                                    == end(skinnedMeshLod.m_staticBufferAssets))
                                {
                                    skinnedMeshLod.m_staticBufferAssets.push_back(streamBufferInfo.m_bufferAssetView.GetBufferAsset());
                                }
                            }
                        }
                    }
                }
            }
        }

        Data::Asset<RPI::ModelAsset> SkinnedMeshInputBuffers::GetModelAsset() const
        {
            return m_modelAsset;
        }

        size_t SkinnedMeshInputBuffers::GetMeshCount(size_t lodIndex) const
        {
            return m_lods[lodIndex].m_meshes.size();
        }

        void SkinnedMeshInputBuffers::SetLodCount(size_t lodCount)
        {
            AZ_Assert(lodCount <= RPI::ModelLodAsset::LodCountMax, "Attempting to set lod count of %d in SkinnedMeshInputBuffers, "
                "which exceeds the maximum count of %d", lodCount, RPI::ModelLodAsset::LodCountMax);
            m_lods.resize(lodCount);
        }

        size_t SkinnedMeshInputBuffers::GetLodCount() const
        {
            return m_lods.size();
        }

        void SkinnedMeshInputBuffers::SetLod(size_t lodIndex, const SkinnedMeshInputLod& lod)
        {
            AZ_Assert(lodIndex < m_lods.size(), "Attempting to set lod at index %d in SkinnedMeshInputBuffers, which is outside the range of %zu. "
                "Make sure SetLodCount has been called before calling SetLod.", lodIndex, m_lods.size());
            m_lods[lodIndex] = lod;

            m_isUploadPending = true;
        }

        const SkinnedMeshInputLod& SkinnedMeshInputBuffers::GetLod(size_t lodIndex) const
        {
            AZ_Assert(lodIndex < m_lods.size(), "Attempting to get lod at index %d in SkinnedMeshInputBuffers, which is outside the range of %zu.", lodIndex, m_lods.size());
            return m_lods[lodIndex];
        }

        AZStd::array_view<AZ::RHI::Ptr<RHI::BufferView>> SkinnedMeshInputBuffers::GetInputBufferViews(size_t lodIndex) const
        {
            return m_lods[lodIndex].m_bufferViews;
        }

        AZ::RHI::Ptr<const RHI::BufferView> SkinnedMeshInputBuffers::GetInputBufferView(size_t lodIndex, uint8_t inputStream) const
        {
            return m_lods[lodIndex].m_inputBuffers[inputStream]->GetBufferView();
        }

        bool SkinnedMeshInputBuffers::HasDynamicColors(size_t lodIndex, [[maybe_unused]] size_t meshIndex) const
        {
            // TODO - treat each mesh individually
            return m_lods[lodIndex].HasDynamicColors();//m_hasDynamicColors[meshIndex];
        }

        uint32_t SkinnedMeshInputBuffers::GetVertexCount(size_t lodIndex, size_t meshIndex) const
        {
            return m_lods[lodIndex].m_meshes[meshIndex].m_vertexCount;
        }

        void SkinnedMeshInputBuffers::SetBufferViewsOnShaderResourceGroup(size_t lodIndex, size_t meshIndex, const Data::Instance<RPI::ShaderResourceGroup>& perInstanceSRG)
        {
            AZ_Assert(lodIndex < m_lods.size() && meshIndex < m_lods[lodIndex].m_modelLodAsset->GetMeshes().size(), "Lod %zu Mesh %zu out of range for model '%s'", lodIndex, meshIndex, m_modelAsset->GetName().GetCStr());

            // Loop over each input buffer view and set it on the srg
            for(const auto&[srgName, bufferView] : m_lods[lodIndex].m_meshes[meshIndex].m_inputBufferViews)
            {
                RHI::ShaderInputBufferIndex srgIndex = perInstanceSRG->FindShaderInputBufferIndex(srgName);
                AZ_Error("SkinnedMeshInputBuffers", srgIndex.IsValid(), "Failed to find shader input index for '%s' in the skinning compute shader per-instance SRG.", srgName.GetCStr());

                [[maybe_unused]] bool success = perInstanceSRG->SetBufferView(srgIndex, bufferView.get());

                AZ_Error("SkinnedMeshInputBuffers", success, "Failed to bind buffer view for %s", srgName.GetCStr());
            }

            // Set the vertex count
            RHI::ShaderInputConstantIndex numVerticesIndex;
            numVerticesIndex = perInstanceSRG->FindShaderInputConstantIndex(Name{ "m_numVertices" });
            AZ_Error("SkinnedMeshInputBuffers", numVerticesIndex.IsValid(), "Failed to find shader input index for m_numVerticies in the skinning compute shader per-instance SRG.");
            perInstanceSRG->SetConstant(numVerticesIndex, m_lods[lodIndex].m_modelLodAsset->GetMeshes()[meshIndex].GetVertexCount());
        }

        // Create a resource view that has a different type than the data it is viewing
        static RHI::BufferViewDescriptor CreateResourceViewWithDifferentFormat(size_t offsetInBytes, uint32_t realElementCount, uint32_t realElementSize,  RHI::Format format, RHI::BufferBindFlags overrideBindFlags)
        {
            RHI::BufferViewDescriptor viewDescriptor;
            size_t elementOffset = offsetInBytes / aznumeric_cast<size_t>(RHI::GetFormatSize(format));
            AZ_Assert(elementOffset <= std::numeric_limits<uint32_t>().max(), "The offset in bytes from the start of the SkinnedMeshOutputStream buffer is too large to be expressed as a uint32_t element offset in the BufferViewDescriptor.");
            viewDescriptor.m_elementOffset = aznumeric_cast<uint32_t>(elementOffset);

            viewDescriptor.m_elementCount = realElementCount * (realElementSize / RHI::GetFormatSize(format));
            viewDescriptor.m_elementFormat = format;
            viewDescriptor.m_elementSize = RHI::GetFormatSize(format);
            viewDescriptor.m_overrideBindFlags = overrideBindFlags;
            return viewDescriptor;
        }

        static bool AllocateLodStream(
            uint8_t outputStreamIndex,
            size_t vertexCount,
            AZStd::intrusive_ptr<SkinnedMeshInstance> instance,
            SkinnedMeshOutputVertexOffsets& streamOffsetsFromBufferStart,
            AZStd::vector<AZStd::intrusive_ptr<SkinnedMeshOutputStreamAllocation>>& lodAllocations)
        {
            const SkinnedMeshOutputVertexStreamInfo& outputStreamInfo = SkinnedMeshVertexStreamPropertyInterface::Get()->GetOutputStreamInfo(static_cast<SkinnedMeshOutputVertexStreams>(outputStreamIndex));

            // Positions use 2x the number of vertices to hold both the current frame and previous frame's data
            size_t positionMultiplier = static_cast<SkinnedMeshOutputVertexStreams>(outputStreamIndex) == SkinnedMeshOutputVertexStreams::Position ? 2u : 1u;
            AZStd::intrusive_ptr<SkinnedMeshOutputStreamAllocation> allocation = SkinnedMeshOutputStreamManagerInterface::Get()->Allocate(vertexCount * static_cast<size_t>(outputStreamInfo.m_elementSize) * positionMultiplier);
            if (!allocation)
            {
                // Suppress the OnMemoryFreed signal when releasing the previous successful allocations
                // The memory was already free before this function was called, so it's not really newly available memory
                AZ_Error("SkinnedMeshInputBuffers", false, "Out of memory to create a skinned mesh instance. Consider increasing r_skinnedMeshInstanceMemoryPoolSize");

                instance->m_allocations.push_back(lodAllocations);
                instance->SuppressSignalOnDeallocate();
                return false;
            }
            lodAllocations.push_back(allocation);
            streamOffsetsFromBufferStart[outputStreamIndex] = aznumeric_cast<uint32_t>(allocation->GetVirtualAddress().m_ptr);

            return true;
        }

        static bool AllocateMorphTargetsForLod(const SkinnedMeshInputLod& lod, size_t vertexCount, AZStd::intrusive_ptr<SkinnedMeshInstance> instance, AZStd::vector<AZStd::intrusive_ptr<SkinnedMeshOutputStreamAllocation>>& lodAllocations)
        {
            // If this skinned mesh lod has morph targets, allocate a buffer for the accumulated deltas that come from the morph target pass
            if (lod.GetMorphTargetMetaDatas().size() > 0)
            {
                // Naively, we're going to allocate enough memory to store the accumulated delta for every vertex.
                // This makes it simple for the skinning shader to index into the buffer, but the memory cost
                // could be reduced by keeping a buffer that maps from vertexId to morph target delta offset ATOM-14427

                // We're also using the skinned mesh output buffer, since it gives us a read-write pool of memory that can be
                // used for dependency tracking between passes. This can be switched to a transient memory pool so that the memory is free
                // later in the frame once skinning is finished ATOM-14429

                size_t perVertexSizeInBytes = static_cast<size_t>(MorphTargetConstants::s_unpackedMorphTargetDeltaSizeInBytes) * MorphTargetConstants::s_morphTargetDeltaTypeCount;
                if (lod.HasDynamicColors())
                {
                    // Naively, if colors are morphed by any of the morph targets,
                    // we'll allocate enough memory to store the accumulated color deltas for every vertex in the lod.
                    // This could be reduced by ATOM-14427
          
                    // We assume that the model has been padded to include colors even for the meshes which don't use them
                    // this could be reduced by dispatching the skinning shade
                    // for one mesh at a time instead of the entire lod at once ATOM-15078

                    // Add four floats for colors
                    perVertexSizeInBytes += 4 * sizeof(float);
                }

                AZStd::intrusive_ptr<SkinnedMeshOutputStreamAllocation> allocation = SkinnedMeshOutputStreamManagerInterface::Get()->Allocate(vertexCount * perVertexSizeInBytes);
                if (!allocation)
                {
                    // Suppress the OnMemoryFreed signal when releasing the previous successful allocations
                    // The memory was already free before this function was called, so it's not really newly available memory
                    AZ_Error("SkinnedMeshInputBuffers", false, "Out of memory to create a skinned mesh instance. Consider increasing r_skinnedMeshInstanceMemoryPoolSize");
                    instance->m_allocations.push_back(lodAllocations);
                    instance->SuppressSignalOnDeallocate();
                    return false;
                }
                else
                {
                    // We're using an offset into a global buffer to be able to access the morph target offsets in a bindless manner.
                    // The offset can at most be a 32-bit uint until AZSL supports 64-bit uints. This gives us a 4GB limit for where the
                    // morph target deltas can live. In practice, the offsets could end up outside that range even if less that 4GB is used
                    // if the memory becomes fragmented. To address it, we can split morph target deltas into their own buffer, allocate
                    // memory in pages with a buffer for each page, or create and bind a buffer view
                    // so we are not doing an offset from the beginning of the buffer
                    AZ_Error("SkinnedMeshInputBuffers", allocation->GetVirtualAddress().m_ptr < static_cast<uintptr_t>(std::numeric_limits<uint32_t>::max()), "Morph target deltas allocated from the skinned mesh memory pool are outside the range that can be accessed from the skinning shader");

                    MorphTargetInstanceMetaData instanceMetaData;

                    // Positions start at the beginning of the allocation
                    instanceMetaData.m_accumulatedPositionDeltaOffsetInBytes = static_cast<int32_t>(allocation->GetVirtualAddress().m_ptr);
                    uint32_t deltaStreamSizeInBytes = static_cast<uint32_t>(vertexCount * MorphTargetConstants::s_unpackedMorphTargetDeltaSizeInBytes);

                    // Followed by normals, tangents, and bitangents
                    instanceMetaData.m_accumulatedNormalDeltaOffsetInBytes = instanceMetaData.m_accumulatedPositionDeltaOffsetInBytes + deltaStreamSizeInBytes;
                    instanceMetaData.m_accumulatedTangentDeltaOffsetInBytes = instanceMetaData.m_accumulatedNormalDeltaOffsetInBytes + deltaStreamSizeInBytes;
                    instanceMetaData.m_accumulatedBitangentDeltaOffsetInBytes = instanceMetaData.m_accumulatedTangentDeltaOffsetInBytes + deltaStreamSizeInBytes;

                    // Followed by colors
                    if (lod.HasDynamicColors())
                    {
                        instanceMetaData.m_accumulatedColorDeltaOffsetInBytes = instanceMetaData.m_accumulatedBitangentDeltaOffsetInBytes + deltaStreamSizeInBytes;
                    }
                    else
                    {
                        instanceMetaData.m_accumulatedColorDeltaOffsetInBytes = MorphTargetConstants::s_invalidDeltaOffset;
                    }

                    // Track both the allocation and the metadata in the instance
                    instance->m_morphTargetInstanceMetaData.push_back(instanceMetaData);
                    lodAllocations.push_back(allocation);
                }
            }
            else
            {
                // No morph targets for this lod
                MorphTargetInstanceMetaData instanceMetaData{ MorphTargetConstants::s_invalidDeltaOffset, MorphTargetConstants::s_invalidDeltaOffset, MorphTargetConstants::s_invalidDeltaOffset, MorphTargetConstants::s_invalidDeltaOffset };
                instance->m_morphTargetInstanceMetaData.push_back(instanceMetaData);
            }

            return true;
        }


        static void AddSubMeshViewToModelLodCreator(
            uint8_t outputStreamIndex,
            uint32_t lodVertexCount,
            uint32_t submeshVertexCount,
            Data::Asset<RPI::BufferAsset> skinnedMeshOutputBufferAsset,
            const SkinnedMeshOutputVertexOffsets& streamOffsetsFromBufferStart,
            SkinnedMeshOutputVertexOffsets& subMeshOffsetsFromStreamStart,
            RPI::ModelLodAssetCreator& modelLodCreator)
        {
            const SkinnedMeshOutputVertexStreamInfo& outputStreamInfo = SkinnedMeshVertexStreamPropertyInterface::Get()->GetOutputStreamInfo(static_cast<SkinnedMeshOutputVertexStreams>(outputStreamIndex));

            // For the purpose of the model, which is fed to the static mesh feature processor, these buffer views are only going to be used as input assembly.
            // The underlying buffer is still writable and will be written to by the skinning shader.
            RHI::BufferViewDescriptor viewDescriptor = CreateResourceViewWithDifferentFormat(static_cast<size_t>(streamOffsetsFromBufferStart[outputStreamIndex]) + static_cast<size_t>(subMeshOffsetsFromStreamStart[outputStreamIndex]), submeshVertexCount, outputStreamInfo.m_elementSize, outputStreamInfo.m_elementFormat, RHI::BufferBindFlags::InputAssembly);

            AZ_Assert(streamOffsetsFromBufferStart[outputStreamIndex] % outputStreamInfo.m_elementSize == 0, "The SkinnedMeshOutputStreamManager is supposed to guarantee that offsets can always align.");

            RPI::BufferAssetView bufferView{ skinnedMeshOutputBufferAsset, viewDescriptor };
            modelLodCreator.AddMeshStreamBuffer(outputStreamInfo.m_semantic, AZ::Name(), bufferView);

            if (static_cast<SkinnedMeshOutputVertexStreams>(outputStreamIndex) == SkinnedMeshOutputVertexStreams::Position)
            {
                // Add stream buffer for position history
                size_t positionHistoryBufferOffsetInBytes = streamOffsetsFromBufferStart[outputStreamIndex] + subMeshOffsetsFromStreamStart[outputStreamIndex] + lodVertexCount * outputStreamInfo.m_elementSize;
                viewDescriptor.m_elementOffset = aznumeric_cast<uint32_t>(positionHistoryBufferOffsetInBytes / outputStreamInfo.m_elementSize);

                bufferView = { skinnedMeshOutputBufferAsset, viewDescriptor };
                modelLodCreator.AddMeshStreamBuffer(RHI::ShaderSemantic{ Name{"POSITIONT"} }, AZ::Name(), bufferView);
            }

            subMeshOffsetsFromStreamStart[outputStreamIndex] += viewDescriptor.m_elementCount * viewDescriptor.m_elementSize;
        }

        AZStd::intrusive_ptr<SkinnedMeshInstance> SkinnedMeshInputBuffers::CreateSkinnedMeshInstance() const
        {
            // This function creates a SkinnedMeshInstance which describes all the buffer views needed to write the output of the skinned mesh compute shader
            // and a model which can be rendered by the MeshFeatureProcessor

            // Static data that doesn't get modified during skinning (e.g. index buffer, uvs) is shared between all instances that use the same SkinnedMeshInputBuffers
            // The buffers for this static data and the per sub-mesh views into these buffers were created when the SkinnedMeshInputBuffers was created.
            // This function adds those views to the model when creating it

            // For the output of the skinned mesh shader, each instance has unique vertex data that exists in a single buffer managed by the SkinnedMeshOutputStreamManager
            // For a given stream all of the vertices for an entire lod is contiguous in memory, allowing the entire lod to be skinned at once in as part of a single dispatch
            // The streams are de-interleaved, and each stream may reside independently within the output buffer as determined by the best fit allocator
            // E.g. the positions may or may not be adjacent to normals, but all of the positions for a single lod with be contiguous

            // To support multiple sub-meshes, views into each stream for each lod are created for the sub-meshes

            //   SkinnedMeshOutputBuffer[.....................................................................................................................................]
            //            lod0 Positions[^                         ^]             lod0Normals[^                         ^]   lod1Positions[^     ^]     lod1Normals[^     ^]
            // lod0 subMesh0+1 Positions[^             ^^          ^] lod0 subMesh0+1 Normals[^             ^^          ^]  lod1 sm0+1 pos[^  ^^ ^] lod1 sm0+1 norm[^  ^^ ^]

            AZ_PROFILE_FUNCTION(AzRender);
            AZStd::intrusive_ptr<SkinnedMeshInstance> instance = aznew SkinnedMeshInstance;

            // Each model gets a unique, random ID, so if the same source model is used for multiple instances, multiple target models will be created.
            RPI::ModelAssetCreator modelCreator;
            modelCreator.Begin(Uuid::CreateRandom());

            // Use the name from the original model
            modelCreator.SetName(m_modelAsset->GetName().GetStringView());

            Data::Asset<RPI::BufferAsset> skinnedMeshOutputBufferAsset = SkinnedMeshOutputStreamManagerInterface::Get()->GetBufferAsset();

            size_t lodIndex = 0;
            for (const SkinnedMeshInputLod& lod : m_lods)
            {
                RPI::ModelLodAssetCreator modelLodCreator;
                modelLodCreator.Begin(Data::AssetId(Uuid::CreateRandom()));

                //
                // Lod
                //
                Data::Asset<RPI::ModelLodAsset> inputLodAsset = m_modelAsset->GetLodAssets()[lodIndex];

                // Add a reference to the shared index buffer
                modelLodCreator.AddLodStreamBuffer(inputLodAsset->GetIndexBufferAsset());

                // There is only one underlying buffer that houses all of the skinned mesh output streams for all skinned mesh instances
                modelLodCreator.AddLodStreamBuffer(skinnedMeshOutputBufferAsset);

                // Add any shared static buffers
                for (const Data::Asset<RPI::BufferAsset>& staticBufferAsset : lod.m_staticBufferAssets)
                {
                    modelLodCreator.AddLodStreamBuffer(staticBufferAsset);
                }

                // Track offsets for each stream, so that the sub-meshes know where to begin
                SkinnedMeshOutputVertexOffsets streamOffsetsFromBufferStart = {0};
                AZStd::vector<AZStd::intrusive_ptr<SkinnedMeshOutputStreamAllocation>> lodAllocations;

                // The skinning shader doesn't differentiate between sub-meshes, it just writes all the vertices at once.
                // So we want to pack all the positions for each sub-mesh together, all the normals together, etc.
                for (uint8_t outputStreamIndex = 0; outputStreamIndex < static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams); ++outputStreamIndex)
                {
                    // Skip colors if they don't exist or are not being morphed
                    if (outputStreamIndex == static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::Color) && !lod.m_hasDynamicColors)
                    {
                        continue;
                    }

                    if (!AllocateLodStream(outputStreamIndex, lod.m_outputVertexCountsByStream[outputStreamIndex], instance, streamOffsetsFromBufferStart, lodAllocations))
                    {
                        return nullptr;
                    }
                }

                if (!AllocateMorphTargetsForLod(lod, lod.m_vertexCount, instance, lodAllocations))
                {
                    return nullptr;
                }

                instance->m_allocations.push_back(lodAllocations);

                //
                // Submesh
                //

                AZStd::vector<SkinnedMeshOutputVertexOffsets> meshOffsetsFromBufferStartInBytes;
                meshOffsetsFromBufferStartInBytes.reserve(lod.m_meshes.size());

                AZStd::vector<uint32_t> meshPositionHistoryBufferOffsetsInBytes;
                meshPositionHistoryBufferOffsetsInBytes.reserve(lod.m_meshes.size());

                SkinnedMeshOutputVertexOffsets currentMeshOffsetsFromStreamStartInBytes = {0};
                // Iterate over each sub-mesh for the lod to create views into the buffers
                for (size_t i = 0; i < lod.m_meshes.size(); ++i)
                {
                    modelLodCreator.BeginMesh();

                    // Set the index buffer view
                    const RPI::ModelLodAsset::Mesh& inputMesh = lod.m_modelLodAsset->GetMeshes()[i];
                    modelLodCreator.SetMeshIndexBuffer(inputMesh.GetIndexBufferAssetView());

                    // Track the offsets from the start of the global output buffer
                    // for the current mesh to feed to the skinning shader so it
                    // knows where to write to
                    SkinnedMeshOutputVertexOffsets currentMeshOffsetsFromBufferStartInBytes = {0};
                    for (uint8_t outputStreamIndex = 0; outputStreamIndex < static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams); ++outputStreamIndex)
                    {
                        currentMeshOffsetsFromBufferStartInBytes[outputStreamIndex] = streamOffsetsFromBufferStart[outputStreamIndex] + currentMeshOffsetsFromStreamStartInBytes[outputStreamIndex];
                    }
                    meshOffsetsFromBufferStartInBytes.push_back(currentMeshOffsetsFromBufferStartInBytes);

                    // Track the offset for the position history buffer
                    uint32_t meshPositionHistoryBufferOffsetInBytes =
                        currentMeshOffsetsFromBufferStartInBytes[static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::Position)] +
                        lod.GetVertexCountForStream(SkinnedMeshOutputVertexStreams::Position) *
                            SkinnedMeshVertexStreamPropertyInterface::Get()
                                ->GetOutputStreamInfo(SkinnedMeshOutputVertexStreams::Position)
                                .m_elementSize;
                    meshPositionHistoryBufferOffsetsInBytes.push_back(meshPositionHistoryBufferOffsetInBytes);

                    // Create and set the views into the skinning output buffers
                    for (uint8_t outputStreamIndex = 0; outputStreamIndex < static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams); ++outputStreamIndex)
                    {
                        // Skip colors if they don't exist or are not being morphed
                        if (outputStreamIndex == static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::Color) && !lod.m_hasDynamicColors)
                        {
                            continue;
                        }
                        
                        // Add a buffer view to the output model so it knows where to read the final skinned vertex data from
                        AddSubMeshViewToModelLodCreator(outputStreamIndex, lod.m_outputVertexCountsByStream[static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::Position)], lod.m_meshes[i].m_vertexCount, skinnedMeshOutputBufferAsset, streamOffsetsFromBufferStart, currentMeshOffsetsFromStreamStartInBytes, modelLodCreator);
                    }

                    // Set the views into the static buffers
                    for (const RPI::ModelLodAsset::Mesh::StreamBufferInfo& staticBufferInfo : lod.m_meshes[i].m_staticBufferInfo)
                    {
                        modelLodCreator.AddMeshStreamBuffer(staticBufferInfo.m_semantic, staticBufferInfo.m_customName, staticBufferInfo.m_bufferAssetView);
                    }

                    Aabb localAabb = inputMesh.GetAabb();
                    modelLodCreator.SetMeshAabb(AZStd::move(localAabb));

                    modelCreator.AddMaterialSlot(m_modelAsset->FindMaterialSlot(inputMesh.GetMaterialSlotId()));
                    modelLodCreator.SetMeshMaterialSlot(inputMesh.GetMaterialSlotId());

                    modelLodCreator.EndMesh();
                }

                // Add all the mesh offsets for the lod
                instance->m_outputStreamOffsetsInBytes.push_back(meshOffsetsFromBufferStartInBytes);
                instance->m_positionHistoryBufferOffsetsInBytes.push_back(meshPositionHistoryBufferOffsetsInBytes);

                Data::Asset<RPI::ModelLodAsset> lodAsset;
                modelLodCreator.End(lodAsset);
                if (!lodAsset.IsReady())
                {
                    // [GFX TODO] During mesh reload the modelLodCreator could report errors and result in the lodAsset not ready.
                    return nullptr;
                }
                modelCreator.AddLodAsset(AZStd::move(lodAsset));

                lodIndex++;
            }

            Data::Asset<RPI::ModelAsset> modelAsset;
            modelCreator.End(modelAsset);

            instance->m_model = RPI::Model::FindOrCreate(modelAsset);
            return instance;
        }

        void SkinnedMeshInputBuffers::WaitForUpload()
        {
            if (m_isUploadPending)
            {
                for (SkinnedMeshInputLod& lod : m_lods)
                {
                    lod.WaitForUpload();
                }
                m_isUploadPending = false;
            }
        }

        bool SkinnedMeshInputBuffers::IsUploadPending() const
        {
            return m_isUploadPending;
        }
    } // namespace Render
}// namespace AZ
