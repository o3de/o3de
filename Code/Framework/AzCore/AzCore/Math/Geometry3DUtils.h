/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace Geometry3dUtils
    {

        //! Generates a set of vertices for a regular unit-size triangulated icosphere.
        //! The returned vertices form a triangle mesh that forms an approximation of a sphere.
        //! @param subdivisionDepth The recursion depth for subdividing the icosphere further. A depth of 0 returns an icosahedron.
        //! @return The set of vertices for the unit-sized triangluated icosphere.
        //! SubdivisionDepth is currently capped at 9 just for sanity purposes. Here are the triangle counts at each depth:
        //!   0 -> 20           5 -> 20480
        //!   1 -> 80           6 -> 81920
        //!   2 -> 320          7 -> 327680
        //!   3 -> 1280         8 -> 1310720
        //!   4 -> 5120         9 -> 5242880
        //! Generally a depth of 3-6 will be sufficient for most purposes.
        AZStd::vector<AZ::Vector3> GenerateIcoSphere(const uint8_t subdivisionDepth);

    } // namespace Geometry3dUtils
} // namespace AZ
