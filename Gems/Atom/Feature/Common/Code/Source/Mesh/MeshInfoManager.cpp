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
#include <Mesh/MeshInfoManager.h>

namespace AZ::Render
{
    // make sure we use the same MeshInfo - struct as the shaders
#define uint unsigned int
#define uint2 AZStd::array<uint, 2>
#include <Atom/Features/MeshInfo/MeshInfo.azsli>
#undef uint
#undef uint2
} // namespace AZ::Render

namespace AZ::Render
{

    // write the (multidevice) ReadIndices into a (single-device) meshInfo - entry
    MeshInfo ConvertToGpuMeshInfo(const MeshInfoEntry* entry, const int deviceIndex)
    {
        MeshInfo out{};

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

        auto ConvertToSingleDevice =
            [&](const BufferViewIndexAndOffset* bufferEntry, int32_t& bufferViewReadIndex, uint32_t& bufferViewByteOffset)
        {
            if (bufferEntry)
            {
                bufferViewReadIndex = static_cast<int32_t>(bufferEntry->m_bindlessReadIndex.at(deviceIndex));
                bufferViewByteOffset = bufferEntry->m_byteOffset;
            }
            else
            {
                bufferViewReadIndex = -1;
                bufferViewByteOffset = 0;
            }
        };

        auto RegisterSemanticMeshBuffer = [&](const char* name, int32_t& bufferViewReadIndex, uint32_t& bufferViewByteOffset)
        {
            const auto bufferEntry = FindSemanticInputBuffer(name);
            ConvertToSingleDevice(bufferEntry, bufferViewReadIndex, bufferViewByteOffset);
        };
        auto RegisterIndexedSemanticMeshBuffer =
            [&](const char* name, const size_t semanticIndex, int32_t& bufferViewReadIndex, uint32_t& bufferViewByteOffset)
        {
            const auto bufferViewEntry = FindSemanticInputBuffer(name, semanticIndex);
            ConvertToSingleDevice(bufferViewEntry, bufferViewReadIndex, bufferViewByteOffset);
        };

        RegisterSemanticMeshBuffer("POSITION", out.m_positionBufferIndex, out.m_positionBufferByteOffset);
        RegisterSemanticMeshBuffer("NORMAL", out.m_normalBufferIndex, out.m_normalBufferByteOffset);
        RegisterSemanticMeshBuffer("TANGENT", out.m_tangentBufferIndex, out.m_tangentBufferByteOffset);
        RegisterSemanticMeshBuffer("BITANGENT", out.m_bitangentBufferIndex, out.m_bitangentBufferByteOffset);
        RegisterIndexedSemanticMeshBuffer("UV", 0, out.m_uv0BufferIndex, out.m_uv0BufferByteOffset);
        RegisterIndexedSemanticMeshBuffer("UV", 1, out.m_uv1BufferIndex, out.m_uv1BufferByteOffset);

        // TODO: add colorbuffer and blendmask here

        out.m_indexBufferIndex = entry->m_indexBuffer.m_bindlessReadIndex.at(deviceIndex);
        out.m_indexBufferByteOffset = entry->m_indexBuffer.m_byteOffset;
        if (entry->m_indexBuffer.m_indexBufferFormat == RHI::IndexFormat::Uint16)
        {
            out.m_flags |= MeshInfoFlags::IndexBufferFlag_16Bit;
        }

        if (entry->m_isSkinnedMesh)
        {
            out.m_flags |= MeshInfoFlags::SkinnedMesh;
        }

        out.m_objectIdForTransform = entry->m_objectIdForTransform;
        out.m_lightingChannels = entry->m_lightingChannels;

        out.m_materialTypeId = entry->m_materialTypeId;
        out.m_materialInstanceId = entry->m_materialInstanceId;
        out.m_uvStreamTangentBitmask = entry->m_streamTangentBitmask.GetFullTangentBitmask();
        return out;
    }

    MeshInfoManager::MeshInfoManager()
        : m_meshInfoBuffer{ "MeshInfo", RPI::CommonBufferPoolType::ReadOnly, static_cast<uint32_t>(sizeof(MeshInfo)) }
    {
    }

    void MeshInfoManager::Activate(RPI::Scene* scene)
    {
        m_meshInfoNeedsUpdate = true;
        UpdateMeshInfoBuffer();

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
        constexpr static size_t MeshInfoMinEntries = 32;
        auto meshInfoindex = m_meshInfoIndices.Aquire();
        // Allocate several entries so we can avoid reallocation both on the CPU and GPU
        const auto numEntries = AlignUpToPowerOfTwo(AZStd::max(MeshInfoMinEntries, static_cast<size_t>(m_meshInfoIndices.MaxCount())));

        if (m_meshInfoData.size() < m_meshInfoIndices.MaxCount())
        {
            m_meshInfoData.resize(numEntries, nullptr);
        }
        m_meshInfoData[meshInfoindex] = aznew MeshInfoEntry;
        m_meshInfoNeedsUpdate = true;
        return MeshInfoHandle{ meshInfoindex };
    }

