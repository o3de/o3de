/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>
#include <DeferredMaterial/DeferredDrawPacketManager.h>

namespace AZ
{
    namespace Render
    {
        auto DeferredDrawPacketManager::CalculateDrawPacketId(const RPI::Material* material, const RPI::ShaderCollection::Item& shaderItem)
            -> DeferredDrawPacketId
        {
            RPI::ShaderOptionGroup shaderOptions = *shaderItem.GetShaderOptions();
            shaderOptions.SetUnspecifiedToDefaultValues();
            auto requestedShaderVariantId = shaderOptions.GetShaderVariantId();

            DeferredDrawPacketId seed{};
            AZStd::hash_combine(seed, material->GetMaterialTypeId());
            AZStd::hash_combine(seed, requestedShaderVariantId);

            return seed;
        }

        auto DeferredDrawPacketManager::GetDeferredDrawPacket(DeferredDrawPacketId id) const -> Data::Instance<DeferredDrawPacket>
        {
            if (HasDeferredDrawPacket(id))
            {
                return m_deferredDrawPackets.at(id);
            }
            return {};
        }

        bool DeferredDrawPacketManager::HasDeferredDrawPacket(DeferredDrawPacketId id) const
        {
            return m_deferredDrawPackets.contains(id);
        }

        bool DeferredDrawPacketManager::HasDrawPacketForDrawList(const RHI::DrawListTag tag) const
        {
            return m_drawListsWithDrawPackets[tag.GetIndex()];
        }

        void DeferredDrawPacketManager::PruneUnusedDrawPackets()
        {
            m_drawListsWithDrawPackets.reset();
            AZStd::vector<DeferredDrawPacketId> toDelete;

            for (auto& [key, data] : m_deferredDrawPackets)
            {
                if (data->GetUseCount() == 1)
                {
                    toDelete.push_back(key);
                }
                else
                {
                    m_drawListsWithDrawPackets[data->GetDrawListTag().GetIndex()] = true;
                }
            }
            for (auto key : toDelete)
            {
                m_deferredDrawPackets.erase(key);
            }
        }

        auto DeferredDrawPacketManager::GetOrCreateDeferredDrawPacket(
            RPI::Scene* scene, RPI::Material* material, const Name materialPipelineName, const RPI::ShaderCollection::Item& shaderItem)
            -> Data::Instance<DeferredDrawPacket>
        {
            auto uniqueId = CalculateDrawPacketId(material, shaderItem);
            auto drawPacket = GetDeferredDrawPacket(uniqueId);

            // the deferred draw-packets don't really support rebuilding, so just create a new one
            if (drawPacket == nullptr || drawPacket->NeedsRebuild())
            {
                // reuse the drawPacketId if we recreate the drawpacket
                auto drawPacketId = drawPacket ? drawPacket->GetDrawPacketId() : static_cast<int32_t>(m_deferredDrawPackets.size());

                drawPacket = aznew DeferredDrawPacket{ this, scene, material, materialPipelineName, shaderItem, drawPacketId };

                m_deferredDrawPackets[uniqueId] = drawPacket;
                m_drawListsWithDrawPackets[drawPacket->GetDrawListTag().GetIndex()] = true;
#ifdef DEFERRED_DRAWPACKET_DEBUG_PRINT
                AZ_Info(
                    "DeferredDrawPacketManager",
                    "Material %s, shader %s: -> Create new draw-packet (MaterialTypeId %d)",
                    material->GetAsset().GetHint().c_str(),
                    shaderItem.GetShaderAsset().GetHint().c_str(),
                    material->GetMaterialTypeId());
#endif /* DEFERRED_DRAWPACKET_DEBUG_PRINT */
            }
#ifdef DEFERRED_DRAWPACKET_DEBUG_PRINT
            else
            {
#ifdef DEFERRED_DRAWPACKET_DEBUG_PRINT
                AZ_Info(
                    "DeferredDrawPacketManager",
                    "Material %s, shader %s: -> Use draw-packet from Material %s (MaterialTypeId %d)",
                    material->GetAsset().GetHint().c_str(),
                    shaderItem.GetShaderAsset().GetHint().c_str(),
                    drawPacket->GetInstigatingMaterialAsset().GetHint().c_str(),
                    material->GetMaterialTypeId());
#endif
            }
#endif /* DEFERRED_DRAWPACKET_DEBUG_PRINT */
            return drawPacket;
        }
    } // namespace Render
} // namespace AZ