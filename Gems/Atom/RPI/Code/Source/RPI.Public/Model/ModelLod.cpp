/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Model/ModelLod.h>
#include <Atom/RPI.Public/Material/Material.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>

#include <AzCore/Debug/EventTrace.h>
#include <AtomCore/Instance/InstanceDatabase.h>

namespace AZ
{
    namespace RPI
    {
        Data::Instance<ModelLod> ModelLod::FindOrCreate(const Data::Asset<ModelLodAsset>& lodAsset, const Data::Asset<ModelAsset>& modelAsset)
        {
            AZStd::any modelAssetAny{&modelAsset};

            return Data::InstanceDatabase<ModelLod>::Instance().FindOrCreate(
                Data::InstanceId::CreateFromAssetId(lodAsset.GetId()),
                lodAsset,
                &modelAssetAny);
        }

        AZStd::array_view<ModelLod::Mesh> ModelLod::GetMeshes() const
        {
            return m_meshes;
        }

        Data::Instance<ModelLod> ModelLod::CreateInternal(const Data::Asset<ModelLodAsset>& lodAsset, const AZStd::any* modelAssetAny)
        {
            AZ_Assert(modelAssetAny != nullptr, "Invalid model asset param");
            auto modelAsset = AZStd::any_cast<Data::Asset<ModelAsset>*>(*modelAssetAny);

            Data::Instance<ModelLod> lod = aznew ModelLod();
            const RHI::ResultCode resultCode = lod->Init(lodAsset, *modelAsset);

            if (resultCode == RHI::ResultCode::Success)
            {
                return lod;
            }

            return nullptr;
        }

        RHI::ResultCode ModelLod::Init(const Data::Asset<ModelLodAsset>& lodAsset, const Data::Asset<ModelAsset>& modelAsset)
        {
            AZ_TRACE_METHOD();

            for (const ModelLodAsset::Mesh& mesh : lodAsset->GetMeshes())
            {
                Mesh meshInstance;

                const BufferAssetView& indexBufferAssetView = mesh.GetIndexBufferAssetView();
                const Data::Asset<BufferAsset>& indexBufferAsset = indexBufferAssetView.GetBufferAsset();
                if (indexBufferAsset)
                {
                    Data::Instance<Buffer> indexBuffer = Buffer::FindOrCreate(indexBufferAsset);

                    if (!indexBuffer)
                    {
                        return RHI::ResultCode::Fail;
                    }

                    const RHI::BufferViewDescriptor& bufferViewDescriptor = indexBufferAssetView.GetBufferViewDescriptor();

                    RHI::IndexFormat indexFormat = RHI::IndexFormat::Uint32;

                    if (bufferViewDescriptor.m_elementSize == sizeof(uint16_t))
                    {
                        indexFormat = RHI::IndexFormat::Uint16;
                    }
                    else if (bufferViewDescriptor.m_elementSize != sizeof(uint32_t))
                    {
                        AZ_Error("ModelLod", false, "Index buffer format is invalid. Only 16 or 32 bit indices are supported.");
                        return RHI::ResultCode::InvalidOperation;
                    }

                    meshInstance.m_indexBufferView = RHI::IndexBufferView(
                        *indexBuffer->GetRHIBuffer(),
                        bufferViewDescriptor.m_elementOffset * bufferViewDescriptor.m_elementSize,
                        bufferViewDescriptor.m_elementCount * bufferViewDescriptor.m_elementSize,
                        indexFormat);

                    RHI::DrawIndexed drawIndexed;
                    drawIndexed.m_indexCount = bufferViewDescriptor.m_elementCount;
                    drawIndexed.m_instanceCount = 1;
                    meshInstance.m_drawArguments = drawIndexed;

                    TrackBuffer(indexBuffer);
                }

                // [GFX TODO][ATOM-838]: We need to figure out how to load only the required streams from disk rather than all available streams.
                for (const auto& streamBufferInfo : mesh.GetStreamBufferInfoList())
                {
                    if (!SetMeshInstanceData(streamBufferInfo, meshInstance))
                    {
                        return RHI::ResultCode::InvalidOperation;
                    }
                }

                const ModelMaterialSlot& materialSlot = modelAsset->FindMaterialSlot(mesh.GetMaterialSlotId());

                meshInstance.m_materialSlotStableId = materialSlot.m_stableId;

                if (materialSlot.m_defaultMaterialAsset.IsReady())
                {
                    meshInstance.m_material = Material::FindOrCreate(materialSlot.m_defaultMaterialAsset);
                }

                m_meshes.emplace_back(AZStd::move(meshInstance));
            }

            m_isUploadPending = true;
            return RHI::ResultCode::Success;
        }

