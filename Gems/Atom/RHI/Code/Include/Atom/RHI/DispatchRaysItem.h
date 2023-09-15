/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/IndirectArguments.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <AzCore/std/containers/array.h>


namespace AZ::RHI
{
    class PipelineState;
    class RayTracingPipelineState;
    class RayTracingShaderTable;
    class ShaderResourceGroup;
    class ImageView;
    class BufferView;
    class DispatchRaysIndirectBuffer;

    //! Arguments used when submitting a (direct) dispatch rays call into a CommandList.
    struct DispatchRaysDirect
    {
        DispatchRaysDirect() = default;

        DispatchRaysDirect(uint32_t width, uint32_t height, uint32_t depth)
            : m_width(width)
            , m_height(height)
            , m_depth(depth)
        {
        }

        uint32_t m_width = 1;
        uint32_t m_height = 1;
        uint32_t m_depth = 1;
    };

    //! Arguments used when submitting an indirect dispatch rays call into a CommandList.
    struct DispatchRaysIndirect : public IndirectArguments
    {
        DispatchRaysIndirect() = default;

        DispatchRaysIndirect(
            uint32_t maxSequenceCount,
            const IndirectBufferView& indirectBuffer,
            uint64_t indirectBufferByteOffset,
            DispatchRaysIndirectBuffer* dispatchRaysIndirectBuffer)
            : DispatchRaysIndirect(maxSequenceCount, indirectBuffer, indirectBufferByteOffset, dispatchRaysIndirectBuffer, nullptr, 0)
        {
        }

        DispatchRaysIndirect(
            uint32_t maxSequenceCount,
            const IndirectBufferView& indirectBuffer,
            uint64_t indirectBufferByteOffset,
            DispatchRaysIndirectBuffer* dispatchRaysIndirectBuffer,
            const Buffer* countBuffer,
            uint64_t countBufferByteOffset)
            : IndirectArguments(maxSequenceCount, indirectBuffer, indirectBufferByteOffset, countBuffer, countBufferByteOffset)
            , m_dispatchRaysIndirectBuffer(dispatchRaysIndirectBuffer)
        {
        }

        DispatchRaysIndirectBuffer* m_dispatchRaysIndirectBuffer = nullptr;
    };

    enum class DispatchRaysType : uint8_t
    {
        Direct = 0, // A dispatch rays call where the arguments are pass directly to the submit function.
        Indirect // An indirect dispatch rays call that uses a buffer that contains the arguments.
    };

    //! Encapsulates the arguments that are specific to a type of dispatch.
    //! It uses a union to be able to store all possible arguments.
    struct DispatchRaysArguments
    {
        AZ_TYPE_INFO(DispatchRaysArguments, "F8BE4C19-F35D-4545-B17F-3C2B4D7EF4FF");

        DispatchRaysArguments()
            : DispatchRaysArguments(DispatchRaysDirect{})
        {
        }

        DispatchRaysArguments(const DispatchRaysDirect& direct)
            : m_type{ DispatchRaysType::Direct }
            , m_direct{ direct }
        {
        }

        DispatchRaysArguments(const DispatchRaysIndirect& indirect)
            : m_type{ DispatchRaysType::Indirect }
            , m_indirect{ indirect }
        {
        }

        DispatchRaysType m_type;
        union {
            /// Arguments for a direct dispatch.
            DispatchRaysDirect m_direct;
            /// Arguments for an indirect dispatch.
            DispatchRaysIndirect m_indirect;
        };
    };

    //! Encapsulates all the necessary information for doing a ray tracing dispatch call.
    struct DispatchRaysItem
    {
        DispatchRaysItem() = default;

        /// Arguments specific to a dispatch type.
        DispatchRaysArguments m_arguments;

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
} // namespace AZ::RHI
