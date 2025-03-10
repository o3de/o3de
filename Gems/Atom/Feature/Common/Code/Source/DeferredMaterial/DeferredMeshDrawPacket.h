
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Model/ModelLod.h>
#include <DeferredMaterial/DeferredDrawPacket.h>
#include <DeferredMaterial/DeferredDrawPacketManager.h>

namespace AZ
{
    namespace Render
    {
        // the DeferredMeshDrawPacket is not a draw-packet as such, but it holds a reference to the actual
        // deferred Drawpacket for the Material, since that is generally shared between multiple meshes
        // We use the reference count to figure out if the DeferredDrawPacket is still needed.
        class DeferredMeshDrawPacket
        {
        public:
            DeferredMeshDrawPacket() = default;
            DeferredMeshDrawPacket(
                Data::Instance<RPI::ModelLod> modelLod, size_t modelLodMeshIndex, Data::Instance<RPI::Material> materialOverride);

            AZ_DEFAULT_COPY(DeferredMeshDrawPacket);
            AZ_DEFAULT_MOVE(DeferredMeshDrawPacket);

            auto GetDeferredDrawPacket(const RHI::DrawListTag& drawList) -> Data::Instance<DeferredDrawPacket>;

            void Update(RPI::Scene* scene, DeferredDrawPacketManager* manager, bool forceRebuild);

        private:
            // a mesh can have a DeferredDrawPacket for multiple DrawListTags
            AZStd::unordered_map<RHI::DrawListTag, Data::Instance<DeferredDrawPacket>> m_deferredDrawPackets;
            Data::Instance<RPI::ModelLod> m_modelLod;
            size_t m_modelLodMeshIndex;
            Data::Instance<RPI::Material> m_material;
            // Tracks whether the Material has change since the DrawPacket was last built
            RPI::Material::ChangeId m_materialChangeId = RPI::Material::DEFAULT_CHANGE_ID;
        };

    } // namespace Render
} // namespace AZ
