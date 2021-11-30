/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxTestUtil.h"

namespace WhiteBox
{
    std::ostream& operator<<(std::ostream& os, const Api::FaceHandle faceHandle)
    {
        return os << Api::ToString(faceHandle).c_str();
    }

    std::ostream& operator<<(std::ostream& os, const Api::VertexHandle vertexHandle)
    {
        return os << Api::ToString(vertexHandle).c_str();
    }

    std::ostream& operator<<(std::ostream& os, const Api::PolygonHandle& polygonHandle)
    {
        return os << Api::ToString(polygonHandle).c_str();
    }

    std::ostream& operator<<(std::ostream& os, const Api::EdgeHandle edgeHandle)
    {
        return os << Api::ToString(edgeHandle).c_str();
    }

    std::ostream& operator<<(std::ostream& os, const Api::HalfedgeHandle halfedgeHandle)
    {
        return os << Api::ToString(halfedgeHandle).c_str();
    }

    std::ostream& operator<<(std::ostream& os, const Api::FaceVertHandles& faceVertHandles)
    {
        return os << Api::ToString(faceVertHandles).c_str();
    }

    std::ostream& operator<<(std::ostream& os, const Api::FaceVertHandlesList& faceVertHandlesList)
    {
        return os << Api::ToString(faceVertHandlesList).c_str();
    }

    AZStd::string VerticesToString(const WhiteBoxMesh* whiteBox, const AZ::Transform& localToWorld)
    {
        const auto vertexPositions = Api::MeshVertexPositions(*whiteBox);

        AZStd::string vertexString = "\n----------------------------------------------\n";
        for (size_t i = 0; i < vertexPositions.size(); i++)
        {
            const auto& p = vertexPositions[i];
            const auto v = localToWorld.TransformPoint(p);
            vertexString += AZStd::string::format("Vertex %zu: %f, %f, %f\n", i, v.GetX(), v.GetY(), v.GetZ());
        }
        vertexString += "----------------------------------------------\n";

        return vertexString;
    }

    AZStd::string FacesMidPointsToString(const WhiteBoxMesh* whiteBox, const AZ::Transform& localToWorld)
    {
        std::stringstream ss;
        const auto faceHandles = Api::MeshFaceHandles(*whiteBox);

        AZStd::string vertexString = "\n----------------------------------------------\n";
        for (size_t i = 0; i < faceHandles.size(); i++)
        {
            const auto p = Api::PolygonMidpoint(
                *whiteBox, Api::FacePolygonHandle(*whiteBox, Api::FaceHandle(aznumeric_cast<int>(i))));
            const auto v = localToWorld.TransformPoint(p);
            vertexString += AZStd::string::format("Face midpoint %zu: %f, %f, %f\n", i, v.GetX(), v.GetY(), v.GetZ());
        }
        vertexString += "----------------------------------------------\n";

        return vertexString;
    }
} // namespace WhiteBox

namespace UnitTest
{
    void Create2x2CubeGrid(WhiteBox::WhiteBoxMesh& whiteBox)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitCube(whiteBox);

        Initialize2x2CubeGrid(whiteBox);
    }

    void Initialize2x2CubeGrid(WhiteBox::WhiteBoxMesh& whiteBox)
    {
        namespace Api = WhiteBox::Api;

        // form a 2x2 grid of connected cubes
        Api::TranslatePolygonAppend(whiteBox, Api::FacePolygonHandle(whiteBox, Api::FaceHandle{4}), 1.0f);
        Api::HideEdge(whiteBox, Api::EdgeHandle{12});
        Api::TranslatePolygonAppend(whiteBox, Api::FacePolygonHandle(whiteBox, Api::FaceHandle{5}), 1.0f);
    }

    void Create3x3CubeGrid(WhiteBox::WhiteBoxMesh& whiteBox)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitCube(whiteBox);

        Initialize3x3CubeGrid(whiteBox);
    }

    void Initialize3x3CubeGrid(WhiteBox::WhiteBoxMesh& whiteBox)
    {
        namespace Api = WhiteBox::Api;

        // form a 3x3 grid of connected cubes
        Api::TranslatePolygonAppend(whiteBox, Api::FacePolygonHandle(whiteBox, Api::FaceHandle{4}), 1.0f);
        Api::TranslatePolygonAppend(whiteBox, Api::FacePolygonHandle(whiteBox, Api::FaceHandle{11}), 1.0f);
        Api::HideEdge(whiteBox, Api::EdgeHandle{21});
        Api::HideEdge(whiteBox, Api::EdgeHandle{12});
        Api::TranslatePolygonAppend(whiteBox, Api::FacePolygonHandle(whiteBox, Api::FaceHandle{27}), 1.0f);
        Api::TranslatePolygonAppend(whiteBox, Api::FacePolygonHandle(whiteBox, Api::FaceHandle{26}), 1.0f);
    }

    void HideAllTopUserEdgesFor2x2Grid(WhiteBox::WhiteBoxMesh& whiteBox)
    {
        namespace Api = WhiteBox::Api;

        for (const auto edgeHandle : {43, 12, 4})
        {
            Api::HideEdge(whiteBox, Api::EdgeHandle{edgeHandle});
        }
    }

    void HideAllTopUserEdgesFor3x3Grid(WhiteBox::WhiteBoxMesh& whiteBox)
    {
        namespace Api = WhiteBox::Api;

        // hide all top 'user' edges (top is one polygon)
        for (const auto edgeHandle : {41, 12, 59, 47, 4, 48, 27, 45})
        {
            Api::HideEdge(whiteBox, Api::EdgeHandle{edgeHandle});
        }
    }

    MultiSpacePoint::MultiSpacePoint(
        const AZ::Vector3& local, const AZ::Transform& toWorld, const AzFramework::CameraState& cameraState)
    {
        m_local = local;
        m_world = toWorld.TransformPoint(m_local);
        m_screen = AzFramework::WorldToScreen(m_world, cameraState);
    }

    const AZ::Vector3& MultiSpacePoint::GetLocalSpace() const
    {
        return m_local;
    }

    const AZ::Vector3& MultiSpacePoint::GetWorldSpace() const
    {
        return m_world;
    }

    const AzFramework::ScreenPoint& MultiSpacePoint::GetScreenSpace() const
    {
        return m_screen;
    }
} // namespace UnitTest
