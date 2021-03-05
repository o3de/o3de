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
