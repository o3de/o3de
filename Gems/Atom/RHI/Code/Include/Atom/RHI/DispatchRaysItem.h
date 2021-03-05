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

#include <Atom/RHI.Reflect/Limits.h>
#include <AzCore/std/containers/array.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>

namespace AZ
{
    namespace RHI
    {
        class RayTracingPipelineState;
        class RayTracingShaderTable;
        class ShaderResourceGroup;
        class ImageView;
        class BufferView;

        //! Encapsulates all the necessary information for doing a ray tracing dispatch call.
        struct DispatchRaysItem
        {
            DispatchRaysItem() = default;

            /// The number of rays to cast
            uint32_t m_width = 0;
            uint32_t m_height = 0;
            uint32_t m_depth = 0;

            /// Ray tracing pipeline state
            const RayTracingPipelineState* m_rayTracingPipelineState = nullptr;

            /// Ray tracing shader table
            const RayTracingShaderTable* m_rayTracingShaderTable = nullptr;

            /// Global SRG
            const ShaderResourceGroup* m_globalSrg = nullptr;

            /// Global shader pipeline state
            const PipelineState* m_globalPipelineState = nullptr;
        };
    }
}
