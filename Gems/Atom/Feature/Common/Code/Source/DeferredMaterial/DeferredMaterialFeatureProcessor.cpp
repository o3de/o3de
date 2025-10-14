/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DeferredMaterialFeatureProcessor.h"
#include <Atom/Feature/RenderCommon.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Mesh/MeshFeatureProcessor.h>

namespace AZ::Render
{

    AZ_CVAR(
        bool,
        r_deferredRenderingEnabled,
        false,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "Enable deferred material draw calls in the MeshFeatureProcessor and the DeferredMaterialFeatureProcessor.");

    constexpr static size_t MeshInfoMinEntries = 32;

    // Helper class that iterates over all meshes in all lods of either a RPI::Model or a DeferredMaterialFeatureProcessor::ModelData.
    class MeshIterator
    {
    public:
        static void ForEachLodMesh(
            const Data::Instance<RPI::Model>& model,
            AZStd::function<bool(const uint32_t, const uint32_t, const RPI::ModelLodAsset::Mesh&, const RPI::ModelLod::Mesh&)> callback)
        {
            const auto& modelAsset = model->GetModelAsset();
            const AZStd::span<const Data::Asset<RPI::ModelLodAsset>>& modelLodAssets = modelAsset->GetLodAssets();
            const AZStd::span<const Data::Instance<RPI::ModelLod>>& modelLods = model->GetLods();
            const auto lodCount = static_cast<uint32_t>(model->GetLodCount());

            for (auto lod = 0u; lod < lodCount; lod++)
            {
                const Data::Instance<RPI::ModelLod>& modelLod = modelLods[lod];
                const Data::Asset<RPI::ModelLodAsset>& modelLodAsset = modelLodAssets[lod];
                const auto meshCount = static_cast<uint32_t>(modelLodAsset->GetMeshes().size());

                for (uint32_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
                {
                    const auto assetMeshes = modelLodAsset->GetMeshes();
                    const RPI::ModelLodAsset::Mesh& assetMesh = assetMeshes[meshIndex];

                    const auto meshes = modelLod->GetMeshes();
                    const RPI::ModelLod::Mesh& mesh = meshes[meshIndex];

                    if (!callback(lod, meshIndex, assetMesh, mesh))
                    {
                        return;
                    }
                }
            }
        }

        static void ForEachLodMesh(
            DeferredMaterialFeatureProcessor::ModelData& modelData,
            AZStd::function<bool(
                const uint32_t, const uint32_t, DeferredMaterialFeatureProcessor::ModelData&, DeferredMaterialFeatureProcessor::MeshData&)>
                callback)
        {
            auto lodCount = static_cast<uint32_t>(modelData.m_lodData.size());
            for (auto lod = 0u; lod < lodCount; lod++)
            {
                auto& modelLodData = modelData.m_lodData[lod];
                auto meshCount = static_cast<uint32_t>(modelLodData.m_meshData.size());
                for (auto meshIndex = 0u; meshIndex < meshCount; meshIndex++)
                {
                    auto& mesh = modelLodData.m_meshData[meshIndex];
                    if (!callback(lod, meshIndex, modelData, mesh))
                    {
                        return;
                    }
                }
            }
        }
    };

