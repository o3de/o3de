/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/DeferredMaterial/DeferredMaterialFeatureProcessor.h>
#include <Atom/Feature/RenderCommon.h>
#include <Atom/RHI/RHIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Mesh/MeshFeatureProcessor.h>

namespace AZ::Render
{
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

            for (auto lod = 0; lod < lodCount; lod++)
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
            for (auto lod = 0; lod < lodCount; lod++)
            {
                auto& modelLodData = modelData.m_lodData[lod];
                auto meshCount = static_cast<uint32_t>(modelLodData.m_meshData.size());
                for (auto meshIndex = 0; meshIndex < meshCount; meshIndex++)
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

    Data::Instance<RPI::Buffer>& DeferredMaterialFeatureProcessor::GetOrCreateDrawPacketIdBuffer(
        const RHI::DrawListTag& drawListTag, const size_t numEntriesHint)
    {
        auto& current = m_frameData.GetCurrentElement();
        auto name = AZStd::string::format("drawPacketIdBuffer_%d_%d", drawListTag.GetIndex(), m_frameData.GetCurrentElementIndex());
        CreateOrResizeBuffer<int32_t>(current.m_drawPacketIdBuffers[drawListTag], name, numEntriesHint);
        return current.m_drawPacketIdBuffers[drawListTag];
    }

    void DeferredMaterialFeatureProcessor::UpdateDrawPacketIdBuffers()
    {
        // figure out which drawListTags are currently in use
        auto tagRegistry = RHI::GetDrawListTagRegistry();
        AZStd::vector<RHI::DrawListTag> drawListTags;

        tagRegistry->VisitTags(
            [&](AZ::Name drawListTagName, RHI::DrawListTag tag)
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
            AZStd::vector<int32_t> drawPacketIds;
            drawPacketIds.resize(numEntries, -1);
            for (auto& [modelId, modelData] : m_modelData)
            {
                MeshIterator::ForEachLodMesh(
                    modelData,
                    [&, drawListTag = drawListTag](
                        const uint32_t lod,
                        const uint32_t meshIndex,
                        DeferredMaterialFeatureProcessor::ModelData& modelData,
                        DeferredMaterialFeatureProcessor::MeshData& meshData)
                    {
                        auto drawPacket = meshData.m_meshDrawPacket.GetDeferredDrawPacket(drawListTag);
                        if (drawPacket)
                        {
                            drawPacketIds[meshData.m_meshInfoIndex] = drawPacket->GetDrawPacketId();
                        }
                        else
                        {
                            drawPacketIds[meshData.m_meshInfoIndex] = -1;
                        }
                        return true;
                    });
            }

            auto buffer = GetOrCreateDrawPacketIdBuffer(drawListTag, drawPacketIds.size());
            auto const drawPacketIdBufferSize = drawPacketIds.size() * sizeof(int32_t);
            buffer->UpdateData(drawPacketIds.data(), drawPacketIdBufferSize);
        }
    }

    void DeferredMaterialFeatureProcessor::AddModel(
        const ModelId& modelId, ModelDataInstanceInterface* meshHandle, const Data::Instance<RPI::Model>& model)
    {
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
            [&](const uint32_t lod, const uint32_t meshIndex, const RPI::ModelLodAsset::Mesh& assetMesh, const RPI::ModelLod::Mesh& mesh)
            {
                auto& modelLodData = modelData.m_lodData[lod];

                // retrieve the material
                const CustomMaterialId customMaterialId(lod, mesh.m_materialSlotStableId);
                const auto& customMaterialInfo = meshHandle->GetCustomMaterialWithFallback(customMaterialId);

                const auto modelLod = model->GetLods()[lod];

                auto drawPacket = DeferredMeshDrawPacket{ modelLod, meshIndex, customMaterialInfo.m_material };

                modelLodData.m_meshData.emplace_back(MeshData{ meshHandle->GetMeshInfoIndex(lod, meshIndex), drawPacket });
                return true;
            });
    }

    void DeferredMaterialFeatureProcessor::RemoveModel(const ModelId& modelId)
    {
        AZStd::scoped_lock lock(m_updateMutex);

        if (m_modelData.find(modelId) != m_modelData.end())
        {
            m_modelData.erase(modelId);
        }
        m_needsUpdate = true;
    }

    AZ::Data::Instance<AZ::RPI::Buffer> DeferredMaterialFeatureProcessor::GetDrawPacketIdBuffer(const RHI::DrawListTag& drawListTag)
    {
        auto& current = m_frameData.GetCurrentElement();
        if (current.m_drawPacketIdBuffers.contains(drawListTag))
        {
            return current.m_drawPacketIdBuffers[drawListTag];
        }
        return nullptr;
    }

    void DeferredMaterialFeatureProcessor::UpdateMeshDrawPackets(bool forceRebuild)
    {
        for (auto& [modelId, modelData] : m_modelData)
        {
            MeshIterator::ForEachLodMesh(
                modelData,
                [&](const uint32_t lod,
                    const uint32_t meshIndex,
                    DeferredMaterialFeatureProcessor::ModelData& modelData,
                    DeferredMaterialFeatureProcessor::MeshData& meshData)
                {
                    meshData.m_meshDrawPacket.Update(GetParentScene(), &m_drawPacketManager, forceRebuild);
                    return true;
                });
        }
    }

    void DeferredMaterialFeatureProcessor::Render(const RenderPacket& renderPacket)
    {
        if (m_needsUpdate || m_drawPacketManager.GetNeedsUpdate() || m_globalShaderOptionsChanged)
        {
            m_frameData.AdvanceCurrentElement();

            // Refresh the references from the MeshDrawPacket to the DeferredDrawPackets and create them on demand
            UpdateMeshDrawPackets(m_globalShaderOptionsChanged);

            // Remove DeferredDrawPackets that aren't referenced anymore
            m_drawPacketManager.PruneUnusedDrawPackets();

            // recreate the drawPacketId - buffers: This needs the draw-Packet ID from the prepared draw Packets
            UpdateDrawPacketIdBuffers();

            // Finalize the deferred draw-packets: This needs the drawPacket ID buffer in the Draw-Srg
            UpdateDrawSrgs();
            m_needsUpdate = false;
            m_globalShaderOptionsChanged = false;
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
        RPI::RenderPipeline* renderPipeline, RPI::SceneNotification::RenderPipelineChangeType changeType)
    {
        // force a rebuild of the draw packets and update the buffers
        m_needsUpdate = true;
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
        m_handleGlobalShaderOptionUpdate =
            RPI::ShaderSystemInterface::GlobalShaderOptionUpdatedEvent::Handler{ [this](const AZ::Name&, RPI::ShaderOptionValue)
                                                                                 {
                                                                                     m_globalShaderOptionsChanged = true;
                                                                                 } };
        RPI::ShaderSystemInterface::Get()->Connect(m_handleGlobalShaderOptionUpdate);

        EnableSceneNotification();
    }

    void DeferredMaterialFeatureProcessor::Deactivate()
    {
        DisableSceneNotification();
    }

} // namespace AZ::Render
