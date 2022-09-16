/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Console/IConsole.h>

namespace WhiteBox
{
    AZ_CVAR_EXTERNED(float, cl_whiteBoxVertexManipulatorSize);
    AZ_CVAR_EXTERNED(float, cl_whiteBoxMouseClickDeltaThreshold);
    AZ_CVAR_EXTERNED(float, cl_whiteBoxModifierMidpointEpsilon);
    AZ_CVAR_EXTERNED(float, cl_whiteBoxEdgeVisualWidth);
    AZ_CVAR_EXTERNED(float, cl_whiteBoxEdgeSelectionWidth);
    AZ_CVAR_EXTERNED(float, cl_whiteBoxSelectedEdgeVisualWidth);
    
    AZ_CVAR_EXTERNED(AZ::Color, ed_whiteBoxVertexHiddenColor);
    AZ_CVAR_EXTERNED(AZ::Color, ed_whiteBoxVertexRestoredColor);

    AZ_CVAR_EXTERNED(AZ::Color, ed_whiteBoxVertexUnselected);
    AZ_CVAR_EXTERNED(AZ::Color, ed_whiteBoxEdgeUnselected);
    AZ_CVAR_EXTERNED(AZ::Color, ed_whiteBoxEdgeDefault);

    AZ_CVAR_EXTERNED(AZ::Color, ed_whiteBoxVertexHover);
    AZ_CVAR_EXTERNED(AZ::Color, ed_whiteBoxPolygonHover);
    AZ_CVAR_EXTERNED(AZ::Color, ed_whiteBoxOutlineHover);

    AZ_CVAR_EXTERNED(AZ::Color, ed_whiteBoxPolygonSelection);
    AZ_CVAR_EXTERNED(AZ::Color, ed_whiteBoxVertexSelection);
    AZ_CVAR_EXTERNED(AZ::Color, ed_whiteBoxOutlineSelection);
    AZ_CVAR_EXTERNED(float, ed_whiteBoxPolygonViewOverlapOffset);

    //! Smallest area squared for a triangle to still be considered valid.
    inline const float DegenerateTriangleAreaSquareEpsilon{std::numeric_limits<float>::epsilon()};

    //! Default tint colour for WhiteBox render material.
    inline const AZ::Vector3 DefaultMaterialTint = AZ::Vector3::CreateOne();

    //! Default texture type for WhiteBox render material.
    inline const bool DefaultMaterialUseTexture = true;

    //! Default game mode visibility for WhiteBox render material.
    inline const bool DefaultVisibility = true;
} // namespace WhiteBox
