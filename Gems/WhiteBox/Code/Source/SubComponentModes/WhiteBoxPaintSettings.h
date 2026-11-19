/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
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
        ResetColor,
        BlendLayer2, //!< Brush vertex-blend weights towards layer 2 of a vertex-blend material.
        BlendLayer3, //!< Brush towards layer 3 (it wins over layer 2 where both are painted).
        BlendBase    //!< Brush back towards the base layer.
    };

    inline bool IsBlendOperation(const FacePaintOperation operation)
    {
        return operation == FacePaintOperation::BlendLayer2 || operation == FacePaintOperation::BlendLayer3 ||
            operation == FacePaintOperation::BlendBase;
    }

    //! Editor brush settings. Face assignments themselves are stored in the mesh.
    struct FacePaintSettings
    {
        FacePaintOperation m_operation = FacePaintOperation::Material;
        AZ::Data::AssetId m_material;
        AZ::u32 m_color = 0xFFFFFFFF; //!< Opaque RGBA, R in the low byte.
        //! Paint every triangle of the hovered polygon, not just the one under the cursor.
        bool m_wholePolygon = true;
        float m_brushRadius = 0.5f;   //!< Blend brush radius in metres.
        float m_brushStrength = 0.35f; //!< How far each dab moves the weights, 0 to 1.
        float m_brushHardness = 0.3f;  //!< 0 fades from the centre, 1 is full strength to the rim.
    };
}
