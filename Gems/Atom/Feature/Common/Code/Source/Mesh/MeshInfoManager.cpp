/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/Feature/Mesh/MeshInfoBus.h>
#include <Atom/RHI.Reflect/Format.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <Mesh/MeshInfoManager.h>

namespace AZ::Render
{
    namespace GPU
    {

        // make sure we use the same MeshInfo - struct as the shaders
        using uint = uint32_t;
        using uint2 = AZStd::array<uint, 2>;
#include "Atom/Features/MeshInfo/MeshInfo.azsli"
        static_assert(sizeof(MeshInfo) % 16 == 0, "GPU struct MeshInfo does not align to 16 bytes");
    } // namespace GPU

} // namespace AZ::Render

namespace AZ::Render
{

    AZ_CVAR(
        bool,
        r_meshInfoEnabled,
        true,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "Enable creation of meshInfo entries for each mesh. Required for raytracing and deferred rendering.");

    // write the (multidevice) ReadIndices into a (single-device) meshInfo - entry
    GPU::MeshInfo ConvertToGpuMeshInfo(const MeshInfoEntry* entry, const int deviceIndex)
    {
        GPU::MeshInfo out{};

        auto FindSemanticInputBuffer = [&](const char* name, const size_t bufferIndex = 0) -> const BufferViewIndexAndOffset*
        {
            const auto semantic = AZ::RHI::ShaderSemantic{ AZ::Name{ name }, bufferIndex };
            auto iterator = entry->m_meshBuffers.find(semantic);
            if (iterator != entry->m_meshBuffers.end())
            {
                return &(iterator->second);
            }
            return nullptr;
        };

        auto ConvertToSingleDevice = [&](const BufferViewIndexAndOffset* bufferEntry,
                                         int32_t& bufferViewReadIndex,
                                         uint32_t& bufferViewByteOffset,
                                         uint32_t& bufferViewByteStride,
                                         uint32_t& format)
        {
            if (bufferEntry)
            {
                bufferViewReadIndex = static_cast<int32_t>(bufferEntry->m_bindlessReadIndex.at(deviceIndex));
                bufferViewByteOffset = bufferEntry->m_byteOffset;
                bufferViewByteStride =
                    AZStd::max(bufferEntry->m_streamBufferView.GetByteStride(), RHI::GetVertexFormatSize(bufferEntry->m_vertexFormat));
                format = AZStd::to_underlying(bufferEntry->m_vertexFormat);
            }
            else
            {
                bufferViewReadIndex = -1;
                bufferViewByteOffset = 0;
                bufferViewByteStride = 0;
                format = AZStd::to_underlying(RHI::VertexFormat::Unknown);
            }
        };

        auto RegisterSemanticMeshBuffer = [&](const char* name,
                                              int32_t& bufferViewReadIndex,
                                              uint32_t& bufferViewByteOffset,
                                              uint32_t& bufferViewStride,
                                              uint32_t& format)
        {
            const auto bufferEntry = FindSemanticInputBuffer(name);
            ConvertToSingleDevice(bufferEntry, bufferViewReadIndex, bufferViewByteOffset, bufferViewStride, format);
        };
        auto RegisterIndexedSemanticMeshBuffer = [&](const char* name,
                                                     const size_t semanticIndex,
                                                     int32_t& bufferViewReadIndex,
                                                     uint32_t& bufferViewByteOffset,
                                                     uint32_t& bufferViewByteStride,
                                                     uint32_t& format)
        {
            const auto bufferViewEntry = FindSemanticInputBuffer(name, semanticIndex);
            ConvertToSingleDevice(bufferViewEntry, bufferViewReadIndex, bufferViewByteOffset, bufferViewByteStride, format);
        };

        // clang-format off
        RegisterSemanticMeshBuffer("POSITION", out.m_positionBufferIndex, out.m_positionBufferByteOffset, out.m_positionBufferByteStride, out.m_positionFormat);
        RegisterSemanticMeshBuffer("NORMAL", out.m_normalBufferIndex, out.m_normalBufferByteOffset, out.m_normalBufferByteStride, out.m_normalFormat);
        RegisterSemanticMeshBuffer("TANGENT", out.m_tangentBufferIndex, out.m_tangentBufferByteOffset, out.m_tangentBufferByteStride, out.m_tangentFormat);
        RegisterSemanticMeshBuffer("BITANGENT", out.m_bitangentBufferIndex, out.m_bitangentBufferByteOffset, out.m_bitangentBufferByteStride, out.m_bitangentFormat);
        RegisterIndexedSemanticMeshBuffer("UV", 0, out.m_uv0BufferIndex, out.m_uv0BufferByteOffset, out.m_uv0BufferByteStride, out.m_uv0Format);
        RegisterIndexedSemanticMeshBuffer("UV", 1, out.m_uv1BufferIndex, out.m_uv1BufferByteOffset, out.m_uv1BufferByteStride, out.m_uv1Format);
        // clang-format on

        // TODO: add colorbuffer and blendmask here

        if (entry->m_indexBuffer.m_bufferView != nullptr)
        {
            out.m_indexBufferIndex = entry->m_indexBuffer.m_bindlessReadIndex.at(deviceIndex);
            out.m_indexBufferByteOffset = entry->m_indexBuffer.m_byteOffset;
            out.m_indexBufferByteStride = RHI::GetIndexFormatSize(entry->m_indexBuffer.m_indexFormat);
            out.m_indexFormat = AZStd::to_underlying(entry->m_indexBuffer.m_indexFormat);
        }
        else
        {
            out.m_indexBufferIndex = -1;
            out.m_indexBufferByteOffset = 0;
            out.m_indexBufferByteStride = 0;
            out.m_indexFormat = AZStd::to_underlying(RHI::IndexFormat::Unknown);
        }

        if (entry->m_clusterOffsetBuffer)
        {
            out.m_clusterBufferIndex = static_cast<int32_t>(entry->m_clusterOffsetBuffer->GetBindlessReadIndex().at(deviceIndex));
        }
        else
        {
            out.m_clusterBufferIndex = -1;
        }

        out.m_objectIdForTransform = entry->m_objectIdForTransform;
        out.m_lightingChannels = entry->m_lightingChannels;

        out.m_materialTypeId = entry->m_materialTypeId;
        out.m_materialInstanceId = entry->m_materialInstanceId;
        out.m_uvStreamTangentBitmask = entry->m_streamTangentBitmask.GetFullTangentBitmask();

        out.m_flags = 0;
        if (entry->m_isSkinnedMesh)
        {
            out.m_flags |= GPU::MeshInfoFlags::SkinnedMesh;
        }
        if (entry->m_hasClusterGeometryBuffers)
        {
            out.m_flags |= GPU::MeshInfoFlags::ClusterGeometryBuffers;
        }

        return out;
    }

