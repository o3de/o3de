/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/DeviceDispatchRaysItem.h>
#include <Atom/RHI/IndirectArguments.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <Atom/RHI/RayTracingPipelineState.h>
#include <Atom/RHI/RayTracingShaderTable.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <AzCore/std/containers/array.h>

namespace AZ
{
    namespace RHI
    {
        class ShaderResourceGroup;
        class ImageView;
        class BufferView;

        //! Arguments used when submitting an indirect dispatch rays call into a CommandList.
        //! The indirect dispatch arguments are the same ones as the indirect draw ones.
        using DispatchRaysIndirect = IndirectArguments;

        //! Encapsulates the arguments that are specific to a type of dispatch.
        //! It uses a union to be able to store all possible arguments.
        struct DispatchRaysArguments
        {
            AZ_TYPE_INFO(DispatchRaysArguments, "7E2A014B-53C5-4E2D-8CEB-4D7CC42BE196");

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

            DeviceDispatchRaysArguments GetDeviceDispatchRaysArguments(int deviceIndex) const
            {
                switch (m_type)
                {
                case DispatchRaysType::Direct:
                    return DeviceDispatchRaysArguments(m_direct);
                case DispatchRaysType::Indirect:
                    return DeviceDispatchRaysArguments(m_indirect.GetDeviceIndirectArguments(deviceIndex));
                default:
                    return DeviceDispatchRaysArguments();
                }
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

            DeviceDispatchRaysItem GetDeviceDispatchRaysItem(int deviceIndex, const DeviceShaderResourceGroup** shaderResourceGroups) const
            {
                DeviceDispatchRaysItem result;

                result.m_arguments = m_arguments.GetDeviceDispatchRaysArguments(deviceIndex);
                result.m_rayTracingPipelineState = m_rayTracingPipelineState->GetDevicePipelineState(deviceIndex).get();
                result.m_rayTracingShaderTable = m_rayTracingShaderTable->GetDeviceRayTracingShaderTable(deviceIndex).get();
                result.m_shaderResourceGroupCount = m_shaderResourceGroupCount;

                if (m_shaderResourceGroups)
                {
                    for (auto i = 0u; i < m_shaderResourceGroupCount; ++i)
                    {
                        shaderResourceGroups[i] = m_shaderResourceGroups[i]->GetDeviceShaderResourceGroup(deviceIndex).get();
                    }

                    result.m_shaderResourceGroups = shaderResourceGroups;
                }

                if (m_globalPipelineState)
                {
                    result.m_globalPipelineState = m_globalPipelineState->GetDevicePipelineState(deviceIndex).get();
                }

                return result;
            }

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
    } // namespace RHI
} // namespace AZ
