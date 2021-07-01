/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Manipulators/ManipulatorView.h>

#include <WhiteBox/WhiteBoxToolApi.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace WhiteBox
{
    //! Displays a polygon with an outline around the edge.
    class ManipulatorViewPolygon : public AzToolsFramework::ManipulatorView
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ_RTTI(ManipulatorViewPolygon, "{B2290233-1D42-4AF5-8949-7CF9601832E2}", AzToolsFramework::ManipulatorView)

        ManipulatorViewPolygon();

        void Draw(
            AzToolsFramework::ManipulatorManagerId managerId,
            const AzToolsFramework::ManipulatorManagerState& managerState,
            AzToolsFramework::ManipulatorId manipulatorId, const AzToolsFramework::ManipulatorState& manipulatorState,
            AzFramework::DebugDisplayRequests& debugDisplay, const AzFramework::CameraState& cameraState,
            const AzToolsFramework::ViewportInteraction::MouseInteraction& mouseInteraction) override;

        AZStd::vector<AZ::Vector3> m_triangles;
        Api::VertexPositionsCollection m_outlines;

        AZ::Color m_outlineColor = AZ::Color(1.0f, 1.0f, 0.0f, 1.0f); //!< Yellow
        AZ::Color m_fillColor = AZ::Color(1.0f, 1.0f, 0.0f, 0.5f); //!< Yellow
    };

    class ManipulatorViewEdge : public AzToolsFramework::ManipulatorView
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ_RTTI(ManipulatorViewEdge, "{42F07925-5B2F-4CC6-9033-CF2FE548BF8A}", AzToolsFramework::ManipulatorView)

        ManipulatorViewEdge();

        void Draw(
            AzToolsFramework::ManipulatorManagerId managerId,
            const AzToolsFramework::ManipulatorManagerState& managerState,
            AzToolsFramework::ManipulatorId manipulatorId, const AzToolsFramework::ManipulatorState& manipulatorState,
            AzFramework::DebugDisplayRequests& debugDisplay, const AzFramework::CameraState& cameraState,
            const AzToolsFramework::ViewportInteraction::MouseInteraction& mouseInteraction) override;

        void SetColor(const AZ::Color& color, const AZ::Color& hoverColor);
        void SetWidth(float width, float hoverWidth);

        AZ::Vector3 m_start;
        AZ::Vector3 m_end;
        AZStd::array<float, 2> m_width;
        AZStd::array<AZ::Color, 2> m_color;
        AZStd::array<bool, 2> m_vertexStartEndHidden; //!< When this manipulator view was created, were the adjoining
                                                      //!< vertex handles hidden or not.
    };

    void TranslatePoints(AZStd::vector<AZ::Vector3>& points, const AZ::Vector3& offset);

    AZStd::unique_ptr<ManipulatorViewPolygon> CreateManipulatorViewPolygon(
        const AZStd::vector<AZ::Vector3>& triangles, const Api::VertexPositionsCollection& outlines);

    AZStd::unique_ptr<ManipulatorViewEdge> CreateManipulatorViewEdge(const AZ::Vector3& start, const AZ::Vector3& end);

} // namespace WhiteBox
