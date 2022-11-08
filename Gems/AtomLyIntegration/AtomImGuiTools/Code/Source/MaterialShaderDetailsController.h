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
    //! Provides some additional utilities for the interaction between AtomImGuiToolsSystemComponent and ImGuiMaterialDetails.
    class MaterialShaderDetailsController
    {
    public:
        //! Sets which entity who's draw packets will be displayed in ImGuiMaterialDetails
        void SetSelectedEntityId(AZ::EntityId entityId);

        //! Returns the collection of MeshDrawPackets currently used by the selected entity.
        const AZ::RPI::MeshDrawPacketLods* GetMeshDrawPackets() const;

        //! Returns the name of the selected entity.
        AZStd::string GetSelectionName() const;

    private:

        AZ::EntityId m_materialDetailsSelectedEntityId;
    };

} // namespace AtomImGuiTools
