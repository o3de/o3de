/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Viewport/WhiteBoxViewportConstants.h"

#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    class ReflectContext;
}

namespace WhiteBox
{
    //! The properties of a WhiteBox rendering material.
    struct WhiteBoxMaterial
    {
        AZ_TYPE_INFO(WhiteBoxMaterial, "{234B98F5-0891-479A-8B5E-E18DD8F9E454}");
        static void Reflect(AZ::ReflectContext* context);

        //! Diffuse color tint for render material.
        AZ::Vector3 m_tint = DefaultMaterialTint;

        //! Flag for whether the textured material (true) or solid color material (false) will be used.
        bool m_useTexture = DefaultMaterialUseTexture;

        //! Flag for whether the material will be visible in game mode (true) or not (false).
        bool m_visible = DefaultVisibility;
    };
} // namespace WhiteBox
