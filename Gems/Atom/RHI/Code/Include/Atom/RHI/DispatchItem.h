/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/DeviceDispatchItem.h>
#include <Atom/RHI/DeviceShaderResourceGroup.h>
#include <Atom/RHI/IndirectArguments.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/containers/array.h>

namespace AZ
{
    namespace RHI
    {
        class PipelineState;
        class ShaderResourceGroup;

        //! Arguments used when submitting an indirect dispatch call into a CommandList.
        //! The indirect dispatch arguments are the same ones as the indirect draw ones.
        using DispatchIndirect = IndirectArguments;

        //! Encapsulates the arguments that are specific to a type of dispatch.
        //! It uses a union to be able to store all possible arguments.
        struct DispatchArguments
        {
            AZ_TYPE_INFO(DispatchArguments, "675E50A1-B3E9-44D6-98C8-20A170A62BBA");

            DispatchArguments()
                : DispatchArguments(DispatchDirect{})
            {
            }

            DispatchArguments(const DispatchDirect& direct)
                : m_type{ DispatchType::Direct }
                , m_direct{ direct }
            {}

            DispatchArguments(const DispatchIndirect& indirect)
                : m_type{ DispatchType::Indirect }
                , m_indirect{ indirect }
            {
            }

            DeviceDispatchArguments GetDeviceDispatchArguments(int deviceIndex) const
            {
                switch (m_type)
                {
                case DispatchType::Direct:
                    return DeviceDispatchArguments(m_direct);
                case DispatchType::Indirect:
                    return DeviceDispatchArguments(m_indirect.GetDeviceIndirectArguments(deviceIndex));
                default:
                    return DeviceDispatchArguments();
                }
            }

            DispatchType m_type;
            union
            {
                /// Arguments for a direct dispatch.
                DispatchDirect m_direct;
                /// Arguments for an indirect dispatch.
                DispatchIndirect m_indirect;
            };
        };

        //! Encapsulates all the necessary information for doing a dispatch call.
        //! This includes all common arguments for the different dispatch type, plus
        //! arguments that are specific to a type.
        struct DispatchItem
        {
            DispatchItem() = default;

            DeviceDispatchItem GetDeviceDispatchItem(int deviceIndex) const
            {
                DeviceDispatchItem result;

                result.m_arguments = m_arguments.GetDeviceDispatchArguments(deviceIndex);
                result.m_shaderResourceGroupCount = m_shaderResourceGroupCount;
                result.m_rootConstantSize = m_rootConstantSize;

                if (m_pipelineState)
                {
                    result.m_pipelineState = m_pipelineState->GetDevicePipelineState(deviceIndex).get();
                }

                for (int i = 0; i < m_shaderResourceGroupCount; ++i)
                {
                    result.m_shaderResourceGroups[i] = m_shaderResourceGroups[i]->GetDeviceShaderResourceGroup(deviceIndex).get();
                }

                if (m_uniqueShaderResourceGroup)
                {
                    result.m_uniqueShaderResourceGroup = m_uniqueShaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get();
                }

                result.m_rootConstants = m_rootConstants;

                return result;
            }

            /// Arguments specific to a dispatch type.
            DispatchArguments m_arguments;

            /// The number of shader resource groups and inline constants in each array.
            uint8_t m_shaderResourceGroupCount = 0;
            uint8_t m_rootConstantSize = 0;

            /// The pipeline state to bind.
            const PipelineState* m_pipelineState = nullptr;

            /// Array of shader resource groups to bind (count must match m_shaderResourceGroupCount).
            AZStd::array<const ShaderResourceGroup*, Limits::Pipeline::ShaderResourceGroupCountMax> m_shaderResourceGroups = {};

            /// Unique SRG, not shared within the draw packet. This is usually a per-draw SRG, populated with the shader variant fallback key
            const ShaderResourceGroup* m_uniqueShaderResourceGroup = nullptr;

            /// Inline constants data.
            const uint8_t* m_rootConstants = nullptr;
        };
    }
}