        ModelLod::StreamInfoList::const_iterator ModelLod::FindFirstUvStreamFromMesh(size_t meshIndex) const
        {
            const Mesh& mesh = m_meshes[meshIndex];

            auto firstUv = AZStd::find_if(mesh.m_streamInfo.begin(), mesh.m_streamInfo.end(), [](const StreamBufferInfo& info) {
                return info.m_semantic.m_name.GetStringView().starts_with(RHI::ShaderSemantic::UvStreamSemantic);
            });

            return firstUv;
        }

        ModelLod::StreamInfoList::const_iterator ModelLod::FindDefaultUvStream(size_t meshIndex, const MaterialUvNameMap& materialUvNameMap) const
        {
            const Mesh& mesh = m_meshes[meshIndex];

            // The default UV is used for cases that there are more UVs defined in the material than in the model.
            // The unmatched UV slots will be filled with the default UV.
            // The default UV is the first one matched in the shader input contract.
            auto defaultUv = mesh.m_streamInfo.end();
            for (const auto& materialUvNamePair : materialUvNameMap)
            {
                const AZ::Name& uvCustomName = materialUvNamePair.m_uvName;
                const RHI::ShaderSemantic& shaderInput = materialUvNamePair.m_shaderInput;
                // Use name matching first. Empty name can't be used because it will match other non-UV streams.
                if (!uvCustomName.IsEmpty())
                {
                    defaultUv = AZStd::find_if(mesh.m_streamInfo.begin(), mesh.m_streamInfo.end(), [&uvCustomName](const StreamBufferInfo& info)
                    {
                        return info.m_customName == uvCustomName;
                    });
                }
                // Use semantic matching second if previous matching failed.
                if (defaultUv == mesh.m_streamInfo.end())
                {
                    defaultUv = AZStd::find_if(mesh.m_streamInfo.begin(), mesh.m_streamInfo.end(), [&shaderInput](const StreamBufferInfo& info)
                    {
                        return info.m_semantic == shaderInput;
                    });
                }
                // Select the first matching
                if (defaultUv != mesh.m_streamInfo.end())
                {
                    break;
                }
            }

            return defaultUv;
        }

