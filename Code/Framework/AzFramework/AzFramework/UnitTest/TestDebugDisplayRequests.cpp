/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TestDebugDisplayRequests.h"

namespace UnitTest
{
    TestDebugDisplayRequests::TestDebugDisplayRequests()
    {
        m_transforms.push(AZ::Transform::CreateIdentity());
    }

    const AZStd::vector<AZ::Vector3>& TestDebugDisplayRequests::GetPoints() const
    {
        return m_points;
    }

    void TestDebugDisplayRequests::ClearPoints()
    {
        m_points.clear();
    }

    AZ::Aabb TestDebugDisplayRequests::GetAabb() const
    {
        return m_points.size() > 0 ? AZ::Aabb::CreatePoints(m_points.data(), m_points.size()) : AZ::Aabb::CreateNull();
    }

    void TestDebugDisplayRequests::DrawWireBox(const AZ::Vector3& min, const AZ::Vector3& max)
    {
        const AZ::Transform& tm = m_transforms.back();
        m_points.push_back(tm.TransformPoint(AZ::Vector3(min.GetX(), min.GetY(), min.GetZ())));
        m_points.push_back(tm.TransformPoint(AZ::Vector3(min.GetX(), min.GetY(), max.GetZ())));
        m_points.push_back(tm.TransformPoint(AZ::Vector3(min.GetX(), max.GetY(), min.GetZ())));
        m_points.push_back(tm.TransformPoint(AZ::Vector3(min.GetX(), max.GetY(), max.GetZ())));
        m_points.push_back(tm.TransformPoint(AZ::Vector3(max.GetX(), min.GetY(), min.GetZ())));
        m_points.push_back(tm.TransformPoint(AZ::Vector3(max.GetX(), min.GetY(), max.GetZ())));
        m_points.push_back(tm.TransformPoint(AZ::Vector3(max.GetX(), max.GetY(), min.GetZ())));
        m_points.push_back(tm.TransformPoint(AZ::Vector3(max.GetX(), max.GetY(), max.GetZ())));
    }

    void TestDebugDisplayRequests::DrawSolidBox(const AZ::Vector3& min, const AZ::Vector3& max)
    {
        DrawWireBox(min, max);
    }

    void TestDebugDisplayRequests::DrawWireQuad(float width, float height)
    {
        const AZ::Transform& tm = m_transforms.back();
        m_points.push_back(tm.TransformPoint(AZ::Vector3(-0.5f * width, 0.0f, -0.5f * height)));
        m_points.push_back(tm.TransformPoint(AZ::Vector3(-0.5f * width, 0.0f, 0.5f * height)));
        m_points.push_back(tm.TransformPoint(AZ::Vector3(0.5f * width, 0.0f, -0.5f * height)));
        m_points.push_back(tm.TransformPoint(AZ::Vector3(0.5f * width, 0.0f, 0.5f * height)));
    }

    void TestDebugDisplayRequests::DrawQuad(float width, float height)
    {
        DrawWireQuad(width, height);
    }

    void TestDebugDisplayRequests::DrawPoints(const AZStd::vector<AZ::Vector3>& points)
    {
        const AZ::Transform& tm = m_transforms.back();
        for (const auto& point : points)
        {
            m_points.push_back(tm.TransformPoint(point));
        }
    }

    void TestDebugDisplayRequests::DrawTriangles(const AZStd::vector<AZ::Vector3>& vertices, [[maybe_unused]] const AZ::Color& color)
    {
        DrawPoints(vertices);
    }

    void TestDebugDisplayRequests::DrawTrianglesIndexed(const AZStd::vector<AZ::Vector3>& vertices,
        [[maybe_unused]] const AZStd::vector<AZ::u32>& indices, [[maybe_unused]] const AZ::Color& color)
    {
        DrawPoints(vertices);
    }

    void TestDebugDisplayRequests::DrawLine(const AZ::Vector3& p1, const AZ::Vector3& p2)
    {
        DrawPoints({ p1, p2 });
    }

    void TestDebugDisplayRequests::DrawLine(const AZ::Vector3& p1, const AZ::Vector3& p2,
        [[maybe_unused]] const AZ::Vector4& col1, [[maybe_unused]] const AZ::Vector4& col2)
    {
        DrawPoints({ p1, p2 });
    }

    void TestDebugDisplayRequests::DrawLines(const AZStd::vector<AZ::Vector3>& lines, [[maybe_unused]] const AZ::Color& color)
    {
        DrawPoints(lines);
    }

    void TestDebugDisplayRequests::PushMatrix(const AZ::Transform& tm)
    {
        m_transforms.push(m_transforms.back() * tm);
    }

    void TestDebugDisplayRequests::PopMatrix()
    {
        if (m_transforms.size() == 1)
        {
            AZ_Error("TestDebugDisplayRequest", false, "Invalid call to PopMatrix when no matrices were pushed.");
        }
        else
        {
            m_transforms.pop();
        }
    }
} // namespace UnitTest
