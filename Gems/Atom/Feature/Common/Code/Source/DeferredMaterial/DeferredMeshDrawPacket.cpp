/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Material/Material.h>
#include <DeferredMaterial/DeferredMeshDrawPacket.h>

namespace AZ
{
    namespace Render
    {

        DeferredMeshDrawPacket::DeferredMeshDrawPacket(
            Data::Instance<RPI::ModelLod> modelLod, size_t modelLodMeshIndex, Data::Instance<RPI::Material> materialOverride)
        {
            m_modelLod = modelLod;
            m_modelLodMeshIndex = modelLodMeshIndex;

            if (materialOverride != nullptr)
            {
                m_material = materialOverride;
            }
            else
            {
                const RPI::ModelLod::Mesh& mesh = m_modelLod->GetMeshes()[modelLodMeshIndex];
                m_material = mesh.m_material;
            }
        }

        auto DeferredMeshDrawPacket::GetDeferredDrawPacket(const RHI::DrawListTag& drawListTag) -> RHI::Ptr<DeferredDrawPacket>
        {
            if (m_deferredDrawPackets.contains(drawListTag))
            {
                return m_deferredDrawPackets.at(drawListTag);
            }
            return {};
        }

        void DeferredMeshDrawPacket::Update(RPI::Scene* scene, DeferredDrawPacketManager* manager, bool forceRebuild)
        {
            if (m_materialChangeId == m_material->GetCurrentChangeId() && forceRebuild == false)
            {
                return;
            }

            // This doesn't mean the draw-packets will be recreated, since the manager keeps a reference to them
            // But if our material changed sufficiently enough, we will get a new one
            m_deferredDrawPackets.clear();

            m_material->ApplyGlobalShaderOptions();

            m_material->ForAllShaderItems(
                [&](const Name& materialPipelineName, const RPI::ShaderCollection::Item& shaderItem)
                {
                    if (shaderItem.IsEnabled() && shaderItem.GetDrawItemType() == RPI::ShaderCollection::Item::DrawItemType::Deferred)
                    {
                        auto deferredDrawPacket =
                            manager->GetOrCreateDeferredDrawPacket(scene, m_material.get(), materialPipelineName, shaderItem);
                        m_deferredDrawPackets[deferredDrawPacket->GetDrawListTag()] = deferredDrawPacket;
                    }
                    return true;
                });
            m_materialChangeId = m_material->GetCurrentChangeId();
            return;
        }

    } // namespace Render
} // namespace AZ