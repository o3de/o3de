/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxViewportConstants.h"

namespace WhiteBox
{

    AZ_CVAR(float, cl_whiteBoxVertexManipulatorSize, 0.125f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, cl_whiteBoxMouseClickDeltaThreshold, 0.001f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, cl_whiteBoxModifierMidpointEpsilon, 0.01f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, cl_whiteBoxEdgeVisualWidth, 5.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, cl_whiteBoxEdgeSelectionWidth, 0.075f, nullptr, AZ::ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, cl_whiteBoxSelectedEdgeVisualWidth, 6.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "");

    AZ_CVAR(float, ed_whiteBoxPolygonViewOverlapOffset, 0.004f, nullptr, AZ::ConsoleFunctorFlags::Null, "The offset highlighted polygon");

    AZ_CVAR(AZ::Color, ed_whiteBoxVertexHiddenColor, AZ::Color::CreateFromRgba(200, 200, 200, 255), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the polygon when hovered over");
    AZ_CVAR(AZ::Color, ed_whiteBoxVertexRestoredColor,AZ::Color::CreateFromRgba(50, 50, 50, 150), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the outline when hovered over");   

    AZ_CVAR(AZ::Color, ed_whiteBoxVertexUnselected, AZ::Color::CreateFromRgba(0, 100, 255, 150), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the verticies when selected");
    AZ_CVAR(AZ::Color, ed_whiteBoxEdgeUnselected, AZ::Color::CreateFromRgba(127, 127, 127, 76), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the verticies when selected");
    AZ_CVAR(AZ::Color, ed_whiteBoxEdgeDefault, AZ::Color::CreateFromRgba(0, 0, 0, 76), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the verticies when selected");
    
    AZ_CVAR(AZ::Color, ed_whiteBoxVertexHover, AZ::Color::CreateFromRgba(0, 150, 255, 200), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the verticies when selected");
    AZ_CVAR(AZ::Color, ed_whiteBoxPolygonHover, AZ::Color::CreateFromRgba(255, 175, 0, 127), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the polygon when hovered over");
    AZ_CVAR(AZ::Color, ed_whiteBoxOutlineHover, AZ::Color::CreateFromRgba(255, 100, 0, 255), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the outline when hovered over");
   
    AZ_CVAR(AZ::Color, ed_whiteBoxPolygonSelection, AZ::Color::CreateFromRgba(0, 150, 255, 127), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the polygon when selected");
    AZ_CVAR(AZ::Color, ed_whiteBoxVertexSelection, AZ::Color::CreateFromRgba(0, 150, 255, 200), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the verticies when selected");
    AZ_CVAR(AZ::Color, ed_whiteBoxOutlineSelection, AZ::Color::CreateFromRgba(0, 150, 255, 255), nullptr, AZ::ConsoleFunctorFlags::Null, "Color of the outline when selected");
} // namespace WhiteBox
