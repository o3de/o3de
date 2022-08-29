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
            //! Sets which draw packet should be selected for viewing. This draw packet must appear in the 
            //! list provided to the Tick() function, or this selection will be ignored.
            void SetSelectedDrawPacket(const RPI::MeshDrawPacket* drawPacket);

            void OpenDialog();
            void CloseDialog();

            //! Per-frame UI draw function. 
            //! @param drawPackets is the tree of draw packets that should be listed for user selection. The selected draw packet
            //!                    will have its shader list displayed with debug information.
            //! @param selectionName is an optional name to be displayed at the top, indicating what entity or what list of draw packets is displayed.
            bool Tick(const AZ::RPI::MeshDrawPacketLods* drawPackets, const char* selectionName = nullptr);

        private:

            bool m_dialogIsOpen = false;

            // There are multiple ways that draw packet selection can be recalled
            const RPI::MeshDrawPacket* m_selectedDrawPacket = nullptr;
            size_t m_selectedLod = 0;
            size_t m_selectedDrawPacketIndex = 0;


        };
    }
} 


#include "ImGuiMaterialDetails.inl"
