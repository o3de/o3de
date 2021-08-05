/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