    void MeshInfoManager::ReleaseMeshInfoEntry(const MeshInfoHandle handle)
    {
        m_meshInfoIndices.Release(handle.GetIndex());
        if (m_meshInfoData.size() > handle.GetIndex())
        {
            // mark the entry as invalid
            m_meshInfoData[handle.GetIndex()] = nullptr;
        }
        m_meshInfoNeedsUpdate = true;
    }

    void MeshInfoManager::UpdateMeshInfoEntry(const MeshInfoHandle handle, AZStd::function<bool(MeshInfoEntry*)> updateFunction)
    {
        if (m_meshInfoData.size() > handle.GetIndex() && m_meshInfoData[handle.GetIndex()] != nullptr)
        {
            m_meshInfoNeedsUpdate = updateFunction(m_meshInfoData[handle.GetIndex()].get());
        }
    }

    void MeshInfoManager::UpdateMeshInfoBuffer()
    {
        if (m_meshInfoNeedsUpdate)
        {
            AZStd::unordered_map<int, AZStd::vector<MeshInfo>> multiDeviceMeshInfo;
            AZStd::unordered_map<int, const void*> updateDataHelper;

            const MeshInfo invalidMeshInfo{ -1 };

            const auto deviceCount{ AZ::RHI::RHISystemInterface::Get()->GetDeviceCount() };
            // Initialize at least one entry for each device so we don't have dangling buffers
            const auto numEntries = AZStd::max(m_meshInfoData.size(), 1ull);

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

            m_meshInfoBuffer.AdvanceCurrentBufferAndUpdateData(updateDataHelper, numEntries * sizeof(MeshInfo));
            m_meshInfoNeedsUpdate = false;
        }
    }

    const RHI::Ptr<MeshInfoEntry>& MeshInfoManager::GetMeshInfoEntry(const MeshInfoHandle handle) const
    {
        if (m_meshInfoData.size() > handle.GetIndex())
        {
            return m_meshInfoData[handle.GetIndex()];
        }
        return m_emptyEntry;
    }

    void CreateShaderInputContract(
        RPI::ShaderInputContract& contract, AZStd::vector<RHI::Format>& channelFormat, RPI::ShaderOptionGroup& streamBoundShaderOptions)
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

        AddStreamChannel("POSITION", RHI::Format::R32G32B32_FLOAT);
        AddStreamChannel("NORMAL", RHI::Format::R32G32B32_FLOAT);
        AddStreamChannel("TANGENT", RHI::Format::R32G32B32A32_FLOAT, true);
        AddStreamChannel("BITANGENT", RHI::Format::R32G32B32_FLOAT, true);
        AddIndexedStreamChannel("UV", 0, RHI::Format::R32G32_FLOAT, true);
        AddIndexedStreamChannel("UV", 1, RHI::Format::R32G32_FLOAT, true);
        // TODO: Vertex-color and BlendMask as optional inputs
    }

    void MeshInfoManager::InitMeshInfoGeometryBuffers(
        const RPI::Model* model,
        const int lod,
        const int meshIndex,
        const RPI::Material* material,
        const RPI::MaterialModelUvOverrideMap& uvMapping,
        MeshInfoEntry* entry)
    {
        auto& modelLod = model->GetLods()[lod];
        auto& mesh = modelLod->GetMeshes()[meshIndex];

        RPI::ShaderInputContract inputContract;
        AZStd::vector<RHI::Format> inputChannelFormat;

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

        CreateShaderInputContract(inputContract, inputChannelFormat, entry->m_optionalInputStreamShaderOptions);

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

        // TODO: deal with shader options that mark optional input streams somehow
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
            // - A new, 'raw' BufferView for the entire geometry-buffer (TODO: we could deduplicate this)
            // - The bindless read-index of this new BufferView
            // - The start-offset inside the new BufferView
            const auto semantic = inputContract.m_streamChannels[streamIndex].m_semantic;
            entry->m_meshBuffers[semantic] = BufferViewIndexAndOffset::Create(streamIter[streamIndex], inputChannelFormat[streamIndex]);
        }

        // Register the Index buffer
        entry->m_indexBuffer = IndexBufferViewIndexAndOffset::Create(mesh.GetIndexBufferView());
    }

} // namespace AZ::Render