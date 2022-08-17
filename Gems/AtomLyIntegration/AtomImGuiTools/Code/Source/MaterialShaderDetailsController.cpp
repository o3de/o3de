/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MaterialShaderDetailsController.h>
#include <AtomLyIntegration/AtomImGuiTools/AtomImGuiToolsBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>

namespace AtomImGuiTools
{

    void MaterialShaderDetailsController::SetSelectedEntityId(AZ::EntityId entityId)
    {
        m_materialDetailsSelectedEntityId = entityId;
    }

    const AZ::RPI::MeshDrawPacketLods* MaterialShaderDetailsController::GetMeshDrawPackets() const
    {
        const AZ::RPI::MeshDrawPacketLods* drawPackets = nullptr;
        AtomImGuiMeshCallbackBus::EventResult(drawPackets, m_materialDetailsSelectedEntityId, &AtomImGuiMeshCallbacks::GetDrawPackets);
        return drawPackets;
    }

    const AZ::RPI::MeshDrawPacket* MaterialShaderDetailsController::FindDrawPacket(
        const AZ::Render::MaterialAssignmentId& materialAssignmentId, AZ::Data::Instance<AZ::RPI::Material> material) const
    {
        using namespace AZ::Render;
        using namespace AZ::RPI;

        const MeshDrawPacketLods* drawPacketLists = GetMeshDrawPackets();
        if (drawPacketLists)
        {
            // If we know the specific LOD and material slot, we can select the exact draw packet being requested
            if (materialAssignmentId.IsLodAndSlotId())
            {
                if (materialAssignmentId.m_lodIndex < drawPacketLists->size())
                {
                    for (const MeshDrawPacket& drawPacket : (*drawPacketLists)[materialAssignmentId.m_lodIndex])
                    {
                        if (drawPacket.GetMesh().m_materialSlotStableId == materialAssignmentId.m_materialSlotStableId)
                        {
                            return &drawPacket;
                        }
                    }
                }
            }

            // If we don't have a specific LOD and material slot, then materialAssignmentId refers to one of the general overrides that
            // can be applied to any mesh or any LOD, so search for any draw packet using this material.
            if (material)
            {
                for (MaterialAssignmentLodIndex lod = 0; lod < drawPacketLists->size(); ++lod)
                {
                    for (const MeshDrawPacket& drawPacket : (*drawPacketLists)[lod])
                    {
                        if (drawPacket.GetMaterial() == material)
                        {
                            return &drawPacket;
                        }
                    }
                }
            }
        }

        return nullptr;
    }

    AZStd::string MaterialShaderDetailsController::GetSelectionName() const
    {
        AZStd::string name;
        AZ::ComponentApplicationBus::BroadcastResult(name, &AZ::ComponentApplicationRequests::GetEntityName, m_materialDetailsSelectedEntityId);
        if (!name.empty())
        {
            name = AZStd::string::format("Entity \"%s\"", name.c_str());
        }
        return name;
    }

} // namespace AtomImGuiTools
