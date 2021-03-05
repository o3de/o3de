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

namespace AZ
{
    namespace Render
    {
        enum class MathFilterKind
        : uint32_t
        {
            None = 0,
            Gaussian
        };

        struct GaussianFilterDescriptor
        {
            float m_standardDeviation = 0.f;
        };

        struct MathFilterDescriptor
        {
            MathFilterDescriptor()
                : m_kind{MathFilterKind::None}
            {}

            MathFilterKind m_kind;
            AZStd::vector<GaussianFilterDescriptor> m_gaussians;
        };
    } // namespace Render
} // namespace AZ
