/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <ShapeTestUtils.h>

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
