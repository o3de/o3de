/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <DeferredMaterial/DeferredDrawPacket.h>

namespace AZ
{
    namespace Render
    {
        // the DeferredDrawPacketManager holds all DeferredDrawPackets of the scene, and deduplicates
        // the Drawpackets from the Meshes
        class DeferredDrawPacketManager
        {
        public:
            using DeferredDrawPacketId = size_t;

            static auto CalculateDrawPacketId(const RPI::Material* material, const RPI::ShaderCollection::Item& shaderItem)
                -> DeferredDrawPacketId;

            auto GetDeferredDrawPacket(DeferredDrawPacketId id) const -> Data::Instance<DeferredDrawPacket>;

            bool HasDeferredDrawPacket(DeferredDrawPacketId id) const;

            auto GetOrCreateDeferredDrawPacket(
                RPI::Scene* scene, RPI::Material* material, const Name materialPipelineName, const RPI::ShaderCollection::Item& shaderItem)
                -> Data::Instance<DeferredDrawPacket>;

            bool HasDrawPacketForDrawList(const RHI::DrawListTag tag) const;

            auto GetDrawPackets() -> AZStd::unordered_map<DeferredDrawPacketId, Data::Instance<DeferredDrawPacket>>&
            {
                return m_deferredDrawPackets;
            }
            void SetNeedsUpdate(const bool needsUpdate)
            {
                m_needsUpdate = needsUpdate;
            }
            bool GetNeedsUpdate() const
            {
                return m_needsUpdate;
            }

            void PruneUnusedDrawPackets();

            Data::Instance<RPI::ShaderResourceGroup> CreatePassSrg(RHI::DrawListTag drawListTag);

        private:
            AZStd::unordered_map<DeferredDrawPacketId, Data::Instance<DeferredDrawPacket>> m_deferredDrawPackets;
            RHI::DrawListMask m_drawListsWithDrawPackets;
            bool m_needsUpdate = false;
        };
    } // namespace Render
} // namespace AZ