    void DeferredMaterialFeatureProcessor::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<DeferredMaterialFeatureProcessor, FeatureProcessor>()->Version(1);
        }
    }

    template<typename T>
    void CreateOrResizeBuffer(Data::Instance<RPI::Buffer>& buffer, const AZStd::string& name, const size_t minNumEntries)
    {
        // we need one entry per meshinfo-entry, so we can use the same Min number of entries
        const auto numEntries = AlignUpToPowerOfTwo(AZStd::max(minNumEntries, MeshInfoMinEntries));
        auto constexpr elementSize = sizeof(T);
        auto const bufferSize = numEntries * elementSize;

        if (buffer == nullptr)
        {
            // Create an empty RPI buffer, it will be updated with data later
            AZ::RPI::CommonBufferDescriptor desc;
            //! Note: If this buffer is bound to a StructuredBuffer, the format has to be unknown.
            //! or we get the error message Buffer Input 'm_meshInfoBuffer[0]': Does not match expected type 'Structured'
            desc.m_elementFormat = AZ::RHI::Format::Unknown;
            //! needs to be ReadWrite, or it can't be bound to RPI Slots for some reason
            desc.m_poolType = AZ::RPI::CommonBufferPoolType::ReadWrite;
            desc.m_elementSize = static_cast<uint32_t>(elementSize);
            desc.m_bufferName = name;
            // allocate size for a few objects
            desc.m_byteCount = bufferSize;
            buffer = AZ::RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        }
        else if (buffer->GetBufferSize() < bufferSize)
        {
            buffer->Resize(bufferSize);
        }
    }

    RPI::RingBuffer& DeferredMaterialFeatureProcessor::GetOrCreateDrawPacketIdRingBuffer(const RHI::DrawListTag& drawListTag)
    {
        auto ringBufferIt = m_drawPacketIdBuffers.find(drawListTag);
        if (ringBufferIt == m_drawPacketIdBuffers.end())
        {
            auto tagRegistry = RHI::GetDrawListTagRegistry();
            auto name = AZStd::string::format("drawPacketIdBuffer_%s", tagRegistry->GetName(drawListTag).GetCStr());
            auto [newIt, inserted] = m_drawPacketIdBuffers.emplace(
                AZStd::make_pair(drawListTag, RPI::RingBuffer{ name, RPI::CommonBufferPoolType::ReadOnly, RHI::Format::R32_UINT }));
            return newIt->second;
        }
        return ringBufferIt->second;
    }

    void DeferredMaterialFeatureProcessor::UpdateDrawPacketIdBuffers()
    {
        // figure out which drawListTags are currently in use
        auto tagRegistry = RHI::GetDrawListTagRegistry();
        AZStd::vector<RHI::DrawListTag> drawListTags;

        tagRegistry->VisitTags(
            [&]([[maybe_unused]] AZ::Name drawListTagName, RHI::DrawListTag tag)
            {
                if (m_drawPacketManager.HasDrawPacketForDrawList(tag))
                {
                    drawListTags.emplace_back(tag);
                }
            });

        auto* mfp = GetParentScene()->GetFeatureProcessor<MeshFeatureProcessor>();
        const auto numEntries = AZStd::max(mfp->GetMeshInfoManager().GetMaxMeshInfoIndex(), 1);

        // create one entry per mesh for each DrawListTag
        for (auto& drawListTag : drawListTags)
        {
            AZStd::vector<uint32_t> drawPacketIds;
            drawPacketIds.resize(numEntries, AZStd::numeric_limits<uint32_t>::max());
            for (auto& [modelId, modelData] : m_modelData)
            {
                MeshIterator::ForEachLodMesh(
                    modelData,
                    [&, drawListTag = drawListTag](
                        [[maybe_unused]] const uint32_t lod,
                        [[maybe_unused]] const uint32_t meshIndex,
                        [[maybe_unused]] DeferredMaterialFeatureProcessor::ModelData& modelData,
                        DeferredMaterialFeatureProcessor::MeshData& meshData)
                    {
                        auto drawPacket = meshData.m_meshDrawPacket.GetDeferredDrawPacket(drawListTag);
                        if (drawPacket)
                        {
                            drawPacketIds[meshData.m_meshInfoIndex] = drawPacket->GetDrawPacketId();
                        }
                        else
                        {
                            drawPacketIds[meshData.m_meshInfoIndex] = AZStd::numeric_limits<uint32_t>::max();
                        }
                        return true;
                    });
            }

            GetOrCreateDrawPacketIdRingBuffer(drawListTag).AdvanceCurrentBufferAndUpdateData(drawPacketIds);
        }
    }

    void DeferredMaterialFeatureProcessor::AddModel(
        const ModelId& modelId, ModelDataInstanceInterface* meshHandle, const Data::Instance<RPI::Model>& model)
    {
        if (!m_enabled)
        {
            return;
        }

        AZStd::scoped_lock lock(m_updateMutex);

        if (m_modelData.find(modelId) != m_modelData.end())
        {
            return;
        }

        auto& modelData = m_modelData[modelId];
        if (modelData.m_lodData.empty())
        {
            modelData.m_lodData.resize(model->GetLodCount());
        }

        m_needsUpdate = true;

        MeshIterator::ForEachLodMesh(
            model,
            [&](const uint32_t lod,
                const uint32_t meshIndex,
                [[maybe_unused]] const RPI::ModelLodAsset::Mesh& assetMesh,
                const RPI::ModelLod::Mesh& mesh)
            {
                auto& modelLodData = modelData.m_lodData[lod];

                // retrieve the material
                const CustomMaterialId customMaterialId(lod, mesh.m_materialSlotStableId);
                const auto& customMaterialInfo = meshHandle->GetCustomMaterialWithFallback(customMaterialId);

                const auto modelLod = model->GetLods()[lod];

                auto drawPacket = DeferredMeshDrawPacket{ modelLod, meshIndex, customMaterialInfo.m_material };

                modelLodData.m_meshData.emplace_back(MeshData{ meshHandle->GetMeshInfoIndex(lod, meshIndex), AZStd::move(drawPacket) });
                return true;
            });
    }

    void DeferredMaterialFeatureProcessor::RemoveModel(const ModelId& modelId)
    {
        if (!m_enabled)
        {
            return;
        }

        AZStd::scoped_lock lock(m_updateMutex);

        auto modelDataIter = m_modelData.find(modelId);

        if (modelDataIter != m_modelData.end())
        {
            m_modelData.erase(modelDataIter);
        }

        m_needsUpdate = true;
    }

    AZ::Data::Instance<AZ::RPI::Buffer> DeferredMaterialFeatureProcessor::GetDrawPacketIdBuffer(const RHI::DrawListTag& drawListTag)
    {
        auto bufferIt = m_drawPacketIdBuffers.find(drawListTag);
        if (bufferIt != m_drawPacketIdBuffers.end() && bufferIt->second.IsCurrentBufferValid())
        {
            return bufferIt->second.GetCurrentBuffer();
        }
        return nullptr;
    }

    void DeferredMaterialFeatureProcessor::UpdateMeshDrawPackets(bool forceRebuild)
    {
        for (auto& [modelId, modelData] : m_modelData)
        {
            MeshIterator::ForEachLodMesh(
                modelData,
                [&]([[maybe_unused]] const uint32_t lod,
                    [[maybe_unused]] const uint32_t meshIndex,
                    [[maybe_unused]] DeferredMaterialFeatureProcessor::ModelData& modelData,
                    DeferredMaterialFeatureProcessor::MeshData& meshData)
                {
                    meshData.m_meshDrawPacket.Update(GetParentScene(), &m_drawPacketManager, forceRebuild);
                    return true;
                });
        }
    }

    void DeferredMaterialFeatureProcessor::Render(const RenderPacket& renderPacket)
    {
        if (!m_enabled)
        {
            return;
        }

        if (m_needsUpdate || m_drawPacketManager.GetNeedsUpdate() || m_forceRebuild)
        {
            // Refresh the references from the MeshDrawPacket to the DeferredDrawPackets and create them on demand
            UpdateMeshDrawPackets(m_forceRebuild);

            // Remove DeferredDrawPackets that aren't referenced anymore
            m_drawPacketManager.PruneUnusedDrawPackets();

            // recreate the drawPacketId - buffers: This needs the draw-Packet ID from the prepared draw Packets
            UpdateDrawPacketIdBuffers();

            // Finalize the deferred draw-packets: This needs the drawPacket ID buffer in the Draw-Srg
            UpdateDrawSrgs();
            m_needsUpdate = false;
            m_forceRebuild = false;
            m_drawPacketManager.SetNeedsUpdate(false);

#ifdef DEFERRED_DRAWPACKET_DEBUG_PRINT
            AZ_Info(
                "DeferredMaterialFeatureProcessor",
                "Currently %lld active deferred draw-packets",
                m_drawPacketManager.GetDrawPackets().size());
            for (auto& [unique_id, drawPacket] : m_drawPacketManager.GetDrawPackets())
            {
                AZ_Info(
                    "DeferredMaterialFeatureProcessor",
                    "    Id %lld, MaterialType %s, Instigating Material %s",
                    unique_id,
                    drawPacket->GetInstigatingMaterialTypeAsset().GetHint().c_str(),
                    drawPacket->GetInstigatingMaterialAsset().GetHint().c_str());
            }
#endif /* DEFERRED_DRAWPACKET_DEBUG_PRINT */
        }

        for (const AZ::RPI::ViewPtr& view : renderPacket.m_views)
        {
            if (view->GetUsageFlags() & AZ::RPI::View::UsageCamera)
            {
                for (auto& [_, drawPacket] : m_drawPacketManager.GetDrawPackets())
                {
                    auto rhiDrawPacket = drawPacket->GetRHIDrawPacket();
                    if (rhiDrawPacket)
                    {
                        view->AddDrawPacket(rhiDrawPacket);
                    }
                }
            }
        }
    }

    void DeferredMaterialFeatureProcessor::OnRenderPipelineChanged(
        [[maybe_unused]] RPI::RenderPipeline* renderPipeline, [[maybe_unused]] RPI::SceneNotification::RenderPipelineChangeType changeType)
    {
        // force a rebuild of the draw packets and update the buffers
        m_forceRebuild = true;
    }

    void DeferredMaterialFeatureProcessor::UpdateDrawSrgs()
    {
        for (auto& [key, drawPacketData] : m_drawPacketManager.GetDrawPackets())
        {
            auto drawListTag = drawPacketData->GetDrawListTag();
            drawPacketData->CompileDrawSrg(GetDrawPacketIdBuffer(drawListTag));
        }
    }

    void DeferredMaterialFeatureProcessor::Activate()
    {
        if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            console->GetCvarValue("r_deferredRenderingEnabled", m_enabled);
        }
        if (m_enabled)
        {
            m_handleGlobalShaderOptionUpdate =
                RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler{ [this](const AZ::Name&, RPI::ShaderOptionValue)
                                                                                     {
                                                                                         m_forceRebuild = true;
                                                                                     } };
            RPI::ShaderSystemInterface::Get()->Connect(m_handleGlobalShaderOptionUpdate);

            EnableSceneNotification();
        }
    }

    void DeferredMaterialFeatureProcessor::Deactivate()
    {
        if (m_enabled)
        {
            DisableSceneNotification();
        }
    }

} // namespace AZ::Render
