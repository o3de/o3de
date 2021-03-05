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

#include "WhiteBox_precompiled.h"

#include "Viewport/WhiteBoxManipulatorBounds.h"
#include "WhiteBoxTestFixtures.h"

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Picking/BoundInterface.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace UnitTest
{
    TEST_F(WhiteBoxTestFixture, SelectUnitQuadFrontFaceShouldIntersect)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        auto faceHandles = Api::MeshFaceHandles(*m_whiteBox);

        WhiteBox::ManipulatorBoundPolygon polygonBounds(AzToolsFramework::Picking::RegisteredBoundId{});
        polygonBounds.m_polygonBound.m_triangles = Api::FacesPositions(*m_whiteBox, faceHandles);

        float distance = 0.0f;
        bool intersected = polygonBounds.IntersectRay(AZ::Vector3(0, -10, 0), AZ::Vector3(0, 1, 0), distance);
        ASSERT_TRUE(intersected);
    }

    TEST_F(WhiteBoxTestFixture, SelectUnitQuadBackFaceShouldNotIntersect)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        auto faceHandles = Api::MeshFaceHandles(*m_whiteBox);

        WhiteBox::ManipulatorBoundPolygon polygonBounds(AzToolsFramework::Picking::RegisteredBoundId{});
        polygonBounds.m_polygonBound.m_triangles = Api::FacesPositions(*m_whiteBox, faceHandles);

        float distance = 0.0f;
        bool intersected = polygonBounds.IntersectRay(AZ::Vector3(0, 10, 0), AZ::Vector3(0, -1, 0), distance);
        ASSERT_FALSE(intersected);
    }

    TEST_F(WhiteBoxTestFixture, SelectUnitQuadOutsideShouldNotIntersect)
    {
        namespace Api = WhiteBox::Api;

        Api::InitializeAsUnitQuad(*m_whiteBox);

        auto faceHandles = Api::MeshFaceHandles(*m_whiteBox);

        WhiteBox::ManipulatorBoundPolygon polygonBounds(AzToolsFramework::Picking::RegisteredBoundId{});
        polygonBounds.m_polygonBound.m_triangles = Api::FacesPositions(*m_whiteBox, faceHandles);

        float distance = 0.0f;
        bool intersected = polygonBounds.IntersectRay(AZ::Vector3(2, -10, 0), AZ::Vector3(0, 1, 0), distance);
        ASSERT_FALSE(intersected);
    }
} // namespace UnitTest
