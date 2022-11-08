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
