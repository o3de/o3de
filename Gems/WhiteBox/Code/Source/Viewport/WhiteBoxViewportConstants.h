/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Console/IConsole.h>
#include <AzCore/std/containers/span.h>

namespace WhiteBox
{
    AZ_CVAR_EXTERNED(float, cl_whiteBoxVertexManipulatorSize); //!< Pick radius of a vertex handle.
    AZ_CVAR_EXTERNED(float, cl_whiteBoxVertexDrawSize); //!< Drawn radius of a vertex handle.
    AZ_CVAR_EXTERNED(float, cl_whiteBoxVertexHoverSize); //!< Drawn radius of a vertex handle under the cursor.
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

    //! A viewport handle size the user can change in the White Box pane. Each one is a console variable, so it can
    //! also be set from the console; the pane saves them per user and the system component restores them on start.
    struct ViewportHandleSetting
    {
        const char* m_key;     //!< Name the value is saved under.
        const char* m_label;
        const char* m_tooltip;
        float m_min;
        float m_max;
        float m_step;
        float m_default;
        float (*m_get)();
        void (*m_set)(float);
    };

    //! Every setting shown in the pane's Viewport Handles section, in display order.
    AZStd::span<const ViewportHandleSetting> ViewportHandleSettings();
    //! Read the saved values into the console variables. Settings that were never saved keep their current value.
    void LoadViewportHandleSettings();
    //! Save the current console variable values.
    void SaveViewportHandleSettings();
    //! Put every setting back to its default and forget the saved values.
    void ResetViewportHandleSettings();

    //! Smallest area squared for a triangle to still be considered valid.
    inline const float DegenerateTriangleAreaSquareEpsilon{std::numeric_limits<float>::epsilon()};

    //! Default tint colour for WhiteBox render material.
    inline const AZ::Vector3 DefaultMaterialTint = AZ::Vector3::CreateOne();

    //! Default texture type for WhiteBox render material.
    inline const bool DefaultMaterialUseTexture = true;

    //! Default game mode visibility for WhiteBox render material.
    inline const bool DefaultVisibility = true;
} // namespace WhiteBox
