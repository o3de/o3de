/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Shader/Metrics/ShaderMetrics.h>

namespace AZ
{
    namespace Render
    {
        //! Visual profiler for Shader Variants.
        class ImGuiShaderMetrics
        {
        public:
            ImGuiShaderMetrics() = default;
            ~ImGuiShaderMetrics() = default;

            //! Draws the stats for the specified metrics.
            void Draw(bool& draw, const AZ::RPI::ShaderVariantMetrics& metrics);
        };
    } // namespace Render
}  // namespace AZ

#include "ImGuiShaderMetrics.inl"
