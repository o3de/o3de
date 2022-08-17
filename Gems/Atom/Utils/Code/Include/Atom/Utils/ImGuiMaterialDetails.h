/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include<AtomCore/Instance/Instance.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>

namespace AZ
{
    namespace RPI
    {
        class Material;
    }
}

namespace AZ
{
    namespace Render
    {
        //! Provides a dialog that shows technical details about individual materials, for example the list of
        //! shaders, the shader options requested, and which shader variant was selected.
        class ImGuiMaterialDetails
        {
        public:
            void SetMaterial(AZ::Data::Instance<AZ::RPI::Material> material);

            void OpenDialog();
            void CloseDialog();
            void Tick();
            void Tick(const char* selectionName, const AZ::RPI::MeshDrawPacketLods* drawPackets);

        private:

            AZ::Data::Instance<AZ::RPI::Material> m_material;
            bool m_dialogIsOpen = false;
            size_t m_selectedLod = 0;
            size_t m_selectedDrawPacket = 0;
        };
    }
} 


#include "ImGuiMaterialDetails.inl"
