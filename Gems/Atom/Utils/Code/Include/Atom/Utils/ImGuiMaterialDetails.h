/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include<AtomCore/Instance/Instance.h>

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
            void Tick();

        private:

            AZ::Data::Instance<AZ::RPI::Material> m_material;
            bool m_dialogIsOpen = false;
        };
    }
} 


#include "ImGuiMaterialDetails.inl"