        ModelLod::StreamInfoList::const_iterator ModelLod::FindMatchingStream(
            size_t meshIndex,
            const MaterialModelUvOverrideMap& materialModelUvMap,
            const MaterialUvNameMap& materialUvNameMap,
            const ShaderInputContract::StreamChannelInfo& contractStreamChannel,
            StreamInfoList::const_iterator defaultUv,
            StreamInfoList::const_iterator firstUv,
            UvStreamTangentBitmask* uvStreamTangentBitmaskOut) const
        {
            const Mesh& mesh = m_meshes[meshIndex];
            auto iter = mesh.m_streamInfo.end();

            // Special matching for UV sets, we will match each UV shader input by following steps:
            // 1. The custom mapping from the name in material to the name in model (modelUvMap)
            // 2. The exact name matching between material and model (uvCustomNames <=> mesh.m_streamInfo.m_customName)
            // 3. The exact semantic matching between material and model (uvDefaultNames <=> mesh.m_streamInfo.m_semantic)
            // 4. If no matching found from the model, then the first applied model UV fills the slot.
            // e.g. (In practice, custom mapping should have the same size as material's UV, or empty if in places like material editor)
            // Material               Model                model UV map           Final Mapping
            // UV0: Unwrapped         UV0: Packed          Unwrapped = Packed     UV0: Unwrapped = UV0: Packed (rule 1: custom mapping)
            // UV1: Packed            UV1: Unwrapped                              UV1: Packed    = UV0: Packed (rule 2: default name mapping)
            // UV2: Tiled             UV2: Repeated                               UV2: Tiled     = UV2: Repeated (rule 3: semantic name mapping)
            // UV3: Extra                                                         UV3: Extra     = UV0: Packed (rule 4: first filling)

            // ensure the semantic is a UV, otherwise skip name matching
            auto materialUvIter = AZStd::find_if(materialUvNameMap.begin(), materialUvNameMap.end(),
                [&contractStreamChannel](const UvNamePair& uvNamePair)
                {
                    // Cost of linear search UV names is low because the size is extremely limited.
                    return uvNamePair.m_shaderInput == contractStreamChannel.m_semantic;
                });
            const bool isUv = materialUvIter != materialUvNameMap.end();
            if (isUv)
            {
                const AZ::Name& materialUvName = materialUvIter->m_uvName;
                auto modelUvMapIter = materialModelUvMap.find(materialUvIter->m_shaderInput);
                if (modelUvMapIter != materialModelUvMap.end())
                {
                    const AZ::Name& modelUvName = modelUvMapIter->second;
                    // Empty name can't be used because it will match other non-UV streams.
                    if (!modelUvName.IsEmpty())
                    {
                        iter = AZStd::find_if(mesh.m_streamInfo.begin(), mesh.m_streamInfo.end(), [&modelUvName](const StreamBufferInfo& info)
                            {
                                return info.m_customName == modelUvName
                                    || info.m_semantic.ToString() == modelUvName.GetStringView(); // For unnamed UVs, use the semantic instead.
                            });
                    }
                }

                if (iter == mesh.m_streamInfo.end())
                {
                    // Empty name can't be used because it will match other non-UV streams.
                    if (!materialUvName.IsEmpty())
                    {
                        iter = AZStd::find_if(mesh.m_streamInfo.begin(), mesh.m_streamInfo.end(), [&materialUvName](const StreamBufferInfo& info)
                            {
                                return info.m_customName == materialUvName;
                            });
                    }
                }
            }

            if (iter == mesh.m_streamInfo.end())
            {
                iter = AZStd::find_if(mesh.m_streamInfo.begin(), mesh.m_streamInfo.end(), [&contractStreamChannel](const StreamBufferInfo& info)
                    {
                        return info.m_semantic == contractStreamChannel.m_semantic;
                    });
            }

            if (iter == mesh.m_streamInfo.end() && isUv)
            {
                iter = defaultUv;
            }

            if (isUv && uvStreamTangentBitmaskOut)
            {
                uvStreamTangentBitmaskOut->ApplyTangent(iter == firstUv ? 0 : UvStreamTangentBitmask::UnassignedTangent);
            }

            return iter;
        }

