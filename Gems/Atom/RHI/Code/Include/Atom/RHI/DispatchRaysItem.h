/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

            /// Shader Resource Groups
            uint32_t m_shaderResourceGroupCount = 0;
            const ShaderResourceGroup* const* m_shaderResourceGroups = nullptr;

            /// Global shader pipeline state
            const PipelineState* m_globalPipelineState = nullptr;
        };
    }
}
