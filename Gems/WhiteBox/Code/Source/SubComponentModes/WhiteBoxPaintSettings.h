/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>

namespace WhiteBox
{
    enum class FacePaintOperation
    {
        Material,
        Color,
        ResetMaterial,
        ResetColor
    };

    //! Editor brush settings. Face assignments themselves are stored in the mesh.
    struct FacePaintSettings
    {
        FacePaintOperation m_operation = FacePaintOperation::Material;
        AZ::Data::AssetId m_material;
        AZ::u32 m_color = 0xFFFFFFFF; //!< Opaque RGBA, R in the low byte.
    };
}