        bool ModelLod::GetStreamsForMesh(
            RHI::InputStreamLayout& layoutOut,
            StreamBufferViewList& streamBufferViewsOut,
            UvStreamTangentBitmask* uvStreamTangentBitmaskOut,
            const ShaderInputContract& contract,
            size_t meshIndex,
            const MaterialModelUvOverrideMap& materialModelUvMap,
            const MaterialUvNameMap& materialUvNameMap) const
        {
            streamBufferViewsOut.clear();

            RHI::InputStreamLayoutBuilder layoutBuilder;

            const Mesh& mesh = m_meshes[meshIndex];

            bool success = true;

            // Searching for the first UV in the mesh, so it can be used to paired with tangent/bitangent stream
            auto firstUv = FindFirstUvStreamFromMesh(meshIndex);
            auto defaultUv = FindDefaultUvStream(meshIndex, materialUvNameMap);
            if (uvStreamTangentBitmaskOut)
            {
                uvStreamTangentBitmaskOut->Reset();
            }

            for (auto& contractStreamChannel : contract.m_streamChannels)
            {
                auto iter = FindMatchingStream(meshIndex, materialModelUvMap, materialUvNameMap, contractStreamChannel, defaultUv, firstUv, uvStreamTangentBitmaskOut);

                if (iter == mesh.m_streamInfo.end())
                {
                    if (contractStreamChannel.m_isOptional)
                    {
                        //We are using R8G8B8A8_UINT as on Metal mesh stream formats need to be atleast 4 byte aligned.
                        RHI::Format dummyStreamFormat = RHI::Format::R8G8B8A8_UINT;
                        layoutBuilder.AddBuffer()->Channel(contractStreamChannel.m_semantic, dummyStreamFormat);
                        // We can't just use a null buffer pointer here because vulkan will occasionally crash. So we bind some valid non-null buffer and view it with length 0.
                        RHI::StreamBufferView dummyBuffer{*mesh.m_indexBufferView.GetBuffer(), 0, 0, 4};
                        streamBufferViewsOut.push_back(dummyBuffer);

                        // Note that all of the below scenarios seem to work find on PC, for both dx12 and vulkan. If the above approach proves to be incompatible
                        // with another platform, consider trying one of the approaches below.

                        //RHI::Format formatDoesntReallyMatter = RHI::Format::R8_UNORM;
                        //layoutBuilder.AddBuffer(RHI::StreamStepFunction::PerInstance)->Channel(contractStreamChannel.m_semantic, formatDoesntReallyMatter);
                        //RHI::StreamBufferView dummyBuffer{*mesh.m_indexBufferView.GetBuffer(), 0, 0, 0};
                        //streamBufferViewsOut.push_back(dummyBuffer);

                        //RHI::Format formatDoesntReallyMatter = RHI::Format::R8G8B8A8_UINT;
                        //layoutBuilder.AddBuffer(RHI::StreamStepFunction::PerInstance)->Channel(contractStreamChannel.m_semantic, formatDoesntReallyMatter);
                        //RHI::StreamBufferView dummyBuffer{*mesh.m_indexBufferView.GetBuffer(), 0, 4, 4};
                        //streamBufferViewsOut.push_back(dummyBuffer);

                        //RHI::Format formatDoesntMatter = RHI::Format::R32G32B32A32_FLOAT;
                        //layoutBuilder.AddBuffer()->Channel(contractStreamChannel.m_semantic, formatDoesntMatter);
                        //RHI::StreamBufferView emptyBuffer{*m_buffers[0]->GetRHIBuffer(), 0, 16, 16};
                        //streamBufferViewsOut.push_back(emptyBuffer);

                        //RHI::Format formatDoesntMatter = RHI::Format::R32G32B32A32_FLOAT;
                        //layoutBuilder.AddBuffer()->Channel(contractStreamChannel.m_semantic, formatDoesntMatter);
                        //RHI::StreamBufferView emptyBuffer{*m_buffers[0]->GetRHIBuffer(), 0, 0, 16};
                        //streamBufferViewsOut.push_back(emptyBuffer);
                    }
                    else
                    {
                        AZ_Warning("Mesh", false, "Mesh does not have all the required input streams. Missing '%s'.", contractStreamChannel.m_semantic.ToString().c_str());
                        success = false;
                    }
                }
                else
                {
                    // Note, we may need to iterate on the details of this validation. It might not be correct for all use cases.
                    if (RHI::GetFormatComponentCount(iter->m_format) < contractStreamChannel.m_componentCount)
                    {
                        AZ_Error("Mesh", false, "Mesh format (%s) for stream '%s' provides %d components but the shader requires %d.",
                            RHI::ToString(iter->m_format),
                            contractStreamChannel.m_semantic.ToString().c_str(),
                            RHI::GetFormatComponentCount(iter->m_format),
                            contractStreamChannel.m_componentCount);
                        success = false;
                    }
                    else
                    {
                        // Note, don't use iter->m_semantic as it can be a UV name matching.
                        layoutBuilder.AddBuffer()->Channel(contractStreamChannel.m_semantic, iter->m_format);

                        RHI::StreamBufferView bufferView(*m_buffers[iter->m_bufferIndex]->GetRHIBuffer(), iter->m_byteOffset, iter->m_byteCount, iter->m_stride);
                        streamBufferViewsOut.push_back(bufferView);
                    }
                }
            }

            if (success)
            {
                layoutOut = layoutBuilder.End();

                success &= RHI::ValidateStreamBufferViews(layoutOut, streamBufferViewsOut);
            }

            return success;
        }

