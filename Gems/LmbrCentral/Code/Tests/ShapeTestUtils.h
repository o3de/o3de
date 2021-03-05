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

#pragma once
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace UnitTest
{
    //! Minimal implementation of DebugDisplayRequests to support testing shapes.
    //! Stores a list of points based on received draw calls to delineate the exterior of the object requested to be drawn.
    class TestDebugDisplayRequests : public AzFramework::DebugDisplayRequests
    {
    public:
        TestDebugDisplayRequests();
        const AZStd::vector<AZ::Vector3>& GetPoints() const;

        // DebugDisplayRequests ...
        void DrawWireBox(const AZ::Vector3& min, const AZ::Vector3& max) override;
        void DrawSolidBox(const AZ::Vector3& min, const AZ::Vector3& max) override;
        void DrawWireQuad(float width, float height) override;
        void DrawQuad(float width, float height) override;
        void PushMatrix(const AZ::Transform& tm) override;
        void PopMatrix() override;
    private:
        AZStd::vector<AZ::Vector3> m_points;
        AZStd::stack<AZ::Transform> m_transforms;
    };
} // namespace UnitTest
