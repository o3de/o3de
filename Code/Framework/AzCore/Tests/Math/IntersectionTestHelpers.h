/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>

namespace UnitTest::IntersectTest
{
    // Implementation of the Moller Trumbore ray/triangle intersection algorithm.
    // ( https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm )
    // This is used for cross-verifying that our current ray/triangle intersection logic produces consistent results with other
    // ray/triangle intersection algorithms and to provide comparative timing benchmarks between different algorithms.

    // One-sided version of the algorithm, using counter-clockwise triangle winding.
    bool MollerTrumboreIntersectSegmentTriangleCCW(
        const AZ::Vector3& p, const AZ::Vector3& q,
        const AZ::Vector3& a, const AZ::Vector3& b, const AZ::Vector3& c,
        AZ::Vector3& triNormal, float& t);

    // Two-sided version of the algorithm
    bool MollerTrumboreIntersectSegmentTriangle(
        const AZ::Vector3& p, const AZ::Vector3& q,
        const AZ::Vector3& a, const AZ::Vector3& b, const AZ::Vector3& c,
        AZ::Vector3& triNormal, float& t);

    // Implementation of the Arenberg ray/triangle intersection algorithm.
    // ( http://graphics.stanford.edu/pub/Graphics/RTNews/html/rtnews5b.html )
    // This is used for cross-verifying that our current ray/triangle intersection logic produces consistent results with other
    // ray/triangle intersection algorithms and to provide comparative timing benchmarks between different algorithms.

    // One-sided version of the algorithm, using counter-clockwise triangle winding.
    bool ArenbergIntersectSegmentTriangleCCW(
        const AZ::Vector3& p, const AZ::Vector3& q,
        const AZ::Vector3& a, const AZ::Vector3& b, const AZ::Vector3& c,
        AZ::Vector3& triNormal, float& t);

    // Two-sided version of the algorithm
    bool ArenbergIntersectSegmentTriangle(
        const AZ::Vector3& p, const AZ::Vector3& q,
        const AZ::Vector3& a, const AZ::Vector3& b, const AZ::Vector3& c,
        AZ::Vector3& triNormal, float& t);

} // namespace UnitTest
