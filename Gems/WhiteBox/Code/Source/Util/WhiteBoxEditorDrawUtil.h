/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/span.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace AzFramework
{
    class DebugDisplayRequests;
    struct ViewportInfo;
} // namespace AzFramework

namespace AZ
{
    class Color;
    class Transform;
} // namespace AZ

namespace WhiteBox
{
    struct WhiteBoxMesh;

    void DrawFace(
        AzFramework::DebugDisplayRequests& debugDisplay,
        WhiteBoxMesh* mesh,
        const Api::PolygonHandle& polygon,
        const AZ::Color& color);
    void DrawOutline(
        AzFramework::DebugDisplayRequests& debugDisplay,
        WhiteBoxMesh* mesh,
        const Api::PolygonHandle& polygon,
        const AZ::Color& color);
    void DrawEdge(
        AzFramework::DebugDisplayRequests& debugDisplay,
        WhiteBoxMesh* mesh,
        const Api::EdgeHandle& edge,
        const AZ::Color& color);
        
    void DrawPoints(
        AzFramework::DebugDisplayRequests& debugDisplay,
        WhiteBoxMesh* mesh,
        const AZ::Transform& worldFromLocal,
        const AzFramework::ViewportInfo& viewportInfo,
        const AZStd::span<Api::VertexHandle>& verts,
        const AZ::Color& color);
} // namespace WhiteBox
