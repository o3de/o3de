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
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RHI/Factory.h>

#include <AzCore/std/algorithm.h>
#include <AzCore/Math/PackedVector3.h>
#include <inttypes.h>

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

        RHI::BufferViewDescriptor SkinnedMeshInputLod::CreateInputViewDescriptor(SkinnedMeshInputVertexStreams inputStream, RHI::Format elementFormat, const RHI::StreamBufferView &streamBufferView)
        {
            RHI::BufferViewDescriptor descriptor;
            uint32_t elementOffset = streamBufferView.GetByteOffset() / streamBufferView.GetByteStride();
            uint32_t elementCount = streamBufferView.GetByteCount() / streamBufferView.GetByteStride();

            if (inputStream == SkinnedMeshInputVertexStreams::BlendIndices)
            {
                // Create a descriptor for a raw view from the StreamBufferView
                descriptor = RHI::BufferViewDescriptor::CreateRaw(streamBufferView.GetByteOffset(), streamBufferView.GetByteCount());
            }
            else if (elementFormat == RHI::Format::R32G32B32_FLOAT)
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
                    RHI::BufferViewDescriptor::CreateTyped(elementOffset, elementCount, elementFormat);
            }

            return descriptor;
        }

        SkinnedMeshInputLod::HasInputStreamArray SkinnedMeshInputLod::CreateInputBufferViews(
            uint32_t lodIndex,
            uint32_t meshIndex,
            const RHI::InputStreamLayout& inputLayout,
            RPI::ModelLod::Mesh& mesh,
            const RHI::StreamBufferIndices& streamIndices,
            const char* modelName)
        {
            SkinnedSubMeshProperties& skinnedSubMesh = m_meshes[meshIndex];
            const auto modelLodAssetMeshes = m_modelLodAsset->GetMeshes();
            const RPI::ModelLodAsset::Mesh& modelLodAssetMesh = modelLodAssetMeshes[meshIndex];

            // Keep track of whether or not an input stream exists
            HasInputStreamArray meshHasInputStream{ false };

            // Loop variables
            auto streamIter = mesh.CreateStreamIterator(streamIndices);
            u8 meshStreamIndex = 0;

            // Create a buffer view for each input stream in the current mesh
            for (; !streamIter.HasEnded(); ++streamIter, ++meshStreamIndex)
            {
                // `IsValid` would return false for dummy buffers, since the index of those buffers equals the size of the buffer view.
                // We shall skip dummy buffers to avoid empty pointers in the following code. 
                if (!streamIter.IsValid())
                {
                    continue;
                }

                // Get the semantic from the input layout, and use that to get the SkinnedMeshStreamInfo
                const SkinnedMeshVertexStreamInfo* streamInfo = SkinnedMeshVertexStreamPropertyInterface::Get()->GetInputStreamInfo(
                    inputLayout.GetStreamChannels()[meshStreamIndex].m_semantic);

                const RHI::StreamBufferView& streamBufferView = *streamIter;
                if (streamInfo && streamBufferView.GetByteCount() > 0)
                {
                    RHI::BufferViewDescriptor descriptor =
                        CreateInputViewDescriptor(streamInfo->m_enum, streamInfo->m_elementFormat, streamBufferView);

                    AZ::RHI::Ptr<AZ::RHI::BufferView> bufferView = const_cast<RHI::Buffer*>(streamBufferView.GetBuffer())->BuildBufferView(descriptor);
                    {
                        // Initialize the buffer view
                        AZStd::string bufferViewName = AZStd::string::format(
                            "%s_lod%" PRIu32 "_mesh%" PRIu32 "_%s", modelName, lodIndex, meshIndex,
                            streamInfo->m_shaderResourceGroupName.GetCStr());
                        bufferView->SetName(Name(bufferViewName));

                        // Keep track of which streams exist for the current mesh
                        meshHasInputStream[static_cast<uint8_t>(streamInfo->m_enum)] = true;
                    }

                    // Add the buffer view along with the shader resource group name, which will be used to bind it to the srg later
                    skinnedSubMesh.m_inputBufferViews.push_back(
                        SkinnedSubMeshProperties::SrgNameViewPair{ streamInfo->m_shaderResourceGroupName, bufferView });

                    if (streamInfo->m_enum == SkinnedMeshInputVertexStreams::BlendWeights)
                    {
                        uint32_t elementCount = streamBufferView.GetByteCount() / streamBufferView.GetByteStride();
                        skinnedSubMesh.m_skinInfluenceCountPerVertex = elementCount / modelLodAssetMesh.GetVertexCount();
                    }
                }
            }

            return meshHasInputStream;
        }

        void SkinnedMeshInputLod::CreateOutputOffsets(
            uint32_t meshIndex,
            const HasInputStreamArray& meshHasInputStream,
            SkinnedMeshOutputVertexOffsets& currentMeshOffsetFromStreamStart)
        {
            SkinnedSubMeshProperties& skinnedSubMesh = m_meshes[meshIndex];
            const auto modelLodAssetMeshes = m_modelLodAsset->GetMeshes();
            const RPI::ModelLodAsset::Mesh& modelLodAssetMesh = modelLodAssetMeshes[meshIndex];

            for (uint8_t outputStreamIndex = 0; outputStreamIndex < static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams);
                 ++outputStreamIndex)
            {
                const SkinnedMeshOutputVertexStreamInfo& outputStreamInfo =
                    SkinnedMeshVertexStreamPropertyInterface::Get()->GetOutputStreamInfo(
                        static_cast<SkinnedMeshOutputVertexStreams>(outputStreamIndex));

                // If there is no input to be skinned, then we won't need to bind the output stream
                if (meshHasInputStream[static_cast<uint8_t>(outputStreamInfo.m_correspondingInputVertexStream)])
                {
                    // Keep track of the offset for the individual mesh
                    skinnedSubMesh.m_vertexOffsetsFromStreamStartInBytes[outputStreamIndex] =
                        currentMeshOffsetFromStreamStart[outputStreamIndex];
                    currentMeshOffsetFromStreamStart[outputStreamIndex] +=
                        modelLodAssetMesh.GetVertexCount() * outputStreamInfo.m_elementSize;
                    // Keep track of the total for the whole lod
                    m_outputVertexCountsByStream[outputStreamIndex] += modelLodAssetMesh.GetVertexCount();
                }
            }
        }

        void SkinnedMeshInputLod::TrackStaticBufferViews(uint32_t meshIndex)
        {
            SkinnedSubMeshProperties& skinnedSubMesh = m_meshes[meshIndex];
            const auto modelLodAssetMeshes = m_modelLodAsset->GetMeshes();
            const RPI::ModelLodAsset::Mesh& modelLodAssetMesh = modelLodAssetMeshes[meshIndex];

            for (const RPI::ModelLodAsset::Mesh::StreamBufferInfo& streamBufferInfo : modelLodAssetMesh.GetStreamBufferInfoList())
            {
                // If it is not part of the skinning compute shader input or output, then it is a static buffer used for rendering instead
                // of skinning
                bool isStaticStream = !SkinnedMeshVertexStreamPropertyInterface::Get()->GetInputStreamInfo(streamBufferInfo.m_semantic) &&
                    !SkinnedMeshVertexStreamPropertyInterface::Get()->GetOutputStreamInfo(streamBufferInfo.m_semantic);

                if (isStaticStream)
                {
                    skinnedSubMesh.m_staticBufferInfo.push_back(streamBufferInfo);

                    // If the buffer asset isn't already tracked by the lod from another mesh, add it here
                    if (AZStd::find(
                            begin(m_staticBufferAssets), end(m_staticBufferAssets), streamBufferInfo.m_bufferAssetView.GetBufferAsset()) ==
                        end(m_staticBufferAssets))
                    {
                        m_staticBufferAssets.push_back(streamBufferInfo.m_bufferAssetView.GetBufferAsset());
                    }
                }
            }
        }

        void SkinnedMeshInputLod::CreateFromModelLod(
            const Data::Asset<RPI::ModelAsset>& modelAsset, const Data::Instance<RPI::Model>& model, uint32_t lodIndex)
        {
            m_modelLodAsset = modelAsset->GetLodAssets()[lodIndex];
            const auto modelLods = model->GetLods();
            const Data::Instance<RPI::ModelLod>& modelLod = modelLods[lodIndex];

            // Collect the vertex count for each output stream
            m_outputVertexCountsByStream = SkinnedMeshOutputVertexCounts{ 0 };
            SkinnedMeshOutputVertexOffsets currentMeshOffsetFromStreamStart = { 0 };

            m_meshes.resize(modelLod->GetMeshes().size());
            for (uint32_t meshIndex = 0; meshIndex < modelLod->GetMeshes().size(); ++meshIndex)
            {
                RPI::ModelLod::Mesh mesh = modelLod->GetMeshes()[meshIndex];
                SkinnedSubMeshProperties& skinnedSubMesh = m_meshes[meshIndex];
                skinnedSubMesh.m_vertexOffsetsFromStreamStartInBytes = SkinnedMeshOutputVertexOffsets{ 0 };

                // Get the source mesh
                const auto modelLodAssetMeshes = m_modelLodAsset->GetMeshes();
                const RPI::ModelLodAsset::Mesh& modelLodAssetMesh = modelLodAssetMeshes[meshIndex];
                skinnedSubMesh.m_vertexCount = modelLodAssetMesh.GetVertexCount();

                // Get all of the streams potentially used as input to the skinning compute shader
                RHI::InputStreamLayout inputLayout;
                RHI::StreamBufferIndices streamIndices;
                [[maybe_unused]] bool success = modelLod->GetStreamsForMesh(
                    inputLayout, streamIndices, nullptr,
                    SkinnedMeshVertexStreamPropertyInterface::Get()->GetComputeShaderInputContract(), meshIndex);

                AZ_Assert( success, "SkinnedMeshInputLod failed to get Streams for model '%s'", modelAsset.GetHint().c_str());

                HasInputStreamArray meshHasInputStream = CreateInputBufferViews(lodIndex, meshIndex, inputLayout,
                    mesh, streamIndices, modelAsset->GetName().GetCStr());

                CreateOutputOffsets(meshIndex, meshHasInputStream, currentMeshOffsetFromStreamStart);

                TrackStaticBufferViews(meshIndex);
            }
        }

        Data::Asset<RPI::ModelLodAsset> SkinnedMeshInputLod::GetModelLodAsset() const
        {
            return m_modelLodAsset;
        }

        uint32_t SkinnedMeshInputLod::GetVertexCount() const
        {
            return m_outputVertexCountsByStream[aznumeric_caster(SkinnedMeshOutputVertexStreams::Position)];
        }

        void SkinnedMeshInputLod::AddMorphTarget(
            const RPI::MorphTargetMetaAsset::MorphTarget& morphTarget,
            const RPI::BufferAssetView* morphBufferAssetView,
            const AZStd::string& bufferNamePrefix,
            float minWeight = 0.0f,
            float maxWeight = 1.0f)
        {
            m_morphTargetComputeMetaDatas.push_back(MorphTargetComputeMetaData{
                minWeight, maxWeight, morphTarget.m_minPositionDelta, morphTarget.m_maxPositionDelta, morphTarget.m_numVertices, morphTarget.m_meshIndex });

            // Create a view into the larger per-lod morph buffer for this particular morph
            // The morphTarget itself refers to an offset from within the mesh, so combine that
            // with the mesh offset to get the view within the lod buffer
            RHI::BufferViewDescriptor morphView = morphBufferAssetView->GetBufferViewDescriptor();
            morphView.m_elementOffset += morphTarget.m_startIndex;
            morphView.m_elementCount = morphTarget.m_numVertices;
            RPI::BufferAssetView morphTargetDeltaView{ morphBufferAssetView->GetBufferAsset(), morphView };

            m_morphTargetInputBuffers.push_back(aznew MorphTargetInputBuffers{ morphTargetDeltaView, bufferNamePrefix });
        }

        const AZStd::vector<MorphTargetComputeMetaData>& SkinnedMeshInputLod::GetMorphTargetComputeMetaDatas() const
        {
            return m_morphTargetComputeMetaDatas;
        }

        const AZStd::vector<AZStd::intrusive_ptr<MorphTargetInputBuffers>>& SkinnedMeshInputLod::GetMorphTargetInputBuffers() const
        {
            return m_morphTargetInputBuffers;
        }

        void SkinnedMeshInputLod::CalculateMorphTargetIntegerEncodings()
        {
            AZStd::vector<float> ranges(m_meshes.size(), 0.0f);

            // The accumulation buffer must be stored as an int to support InterlockedAdd in AZSL
            // Conservatively determine the largest value, positive or negative across the entire skinned mesh lod, which is used for encoding/decoding the accumulation buffer
            for (const MorphTargetComputeMetaData& metaData : m_morphTargetComputeMetaDatas)
            {
                float maxWeight = AZStd::max(std::abs(metaData.m_minWeight), std::abs(metaData.m_maxWeight));
                float maxDelta = AZStd::max(std::abs(metaData.m_minDelta), std::abs(metaData.m_maxDelta));
                // Normal, Tangent, and Bitangent deltas can be as high as 2
                maxDelta = AZStd::max(maxDelta, 2.0f);
                // Since multiple morphs can be fully active at once, sum the maximum offset in either positive or negative direction
                // that can be applied each individual morph to get the maximum offset that could be applied across all morphs
                ranges[metaData.m_meshIndex] += maxWeight * maxDelta;
            }

            // Calculate the final encoding value
            for (size_t i = 0; i < ranges.size(); ++i)
            {
                if (ranges[i] < std::numeric_limits<float>::epsilon())
                {
                    // There are no morph targets for this mesh
                    ranges[i] = -1.0f;
                }
                else
                {
                    // Given a conservative maximum value of a delta (minimum if negated), set a value for encoding a float as an integer that maximizes precision
                    // while still being able to represent the entire range of possible offset values for this instance
                    // For example, if at most all the deltas accumulated fell between a -1 and 1 range, we'd encode it as an integer by multiplying it by 2,147,483,647.
                    // If the delta has a larger range, we multiply it by a smaller number, increasing the range of representable values but decreasing the precision
                    m_meshes[i].m_morphTargetIntegerEncoding = static_cast<float>(std::numeric_limits<int>::max()) / ranges[i];
                }
            }
        }

        bool SkinnedMeshInputLod::HasMorphTargetsForMesh(uint32_t meshIndex) const
        {
            return m_meshes[meshIndex].m_morphTargetIntegerEncoding > 0.0f;
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
                for (uint32_t lodIndex = 0; lodIndex < m_model->GetLodCount(); ++lodIndex)
                {
                    // Add a new lod to the SkinnedMeshInputBuffers
                    SkinnedMeshInputLod& skinnedMeshLod = m_lods[lodIndex];
                    skinnedMeshLod.CreateFromModelLod(m_modelAsset, m_model, lodIndex);
                }
            }
        }

        Data::Asset<RPI::ModelAsset> SkinnedMeshInputBuffers::GetModelAsset() const
        {
            return m_modelAsset;
        }

        Data::Instance<RPI::Model> SkinnedMeshInputBuffers::GetModel() const
        {
            return m_model;
        }

        uint32_t SkinnedMeshInputBuffers::GetMeshCount(uint32_t lodIndex) const
        {
            return aznumeric_caster(m_lods[lodIndex].m_meshes.size());
        }

        uint32_t SkinnedMeshInputBuffers::GetLodCount() const
        {
            return aznumeric_caster(m_lods.size());
        }

        const SkinnedMeshInputLod& SkinnedMeshInputBuffers::GetLod(uint32_t lodIndex) const
        {
            AZ_Assert(lodIndex < m_lods.size(), "Attempting to get lod at index %" PRIu32 " in SkinnedMeshInputBuffers, which is outside the range of %zu.", lodIndex, m_lods.size());
            return m_lods[lodIndex];
        }

        uint32_t SkinnedMeshInputBuffers::GetVertexCount(uint32_t lodIndex, uint32_t meshIndex) const
        {
            return m_lods[lodIndex].m_meshes[meshIndex].m_vertexCount;
        }

        uint32_t SkinnedMeshInputBuffers::GetInfluenceCountPerVertex(uint32_t lodIndex, uint32_t meshIndex) const
        {
            return m_lods[lodIndex].m_meshes[meshIndex].m_skinInfluenceCountPerVertex;
        }

        const AZStd::vector<MorphTargetComputeMetaData>& SkinnedMeshInputBuffers::GetMorphTargetComputeMetaDatas(uint32_t lodIndex) const
        {
            return m_lods[lodIndex].m_morphTargetComputeMetaDatas;
        }

        const AZStd::vector<AZStd::intrusive_ptr<MorphTargetInputBuffers>>& SkinnedMeshInputBuffers::GetMorphTargetInputBuffers(uint32_t lodIndex) const
        {
            return m_lods[lodIndex].m_morphTargetInputBuffers;
        }

        float SkinnedMeshInputBuffers::GetMorphTargetIntegerEncoding(uint32_t lodIndex, uint32_t meshIndex) const
        {
            return m_lods[lodIndex].m_meshes[meshIndex].m_morphTargetIntegerEncoding;
        }

        void SkinnedMeshInputBuffers::AddMorphTarget(
            uint32_t lodIndex,
            const RPI::MorphTargetMetaAsset::MorphTarget& morphTarget,
            const RPI::BufferAssetView* morphBufferAssetView,
            const AZStd::string& bufferNamePrefix,
            float minWeight,
            float maxWeight)
        {
            m_lods[lodIndex].AddMorphTarget(morphTarget, morphBufferAssetView, bufferNamePrefix, minWeight, maxWeight);
        }

        void SkinnedMeshInputBuffers::Finalize()
        {
            for (SkinnedMeshInputLod& lod : m_lods)
            {
                lod.CalculateMorphTargetIntegerEncodings();
            }
        }

        void SkinnedMeshInputBuffers::SetBufferViewsOnShaderResourceGroup(
            uint32_t lodIndex, uint32_t meshIndex, const Data::Instance<RPI::ShaderResourceGroup>& perInstanceSRG)
        {
            AZ_Assert(lodIndex < m_lods.size() && meshIndex < m_lods[lodIndex].m_modelLodAsset->GetMeshes().size(), "Lod %" PRIu32 " Mesh %" PRIu32 " out of range for model '%s'", lodIndex, meshIndex, m_modelAsset->GetName().GetCStr());


            // Loop over each input buffer view and set it on the srg
            for (const SkinnedSubMeshProperties::SrgNameViewPair& nameViewPair :
                 m_lods[lodIndex].m_meshes[meshIndex].m_inputBufferViews)
            {
                RHI::ShaderInputBufferIndex srgIndex = perInstanceSRG->FindShaderInputBufferIndex(nameViewPair.m_srgName);
                AZ_Error(
                    "SkinnedMeshInputBuffers", srgIndex.IsValid(),
                    "Failed to find shader input index for '%s' in the skinning compute shader per-instance SRG.",
                    nameViewPair.m_srgName.GetCStr());

                [[maybe_unused]] bool success = perInstanceSRG->SetBufferView(srgIndex, nameViewPair.m_bufferView.get());

                AZ_Error("SkinnedMeshInputBuffers", success, "Failed to bind buffer view for %s", nameViewPair.m_srgName.GetCStr());
            }

            RHI::ShaderInputConstantIndex srgConstantIndex;
            // Set the vertex count
            srgConstantIndex = perInstanceSRG->FindShaderInputConstantIndex(Name{ "m_numVertices" });
            AZ_Error(
                "SkinnedMeshInputBuffers", srgConstantIndex.IsValid(),
                "Failed to find shader input index for m_numVerticies in the skinning compute shader per-instance SRG.");
            perInstanceSRG->SetConstant(srgConstantIndex, m_lods[lodIndex].m_meshes[meshIndex].m_vertexCount);

            // Set the max influences per vertex for the mesh
            srgConstantIndex = perInstanceSRG->FindShaderInputConstantIndex(Name{ "m_numInfluencesPerVertex" });
            AZ_Error(
                "SkinnedMeshInputBuffers", srgConstantIndex.IsValid(),
                "Failed to find shader input index for m_numInfluencesPerVertex in the skinning compute shader per-instance SRG.");
            perInstanceSRG->SetConstant(srgConstantIndex, m_lods[lodIndex].m_meshes[meshIndex].m_skinInfluenceCountPerVertex);
        }

        // Create a resource view that has a different type than the data it is viewing
        static RHI::BufferViewDescriptor CreateResourceViewWithDifferentFormat(
            uint64_t offsetInBytes,
            uint32_t realElementCount,
            uint32_t realElementSize,
            RHI::Format format,
            RHI::BufferBindFlags overrideBindFlags)
        {
            RHI::BufferViewDescriptor viewDescriptor;
            uint64_t elementOffset = offsetInBytes / RHI::GetFormatSize(format);
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

        static bool AllocateMorphTargetsForLod(const SkinnedMeshInputLod& lod, AZStd::intrusive_ptr<SkinnedMeshInstance> instance, AZStd::vector<AZStd::intrusive_ptr<SkinnedMeshOutputStreamAllocation>>& lodAllocations)
        {
            AZStd::vector<MorphTargetInstanceMetaData> instanceMetaDatas;
            for (uint32_t meshIndex = 0; meshIndex < lod.GetModelLodAsset()->GetMeshes().size(); ++meshIndex)
            {
                uint32_t vertexCount = lod.GetModelLodAsset()->GetMeshes()[meshIndex].GetVertexCount();

                // If this skinned mesh has morph targets, allocate a buffer for the accumulated deltas that come from the morph target pass
                if (lod.HasMorphTargetsForMesh(meshIndex))
                {
                    // Naively, we're going to allocate enough memory to store the accumulated delta for every vertex.
                    // This makes it simple for the skinning shader to index into the buffer, but the memory cost
                    // could be reduced by keeping a buffer that maps from vertexId to morph target delta offset ATOM-14427

                    // We're also using the skinned mesh output buffer, since it gives us a read-write pool of memory that can be
                    // used for dependency tracking between passes. This can be switched to a transient memory pool so that the memory is free
                    // later in the frame once skinning is finished ATOM-14429

                    size_t perVertexSizeInBytes = static_cast<size_t>(MorphTargetConstants::s_unpackedMorphTargetDeltaSizeInBytes) * MorphTargetConstants::s_morphTargetDeltaTypeCount;

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

                        // Track both the allocation and the metadata in the instance
                        instanceMetaDatas.push_back(instanceMetaData);
                        lodAllocations.push_back(allocation);
                    }
                }
                else
                {
                    // Use invalid offsets to indicate there are no morph targets for this mesh
                    // This allows the SkinnedMeshDispatchItem to know it doesn't need to consume morph target deltas during skinning.
                    MorphTargetInstanceMetaData instanceMetaData{ MorphTargetConstants::s_invalidDeltaOffset, MorphTargetConstants::s_invalidDeltaOffset, MorphTargetConstants::s_invalidDeltaOffset, MorphTargetConstants::s_invalidDeltaOffset };
                    instanceMetaDatas.push_back(instanceMetaData);
                }
            }

            instance->m_morphTargetInstanceMetaData.push_back(instanceMetaDatas);

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
            RHI::BufferViewDescriptor viewDescriptor = CreateResourceViewWithDifferentFormat(
                aznumeric_cast<uint64_t>(streamOffsetsFromBufferStart[outputStreamIndex]) + aznumeric_cast<uint64_t>(subMeshOffsetsFromStreamStart[outputStreamIndex]),
                submeshVertexCount, outputStreamInfo.m_elementSize, outputStreamInfo.m_elementFormat, RHI::BufferBindFlags::InputAssembly);

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
                    if (!AllocateLodStream(outputStreamIndex, lod.m_outputVertexCountsByStream[outputStreamIndex], instance, streamOffsetsFromBufferStart, lodAllocations))
                    {
                        return nullptr;
                    }
                }

                if (!AllocateMorphTargetsForLod(lod, instance, lodAllocations))
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

                AZStd::vector<bool> isSkinningEnabledPerMesh;
                isSkinningEnabledPerMesh.reserve(lod.m_meshes.size());

                SkinnedMeshOutputVertexOffsets currentMeshOffsetsFromStreamStartInBytes = {0};
                // Iterate over each sub-mesh for the lod to create views into the buffers
                for (size_t i = 0; i < lod.m_meshes.size(); ++i)
                {
                    modelLodCreator.BeginMesh();

                    // Set the index buffer view
                    const auto inputMeshes = lod.m_modelLodAsset->GetMeshes();
                    const RPI::ModelLodAsset::Mesh& inputMesh = inputMeshes[i];
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
                        lod.GetVertexCount() *
                            SkinnedMeshVertexStreamPropertyInterface::Get()
                                ->GetOutputStreamInfo(SkinnedMeshOutputVertexStreams::Position)
                                .m_elementSize;
                    meshPositionHistoryBufferOffsetsInBytes.push_back(meshPositionHistoryBufferOffsetInBytes);

                    // Create and set the views into the skinning output buffers
                    for (uint8_t outputStreamIndex = 0; outputStreamIndex < static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams); ++outputStreamIndex)
                    {
                        // Add a buffer view to the output model so it knows where to read the final skinned vertex data from
                        AddSubMeshViewToModelLodCreator(
                            outputStreamIndex, lod.m_outputVertexCountsByStream[static_cast<uint8_t>(outputStreamIndex)],
                            lod.m_meshes[i].m_vertexCount, skinnedMeshOutputBufferAsset, streamOffsetsFromBufferStart,
                            currentMeshOffsetsFromStreamStartInBytes, modelLodCreator);
                    }

                    // Set the views into the static buffers
                    for (const RPI::ModelLodAsset::Mesh::StreamBufferInfo& staticBufferInfo : lod.m_meshes[i].m_staticBufferInfo)
                    {
                        modelLodCreator.AddMeshStreamBuffer(staticBufferInfo.m_semantic, staticBufferInfo.m_customName, staticBufferInfo.m_bufferAssetView);
                    }

                    // Skip the skinning dispatch if there are no skin influences.
                    isSkinningEnabledPerMesh.push_back(lod.m_meshes[i].m_skinInfluenceCountPerVertex > 0);

                    Aabb localAabb = inputMesh.GetAabb();
                    modelLodCreator.SetMeshAabb(localAabb);

                    modelCreator.AddMaterialSlot(m_modelAsset->FindMaterialSlot(inputMesh.GetMaterialSlotId()));
                    modelLodCreator.SetMeshMaterialSlot(inputMesh.GetMaterialSlotId());

                    modelLodCreator.EndMesh();
                }

                // Add all the mesh offsets for the lod
                instance->m_outputStreamOffsetsInBytes.push_back(meshOffsetsFromBufferStartInBytes);
                instance->m_positionHistoryBufferOffsetsInBytes.push_back(meshPositionHistoryBufferOffsetsInBytes);
                instance->m_isSkinningEnabled.push_back(isSkinningEnabledPerMesh);

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
    } // namespace Render
}// namespace AZ
