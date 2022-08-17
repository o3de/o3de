/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>

namespace AtomImGuiTools
{
    class MaterialShaderDetailsController
    {
    public:
        void SetSelectedEntityId(AZ::EntityId entityId);
        const AZ::RPI::MeshDrawPacketLods* GetMeshDrawPackets() const;
        AZStd::string GetSelectionName() const;

    private:

        AZ::EntityId m_materialDetailsSelectedEntityId;
    };

} // namespace AtomImGuiTools