    MeshInfoManager::MeshInfoManager()
        : m_meshInfoBuffer{ "MeshInfo", RPI::CommonBufferPoolType::ReadOnly, static_cast<uint32_t>(sizeof(GPU::MeshInfo)) }
    {
    }

    void MeshInfoManager::Activate(RPI::Scene* scene)
    {
        m_meshInfoNeedsUpdate = true;
        UpdateMeshInfoBuffer();

        if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            console->GetCvarValue("r_meshInfoEnabled", m_isEnabled);
        }

        m_sceneId = scene->GetId();

        // We need to register the buffer in the SceneSrg even if we are disabled
        m_updateSceneSrgHandler = RPI::Scene::PrepareSceneSrgEvent::Handler(
            [this](RPI::ShaderResourceGroup* sceneSrg)
            {
                sceneSrg->SetBufferView(m_meshInfoIndex, GetMeshInfoBuffer()->GetBufferView());
            });
        scene->ConnectEvent(m_updateSceneSrgHandler);
    }
    void MeshInfoManager::Deactivate()
    {
        m_updateSceneSrgHandler.Disconnect();
    }

    auto MeshInfoManager::AcquireMeshInfoEntry() -> MeshInfoHandle
    {
        MeshInfoHandle handle{ MeshInfoHandle::NullIndex };
        if (!m_isEnabled)
        {
            return handle;
        }
        // scoped lock, so we can be sure the meshInfoIndex is unique
        {
            AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
            constexpr static size_t MeshInfoMinEntries = 32;
            auto meshInfoindex = m_meshInfoIndices.Aquire();
            // Allocate several entries so we can avoid reallocation both on the CPU and GPU
            const auto numEntries = AlignUpToPowerOfTwo(AZStd::max(MeshInfoMinEntries, static_cast<size_t>(m_meshInfoIndices.MaxCount())));

            if (m_meshInfoData.size() < m_meshInfoIndices.MaxCount())
            {
                m_meshInfoData.resize(numEntries, nullptr);
            }
            handle = MeshInfoHandle{ meshInfoindex };
            m_meshInfoData[meshInfoindex] = aznew MeshInfoEntry;
        }

        m_meshInfoNeedsUpdate = true;

        // Notify other components that we allocated a new entry MeshInfoIndex
        MeshInfoNotificationBus::Event(m_sceneId, &MeshInfoNotificationBus::Events::OnAcquireMeshInfoEntry, handle);
        return handle;
    }

    void MeshInfoManager::ReleaseMeshInfoEntry(const MeshInfoHandle handle)
    {
        if (!m_isEnabled)
        {
            return;
        }

        // Notify other components that we are about to delete an entry for the MeshInfoIndex
        MeshInfoNotificationBus::Event(m_sceneId, &MeshInfoNotificationBus::Events::OnReleaseMeshInfoEntry, handle);
        {
            AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
            m_meshInfoIndices.Release(handle.GetIndex());
            AZ_Assert(m_meshInfoData.size() > handle.GetIndex(), "ReleaseMeshInfoEntry() called with invalid index");
            if (m_meshInfoData.size() > handle.GetIndex())
            {
                // mark the entry as invalid
                m_meshInfoData[handle.GetIndex()] = nullptr;
            }
        }
        m_meshInfoNeedsUpdate = true;
    }

    void MeshInfoManager::UpdateMeshInfoEntry(const MeshInfoHandle handle, AZStd::function<bool(MeshInfoEntry*)> updateFunction)
    {
        if (!m_isEnabled)
        {
            return;
        }

        RHI::Ptr<MeshInfoEntry> entry;
        {
            AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
            if (m_meshInfoData.size() > handle.GetIndex() && m_meshInfoData[handle.GetIndex()] != nullptr)
            {
                // make a copy of the smart pointer to make sure the entry isn't deleted during the update function
                entry = m_meshInfoData[handle.GetIndex()];
            }
            else
            {
                AZ_Assert(false, "UpdateMeshInfoEntry() called with invalid index");
            }
        }
        if (entry)
        {
            m_meshInfoNeedsUpdate |= updateFunction(entry.get());
        }
    }

    void MeshInfoManager::UpdateMeshInfoBuffer()
    {
        if (m_meshInfoNeedsUpdate)
        {
            AZStd::unordered_map<int, AZStd::vector<GPU::MeshInfo>> multiDeviceMeshInfo;
            AZStd::unordered_map<int, const void*> updateDataHelper;

            const GPU::MeshInfo invalidMeshInfo{ -1 };

            const auto deviceCount{ AZ::RHI::RHISystemInterface::Get()->GetDeviceCount() };
            // Initialize at least one entry for each device so we don't have dangling buffers
            const auto numEntries = AZStd::max(m_meshInfoData.size(), size_t{ 1 });

            for (auto deviceIndex{ 0 }; deviceIndex < deviceCount; ++deviceIndex)
            {
                // reserve the memory that will be uploaded to the GPU
                multiDeviceMeshInfo[deviceIndex].resize(numEntries, invalidMeshInfo);
                updateDataHelper[deviceIndex] = multiDeviceMeshInfo[deviceIndex].data();
            }

            for (auto deviceIndex{ 0 }; deviceIndex < deviceCount; ++deviceIndex)
            {
                for (int meshInfoIndex = 0; meshInfoIndex < numEntries; meshInfoIndex++)
                {
                    if (m_meshInfoData.empty() || m_meshInfoData[meshInfoIndex] == nullptr)
                    {
                        continue;
                    }
                    auto& entry = m_meshInfoData[meshInfoIndex];
                    multiDeviceMeshInfo[deviceIndex][meshInfoIndex] = ConvertToGpuMeshInfo(entry.get(), deviceIndex);
                }
            }

            m_meshInfoBuffer.AdvanceCurrentBufferAndUpdateData(updateDataHelper, numEntries * sizeof(GPU::MeshInfo));
            m_meshInfoNeedsUpdate = false;
        }
    }

    const RHI::Ptr<MeshInfoEntry> MeshInfoManager::GetMeshInfoEntry(const MeshInfoHandle handle)
    {
        if (!m_isEnabled)
        {
            return { nullptr };
        }

        RHI::Ptr<MeshInfoEntry> entry{ nullptr };
        {
            // we need to lock here, because someone could modify the array between checking and fetching the entry
            AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
            if (m_meshInfoData.size() > handle.GetIndex())
            {
                entry = m_meshInfoData[handle.GetIndex()];
            }
        }
        return entry;
    }

    const Data::Instance<RPI::Buffer>& MeshInfoManager::GetMeshInfoBuffer() const
    {
        return m_meshInfoBuffer.GetCurrentBuffer();
    }

    void CreateShaderInputContract(RPI::ShaderInputContract& contract, AZStd::vector<RHI::Format>& channelFormat)
    {
        auto AddIndexedStreamChannel =
            [&](const char* name, const uint32_t bufferIndex, const RHI::Format format, const bool isOptional = false)
        {
            RPI::ShaderInputContract::StreamChannelInfo streamChannelInfo;
            streamChannelInfo.m_semantic = RHI::ShaderSemantic{ AZ::Name(name), bufferIndex };
            streamChannelInfo.m_componentCount = RHI::GetFormatComponentCount(format);
            streamChannelInfo.m_isOptional = isOptional;
            contract.m_streamChannels.emplace_back(AZStd::move(streamChannelInfo));
            channelFormat.emplace_back(format);
        };

        auto AddStreamChannel = [&](const char* name, const RHI::Format format, const bool isOptional = false)
        {
            return AddIndexedStreamChannel(name, 0, format, isOptional);
        };

        // TODO: The vertex-formats can be way more flexible
        static const RHI::Format PositionStreamFormat = RHI::Format::R32G32B32_FLOAT;
        static const RHI::Format NormalStreamFormat = RHI::Format::R32G32B32_FLOAT;
        static const RHI::Format TangentStreamFormat = RHI::Format::R32G32B32A32_FLOAT;
        static const RHI::Format BitangentStreamFormat = RHI::Format::R32G32B32_FLOAT;
        static const RHI::Format UVStreamFormat = RHI::Format::R32G32_FLOAT;

        AddStreamChannel("POSITION", PositionStreamFormat);
        AddStreamChannel("NORMAL", NormalStreamFormat);
        AddStreamChannel("TANGENT", TangentStreamFormat, true);
        AddStreamChannel("BITANGENT", BitangentStreamFormat, true);
        AddIndexedStreamChannel("UV", 0, UVStreamFormat, true);
        AddIndexedStreamChannel("UV", 1, UVStreamFormat, true);

        // TODO: Figure out how to add Vertex-color and BlendMask as optional inputs
    }

    // utility function to initialize the buffer indices from default models
    void MeshInfoManager::InitMeshInfoGeometryBuffers(
        const RPI::Model* model,
        const size_t lod,
        const size_t meshIndex,
        const RPI::Material* material,
        const RPI::MaterialModelUvOverrideMap& uvMapping,
        MeshInfoEntry* entry)
    {
        auto& modelLod = model->GetLods()[lod];
        auto& mesh = modelLod->GetMeshes()[meshIndex];

        RPI::ShaderInputContract inputContract;
        AZStd::vector<RHI::Format> inputChannelFormat;

        // TODO: Find out if shaders need optional inputs
        // material->ForAllShaderItems(
        //     [&](const Name& name, const RPI::ShaderCollection::Item& item)
        //     {
        //         if (shaderItem.IsEnabled() && shaderItem.GetShaderTag() == AZ::Name("deferred"))
        //         {
        //             entry->m_optionalInputStreamShaderOptions = *shaderItem.GetShaderOptions();
        //             return false; // end iteraton
        //         }
        //         return true;
        //     });

        CreateShaderInputContract(inputContract, inputChannelFormat);

        // retrieve vertex/index buffers
        RHI::InputStreamLayout inputStreamLayout;
        RHI::StreamBufferIndices streamIndices;

        [[maybe_unused]] bool result = modelLod->GetStreamsForMesh(
            inputStreamLayout,
            streamIndices,
            &entry->m_streamTangentBitmask,
            inputContract,
            meshIndex,
            uvMapping,
            material->GetAsset()->GetMaterialTypeAsset()->GetUvNameMap());
        AZ_Assert(result, "Failed to retrieve mesh stream buffer views");

        // TODO: deal with shader options that mark optional input streams
        // modelLod->CheckOptionalStreams(
        //     entry->m_optionalInputStreamShaderOptions,
        //     inputContract,
        //     meshIndex,
        //     uvMapping,
        //     material->GetAsset()->GetMaterialTypeAsset()->GetUvNameMap());

        // build raw buffer views and get the bindless read indices of all found input buffers
        auto streamIter = mesh.CreateStreamIterator(streamIndices);
        for (int streamIndex = 0; streamIndex < inputContract.m_streamChannels.size(); ++streamIndex)
        {
            // For each semantic buffer view entry we store:
            // - a reference to the original StreamBufferView
            // - A new, 'raw' BufferView for the entire geometry-buffer (Note: we could deduplicate this)
            // - The bindless read-index of this new BufferView
            // - The start-offset inside the new BufferView
            const auto semantic = inputContract.m_streamChannels[streamIndex].m_semantic;
            auto& streamBuffer = streamIter[static_cast<u16>(streamIndex)];
            entry->m_meshBuffers[semantic] =
                BufferViewIndexAndOffset::Create(streamBuffer, RHI::ConvertToVertexFormat(inputChannelFormat[streamIndex]));
        }

        // Register the Index buffer
        entry->m_indexBuffer = IndexBufferViewIndexAndOffset::Create(mesh.GetIndexBufferView());
    }
} // namespace AZ::Render