        void ModelLod::CheckOptionalStreams(
            ShaderOptionGroup& shaderOptions,
            const ShaderInputContract& contract,
            size_t meshIndex,
            const MaterialModelUvOverrideMap& materialModelUvMap,
            const MaterialUvNameMap& materialUvNameMap) const
        {
            const Mesh& mesh = m_meshes[meshIndex];

            auto defaultUv = FindDefaultUvStream(meshIndex, materialUvNameMap);
            auto firstUv = FindFirstUvStreamFromMesh(meshIndex);

            for (auto& contractStreamChannel : contract.m_streamChannels)
            {
                if (!contractStreamChannel.m_isOptional)
                {
                    continue;
                }

                AZ_Assert(contractStreamChannel.m_streamBoundIndicatorIndex.IsValid(), "m_streamBoundIndicatorIndex was invalid for an optional shader input stream");

                auto iter = FindMatchingStream(meshIndex, materialModelUvMap, materialUvNameMap, contractStreamChannel, defaultUv, firstUv, nullptr);

                ShaderOptionValue isStreamBound = (iter == mesh.m_streamInfo.end()) ? ShaderOptionValue{0} : ShaderOptionValue{1};
                shaderOptions.SetValue(contractStreamChannel.m_streamBoundIndicatorIndex, isStreamBound);
            }
        }

        bool ModelLod::SetMeshInstanceData(
            const ModelLodAsset::Mesh::StreamBufferInfo& streamBufferInfo,
            Mesh& meshInstance)
        {
            AZ_TRACE_METHOD();

            const Data::Asset<BufferAsset>& streamBufferAsset = streamBufferInfo.m_bufferAssetView.GetBufferAsset();
            const Data::Instance<Buffer>& streamBuffer = Buffer::FindOrCreate(streamBufferAsset);
            if (streamBuffer == nullptr)
            {
                AZ_Error("ModelLod", false, "Failed to create stream buffer! Possibly out of memory!");
                return false;
            }

            const RHI::BufferViewDescriptor& bufferViewDescriptor = streamBufferInfo.m_bufferAssetView.GetBufferViewDescriptor();

            StreamBufferInfo info;
            info.m_semantic = streamBufferInfo.m_semantic;
            info.m_customName = streamBufferInfo.m_customName;
            info.m_format = bufferViewDescriptor.m_elementFormat;
            info.m_byteOffset = bufferViewDescriptor.m_elementOffset * bufferViewDescriptor.m_elementSize;
            info.m_byteCount = bufferViewDescriptor.m_elementCount * bufferViewDescriptor.m_elementSize;
            info.m_stride = bufferViewDescriptor.m_elementSize;
            info.m_bufferIndex = TrackBuffer(streamBuffer);

            meshInstance.m_streamInfo.push_back(info);

            return true;
        }

        void ModelLod::WaitForUpload()
        {
            if (m_isUploadPending)
            {
                for (const Data::Instance<Buffer>& buffer : m_buffers)
                {
                    buffer->WaitForUpload();
                }
                m_isUploadPending = false;
            }
        }

        uint32_t ModelLod::TrackBuffer(const Data::Instance<Buffer>& buffer)
        {
            for (uint32_t i = 0; i < m_buffers.size(); ++i)
            {
                auto& existingBuffer = m_buffers[i];
                if (existingBuffer.get() == buffer)
                {
                    return i;
                }
            }

            m_buffers.emplace_back(buffer);
            return static_cast<uint32_t>(m_buffers.size() - 1);
        }
    } // namespace RPI
} // namespace AZ
