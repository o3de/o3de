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

        void SkinnedMeshInputLod::SetIndexCount(uint32_t indexCount)
        {
            m_indexCount = indexCount;
        }

        void SkinnedMeshInputLod::SetVertexCount(uint32_t vertexCount)
        {
            m_vertexCount = vertexCount;
        }

        uint32_t SkinnedMeshInputLod::GetVertexCount() const
        {
            return m_vertexCount;
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

        void SkinnedMeshInputLod::SetModelLodAsset(const Data::Asset<RPI::ModelLodAsset>& modelLodAsset)
        {
            m_modelLodAsset = modelLodAsset;
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

            m_staticBufferAssets[static_cast<uint8_t>(staticStream)] = bufferAsset;
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
            m_staticBufferAssets[static_cast<uint8_t>(staticStream)] = bufferAsset;
            m_staticBuffers[static_cast<uint8_t>(staticStream)] = RPI::Buffer::FindOrCreate(bufferAsset);
        }

        void SkinnedMeshInputLod::SetSubMeshProperties(const AZStd::vector<SkinnedSubMeshProperties>& subMeshProperties)
        {
            m_subMeshProperties = subMeshProperties;
            CreateSharedSubMeshBufferViews();
        }

        const AZStd::vector<SkinnedSubMeshProperties>& SkinnedMeshInputLod::GetSubMeshProperties() const
        {
            return m_subMeshProperties;
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

        void SkinnedMeshInputLod::CreateSharedSubMeshBufferViews()
        {
            AZStd::array_view<RPI::ModelLodAsset::Mesh> meshes = m_modelLodAsset->GetMeshes();
            m_sharedSubMeshViews.resize(meshes.size());

            // The index and static buffer views will be shared by all instances that use the same SkinnedMeshInputBuffers, so set them here
            for (size_t i = 0; i < meshes.size(); ++i)
            {                
                // Set the view into the index buffer
                m_sharedSubMeshViews[i].m_indexBufferView = meshes[i].GetIndexBufferAssetView();

                // Set the views into the static buffers
                for (uint8_t staticStreamIndex = 0; staticStreamIndex < static_cast<uint8_t>(SkinnedMeshStaticVertexStreams::NumVertexStreams); ++staticStreamIndex)
                {
                    // Skip colors if they don't exist or are dynamic
                    if (staticStreamIndex == static_cast<uint8_t>(SkinnedMeshStaticVertexStreams::Color) && !m_hasStaticColors)
                    {
                        continue;
                    }

                    const SkinnedMeshVertexStreamInfo& streamInfo = SkinnedMeshVertexStreamPropertyInterface::Get()->GetStaticStreamInfo(static_cast<SkinnedMeshStaticVertexStreams>(staticStreamIndex));
                    const RPI::BufferAssetView* bufferView = meshes[i].GetSemanticBufferAssetView(streamInfo.m_semantic.m_name);
                    if (bufferView)
                    {
                        m_sharedSubMeshViews[i].m_staticStreamViews[staticStreamIndex] = bufferView;
                    }
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

        void SkinnedMeshInputBuffers::SetAssetId(Data::AssetId assetId)
        {
            m_assetId = assetId;
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

        uint32_t SkinnedMeshInputBuffers::GetVertexCount(size_t lodIndex) const
        {
            return m_lods[lodIndex].m_vertexCount;
        }

        void SkinnedMeshInputBuffers::SetBufferViewsOnShaderResourceGroup(size_t lodIndex, const Data::Instance<RPI::ShaderResourceGroup>& perInstanceSRG)
        {
            // Get the SRG indices for each input stream
            for (uint8_t inputStream = 0; inputStream < static_cast<uint8_t>(SkinnedMeshInputVertexStreams::NumVertexStreams); ++inputStream)
            {
                // Skip colors if they don't exist or are not being morphed
                if (inputStream == static_cast<uint8_t>(SkinnedMeshInputVertexStreams::Color) && !m_lods[lodIndex].m_hasDynamicColors)
                {
                    continue;
                }

                const SkinnedMeshVertexStreamInfo& streamInfo = SkinnedMeshVertexStreamPropertyInterface::Get()->GetInputStreamInfo(static_cast<SkinnedMeshInputVertexStreams>(inputStream));
                RHI::ShaderInputBufferIndex srgIndex = perInstanceSRG->FindShaderInputBufferIndex(streamInfo.m_shaderResourceGroupName);
                AZ_Error("SkinnedMeshInputBuffers", srgIndex.IsValid(), "Failed to find shader input index for '%s' in the skinning compute shader per-instance SRG.", streamInfo.m_shaderResourceGroupName.GetCStr());

                [[maybe_unused]] bool success = false;
                if (m_lods[lodIndex].m_inputBuffers[inputStream])
                {
                    success = perInstanceSRG->SetBufferView(srgIndex, m_lods[lodIndex].m_bufferViews[inputStream].get());
                }

                AZ_Error("SkinnedMeshInputBuffers", success, "Failed to bind buffer view for %s", streamInfo.m_bufferName.GetCStr());
            }

            // Set the vertex count
            RHI::ShaderInputConstantIndex numVerticesIndex;
            numVerticesIndex = perInstanceSRG->FindShaderInputConstantIndex(Name{ "m_numVertices" });
            AZ_Error("SkinnedMeshInputBuffers", numVerticesIndex.IsValid(), "Failed to find shader input index for m_numVerticies in the skinning compute shader per-instance SRG.");
            perInstanceSRG->SetConstant(numVerticesIndex, m_lods[lodIndex].m_vertexCount);
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
            AZStd::vector<uint32_t>& streamOffsetsFromBufferStart,
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
            streamOffsetsFromBufferStart.push_back(aznumeric_cast<uint32_t>(allocation->GetVirtualAddress().m_ptr));

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
            const AZStd::vector<uint32_t>& streamOffsetsFromBufferStart,
            AZStd::vector<size_t>& subMeshOffsetsFromStreamStart,
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

            AZ_PROFILE_SCOPE(AzRender, "SkinnedMeshInputBuffers: CreateSkinnedMeshInstance");
            AZStd::intrusive_ptr<SkinnedMeshInstance> instance = aznew SkinnedMeshInstance;

            // Each model gets a unique, random ID, so if the same source model is used for multiple instances, multiple target models will be created.
            RPI::ModelAssetCreator modelCreator;
            modelCreator.Begin(Uuid::CreateRandom());

            // Using the filename as a name for the model
            AZStd::string assetPath;
            Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &Data::AssetCatalogRequests::GetAssetPathById, m_assetId);
            AZStd::string fullFileName;
            AzFramework::StringFunc::Path::GetFullFileName(assetPath.c_str(), fullFileName);

            modelCreator.SetName(fullFileName + "_SkinnedMeshOutput");

            Data::Asset<RPI::BufferAsset> skinnedMeshOutputBufferAsset = SkinnedMeshOutputStreamManagerInterface::Get()->GetBufferAsset();

            size_t lodIndex = 0;
            for (const SkinnedMeshInputLod& lod : m_lods)
            {
                RPI::ModelLodAssetCreator modelLodCreator;
                modelLodCreator.Begin(Data::AssetId(Uuid::CreateRandom()));

                //
                // Lod
                //

                // Add a reference to the shared index buffer
                modelLodCreator.AddLodStreamBuffer(lod.m_indexBufferAsset);

                // There is only one underlying buffer that houses all of the skinned mesh output streams for all skinned mesh instances
                modelLodCreator.AddLodStreamBuffer(skinnedMeshOutputBufferAsset);

                // Add references to the shared static buffers
                // Only uv0 for now
                modelLodCreator.AddLodStreamBuffer(lod.m_staticBufferAssets[static_cast<uint8_t>(SkinnedMeshStaticVertexStreams::UV_0)]);

                // Track offsets for each stream, so that the sub-meshes know where to begin
                AZStd::vector<uint32_t> streamOffsetsFromBufferStart;
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

                    if (!AllocateLodStream(outputStreamIndex, aznumeric_cast<size_t>(lod.m_vertexCount), instance, streamOffsetsFromBufferStart, lodAllocations))
                    {
                        return nullptr;
                    }
                }

                if (!AllocateMorphTargetsForLod(lod, lod.m_vertexCount, instance, lodAllocations))
                {
                    return nullptr;
                }

                instance->m_outputStreamOffsetsInBytes.push_back(streamOffsetsFromBufferStart);
                instance->m_allocations.push_back(lodAllocations);

                //
                // Submesh
                //

                AZStd::vector<size_t> subMeshOffsetsFromStreamStart(streamOffsetsFromBufferStart.size(), 0);

                // Iterate over each sub-mesh for the lod to create views into the buffers
                AZ_Assert(lod.m_subMeshProperties.size() == lod.m_sharedSubMeshViews.size(), "Skinned sub-mesh property and view vectors are mis-matched in size.")
                for (size_t i = 0; i < lod.m_subMeshProperties.size(); ++i)
                {
                    modelLodCreator.BeginMesh();

                    // Set the index buffer view
                    modelLodCreator.SetMeshIndexBuffer(lod.m_sharedSubMeshViews[i].m_indexBufferView);

                    // Create and set the views into the skinning output buffers
                    for (uint8_t outputStreamIndex = 0; outputStreamIndex < static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams); ++outputStreamIndex)
                    {
                        // Skip colors if they don't exist or are not being morphed
                        if (outputStreamIndex == static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::Color) && !lod.m_hasDynamicColors)
                        {
                            continue;
                        }
                        AddSubMeshViewToModelLodCreator(outputStreamIndex, lod.m_vertexCount, lod.m_subMeshProperties[i].m_vertexCount, skinnedMeshOutputBufferAsset, streamOffsetsFromBufferStart, subMeshOffsetsFromStreamStart, modelLodCreator);
                    }                    

                    // Set the views into the static buffers
                    for (uint8_t staticStreamIndex = 0; staticStreamIndex < static_cast<uint8_t>(SkinnedMeshStaticVertexStreams::NumVertexStreams); ++staticStreamIndex)
                    {
                        // Skip colors if they don't exist or are dynamic
                        if (!lod.m_sharedSubMeshViews[i].m_staticStreamViews[staticStreamIndex]
                            || (staticStreamIndex == static_cast<uint8_t>(SkinnedMeshStaticVertexStreams::Color) && !lod.m_hasStaticColors))
                        {
                            continue;
                        }

                        const SkinnedMeshVertexStreamInfo& staticStreamInfo = SkinnedMeshVertexStreamPropertyInterface::Get()->GetStaticStreamInfo(static_cast<SkinnedMeshStaticVertexStreams>(staticStreamIndex));
                        modelLodCreator.AddMeshStreamBuffer(staticStreamInfo.m_semantic, AZ::Name(), *lod.m_sharedSubMeshViews[i].m_staticStreamViews[staticStreamIndex]);
                    }

                    Aabb localAabb = lod.m_subMeshProperties[i].m_aabb;
                    modelLodCreator.SetMeshAabb(AZStd::move(localAabb));

                    modelCreator.AddMaterialSlot(lod.m_subMeshProperties[i].m_materialSlot);
                    modelLodCreator.SetMeshMaterialSlot(lod.m_subMeshProperties[i].m_materialSlot.m_stableId);

                    modelLodCreator.EndMesh();
                }

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
