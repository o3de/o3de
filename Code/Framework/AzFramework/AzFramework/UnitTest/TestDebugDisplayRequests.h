/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/stack.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace UnitTest
{
    //! Null implementation of DebugDisplayRequests for dummy draw calls.
    class NullDebugDisplayRequests : public AzFramework::DebugDisplayRequests
    {
    public:
        virtual ~NullDebugDisplayRequests() = default;
    };

    //! Minimal implementation of DebugDisplayRequests to support testing shapes.
    //! Stores a list of points based on received draw calls to delineate the exterior of the object requested to be drawn.
    class TestDebugDisplayRequests : public AzFramework::DebugDisplayRequests
    {
    public:
        TestDebugDisplayRequests();
        ~TestDebugDisplayRequests() override = default;

        const AZStd::vector<AZ::Vector3>& GetPoints() const;
        void ClearPoints();
        //! Returns the AABB of the points generated from received draw calls.
        AZ::Aabb GetAabb() const;

        // DebugDisplayRequests ...
        void DrawWireBox(const AZ::Vector3& min, const AZ::Vector3& max) override;
        void DrawSolidBox(const AZ::Vector3& min, const AZ::Vector3& max) override;
        using AzFramework::DebugDisplayRequests::DrawWireQuad;
        void DrawWireQuad(float width, float height) override;
        using AzFramework::DebugDisplayRequests::DrawQuad;
        void DrawQuad(float width, float height, bool drawShaded) override;
        void DrawTriangles(const AZStd::vector<AZ::Vector3>& vertices, const AZ::Color& color) override;
        void DrawTrianglesIndexed(const AZStd::vector<AZ::Vector3>& vertices, const AZStd::vector<AZ::u32>& indices, const AZ::Color& color) override;
        void DrawLine(const AZ::Vector3& p1, const AZ::Vector3& p2) override;
        void DrawLine(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector4& col1, const AZ::Vector4& col2) override;
        void DrawLines(const AZStd::vector<AZ::Vector3>& lines, const AZ::Color& color) override;
        void DrawWireSphere(const AZ::Vector3& pos, float radius) override;
        void DrawWireSphere([[maybe_unused]] const AZ::Vector3& pos, [[maybe_unused]] const AZ::Vector3 radius) override {}
        void PushMatrix(const AZ::Transform& tm) override;
        void PopMatrix() override;
        void PushPremultipliedMatrix(const AZ::Matrix3x4& matrix) override;
        AZ::Matrix3x4 PopPremultipliedMatrix() override;
    private:
        void DrawPoints(const AZStd::vector<AZ::Vector3>& points);
        AZStd::vector<AZ::Vector3> m_points;
        AZStd::stack<AZ::Transform> m_transforms;
    };
} // namespace UnitTest
