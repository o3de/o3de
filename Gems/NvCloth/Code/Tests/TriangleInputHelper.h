/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <NvCloth/Types.h>

namespace UnitTest
{
    //! Class to provide triangle input data for tests
    struct TriangleInput
    {
        AZStd::vector<NvCloth::SimParticleFormat> m_vertices;
        AZStd::vector<NvCloth::SimIndexType> m_indices;
        AZStd::vector<NvCloth::SimUVType> m_uvs;
    };

    //! Creates triangle data for a plane in XY axis with any dimensions and segments.
    TriangleInput CreatePlane(
        float width,
        float height,
        AZ::u32 segmentsX,
        AZ::u32 segmentsY);
} // namespace UnitTest
