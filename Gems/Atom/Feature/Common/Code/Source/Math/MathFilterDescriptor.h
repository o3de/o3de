/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
