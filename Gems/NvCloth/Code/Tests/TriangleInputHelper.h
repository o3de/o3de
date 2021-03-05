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
