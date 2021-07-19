/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Transform.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzTest/AzTest.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    std::ostream& operator<<(std::ostream& os, Api::FaceHandle faceHandle);
    std::ostream& operator<<(std::ostream& os, Api::VertexHandle vertexHandle);
    std::ostream& operator<<(std::ostream& os, Api::PolygonHandle& polygonHandle);
    std::ostream& operator<<(std::ostream& os, Api::EdgeHandle edgeHandle);
    std::ostream& operator<<(std::ostream& os, Api::HalfedgeHandle halfedgeHandle);
    std::ostream& operator<<(std::ostream& os, Api::FaceVertHandles& faceVertHandles);
    std::ostream& operator<<(std::ostream& os, Api::FaceVertHandlesList& faceVertHandlesList);

    // debug utility to write the white box mesh vertex data to a string
    AZStd::string VerticesToString(
        const WhiteBox::WhiteBoxMesh* whiteBox, const AZ::Transform& localToWorld = AZ::Transform::CreateIdentity());

    // debug utility to write the white box mesh face mid point data to a string
    AZStd::string FacesMidPointsToString(
        const WhiteBox::WhiteBoxMesh* whiteBox, const AZ::Transform& localToWorld = AZ::Transform::CreateIdentity());
} // namespace WhiteBox

namespace UnitTest
{
    void Create2x2CubeGrid(WhiteBox::WhiteBoxMesh& whiteBox);
    void Initialize2x2CubeGrid(WhiteBox::WhiteBoxMesh& whiteBox);
    void Create3x3CubeGrid(WhiteBox::WhiteBoxMesh& whiteBox);
    void Initialize3x3CubeGrid(WhiteBox::WhiteBoxMesh& whiteBox);
    void HideAllTopUserEdgesFor2x2Grid(WhiteBox::WhiteBoxMesh& whiteBox);
    void HideAllTopUserEdgesFor3x3Grid(WhiteBox::WhiteBoxMesh& whiteBox);

    // convenience type to hold the local, world and screen space positions of a point
    class MultiSpacePoint
    {
    public:
        MultiSpacePoint(
            const AZ::Vector3& local, const AZ::Transform& toWorld, const AzFramework::CameraState& cameraState);
        const AZ::Vector3& GetLocalSpace() const;
        const AZ::Vector3& GetWorldSpace() const;
        const AzFramework::ScreenPoint& GetScreenSpace() const;

    private:
        AZ::Vector3 m_local;
        AZ::Vector3 m_world;
        AzFramework::ScreenPoint m_screen;
    };
} // namespace UnitTest
