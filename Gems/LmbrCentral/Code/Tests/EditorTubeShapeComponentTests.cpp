/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <Source/Shape/EditorTubeShapeComponentMode.h>

namespace AZ
{
    void PrintTo(const AZ::SplineAddress& splineAddress, std::ostream* os)
    {
        *os << "SplineAddress { segmentIndex: " << splineAddress.m_segmentIndex << ", segmentFraction: " << splineAddress.m_segmentFraction
            << " }";
    }
} // namespace AZ

namespace UnitTest
{
    class EditorTubeShapeFixture
        : public LeakDetectionFixture
        , public ::testing::WithParamInterface<bool>
    {
    };

    // test both open and closed versions of the spline
    INSTANTIATE_TEST_CASE_P(GenerateTubeManipulatorStates, EditorTubeShapeFixture, ::testing::Values(true, false));

    TEST_P(EditorTubeShapeFixture, GenerateTubeManipulatorStates_returns_no_TubeManipulatorStates_when_spline_is_empty)
    {
        using ::testing::Eq;

        // given (an empty spline)
        AZ::BezierSpline spline;
        spline.SetClosed(GetParam());

        // when (tube manipulator states are attempted to be created)
        const auto tubeManipulatorStates = LmbrCentral::GenerateTubeManipulatorStates(spline);

        // then (none are returned)
        EXPECT_THAT(tubeManipulatorStates.empty(), Eq(true));
    }

    TEST_P(EditorTubeShapeFixture, GenerateTubeManipulatorStates_returns_one_TubeManipulatorStates_when_spline_has_one_vertex)
    {
        using ::testing::Eq;

        // given (an empty spline)
        AZ::BezierSpline spline;
        spline.SetClosed(GetParam());
        spline.m_vertexContainer.AddVertex(AZ::Vector3::CreateZero());

        // when (tube manipulator states are attempted to be created)
        const auto tubeManipulatorStates = LmbrCentral::GenerateTubeManipulatorStates(spline);

        // then (one is returned)
        EXPECT_THAT(tubeManipulatorStates.size(), Eq(1));
        EXPECT_THAT(tubeManipulatorStates[0].m_splineAddress, Eq(AZ::SplineAddress(0, 0.0f)));
        EXPECT_THAT(tubeManipulatorStates[0].m_vertIndex, Eq(0));
    }

    TEST_P(EditorTubeShapeFixture, GenerateTubeManipulatorStates_returns_two_TubeManipulatorStates_when_spline_has_two_vertices)
    {
        using ::testing::Eq;

        // given (an empty spline)
        AZ::BezierSpline spline;
        spline.SetClosed(GetParam());

        spline.m_vertexContainer.AddVertex(AZ::Vector3::CreateZero());
        spline.m_vertexContainer.AddVertex(AZ::Vector3::CreateAxisX(1.0f));

        // when (tube manipulator states are attempted to be created)
        const auto tubeManipulatorStates = LmbrCentral::GenerateTubeManipulatorStates(spline);

        // then (two are returned)
        EXPECT_THAT(tubeManipulatorStates.size(), Eq(2));

        EXPECT_THAT(tubeManipulatorStates[0].m_splineAddress, Eq(AZ::SplineAddress(0, 0.0f)));
        EXPECT_THAT(tubeManipulatorStates[0].m_vertIndex, Eq(0));

        EXPECT_THAT(tubeManipulatorStates[1].m_splineAddress, Eq(AZ::SplineAddress(0, 1.0f)));
        EXPECT_THAT(tubeManipulatorStates[1].m_vertIndex, Eq(1));
    }

    TEST_P(EditorTubeShapeFixture, GenerateTubeManipulatorStates_returns_three_TubeManipulatorStates_when_spline_has_three_vertices)
    {
        using ::testing::Eq;

        // given (an empty spline)
        AZ::BezierSpline spline;
        spline.SetClosed(GetParam());

        spline.m_vertexContainer.AddVertex(AZ::Vector3::CreateAxisX(-1.0f));
        spline.m_vertexContainer.AddVertex(AZ::Vector3::CreateZero());
        spline.m_vertexContainer.AddVertex(AZ::Vector3::CreateAxisX(1.0f));

        // when (tube manipulator states are attempted to be created)
        const auto tubeManipulatorStates = LmbrCentral::GenerateTubeManipulatorStates(spline);

        // then (three are returned)
        EXPECT_THAT(tubeManipulatorStates.size(), Eq(3));

        EXPECT_THAT(tubeManipulatorStates[0].m_splineAddress, Eq(AZ::SplineAddress(0, 0.0f)));
        EXPECT_THAT(tubeManipulatorStates[0].m_vertIndex, Eq(0));

        EXPECT_THAT(tubeManipulatorStates[1].m_splineAddress, Eq(AZ::SplineAddress(1, 0.0f)));
        EXPECT_THAT(tubeManipulatorStates[1].m_vertIndex, Eq(1));

        EXPECT_THAT(tubeManipulatorStates[2].m_splineAddress, Eq(AZ::SplineAddress(1, 1.0f)));
        EXPECT_THAT(tubeManipulatorStates[2].m_vertIndex, Eq(2));
    }
} // namespace UnitTest
