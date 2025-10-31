/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/DeviceIndirectArguments.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/Casting/numeric_cast.h>

namespace AZ::RHI
{
    class DevicePipelineState;
    class DeviceShaderResourceGroup;

    //! Arguments used when submitting a (direct) dispatch call into a CommandList.
    struct DispatchDirect
    {
        DispatchDirect() = default;

        DispatchDirect(
            uint32_t totalNumberOfThreadsX,
            uint32_t totalNumberOfThreadsY,
            uint32_t totalNumberOfThreadsZ,
            uint16_t threadsPerGroupX,
            uint16_t threadsPerGroupY,
            uint16_t threadsPerGroupZ)
            : m_totalNumberOfThreadsX(totalNumberOfThreadsX)
            , m_totalNumberOfThreadsY(totalNumberOfThreadsY)
            , m_totalNumberOfThreadsZ(totalNumberOfThreadsZ)
            , m_threadsPerGroupX(threadsPerGroupX)
            , m_threadsPerGroupY(threadsPerGroupY)
            , m_threadsPerGroupZ(threadsPerGroupZ)
        {}

        uint32_t GetNumberOfGroupsX() const
        {
            return DivideAndRoundUp(m_totalNumberOfThreadsX, aznumeric_cast<uint32_t>(m_threadsPerGroupX));
        }

        uint32_t GetNumberOfGroupsY() const
        {
            return DivideAndRoundUp(m_totalNumberOfThreadsY, aznumeric_cast<uint32_t>(m_threadsPerGroupY));
        }

        uint32_t GetNumberOfGroupsZ() const
        {
            return DivideAndRoundUp(m_totalNumberOfThreadsZ, aznumeric_cast<uint32_t>(m_threadsPerGroupZ));
        }

        // Different platforms require number of groups or number of threads or both in their Dispatch() call
        uint32_t m_totalNumberOfThreadsX = 1; // = numberOfGroupsX * m_threadsPerGroupX
        uint32_t m_totalNumberOfThreadsY = 1; // = numberOfGroupsY * m_threadsPerGroupY
        uint32_t m_totalNumberOfThreadsZ = 1; // = numberOfGroupsZ * m_threadsPerGroupZ

        // Group size (number of threads in a group) must match the declaration in the shader
        uint16_t m_threadsPerGroupX = 1; // = m_totalNumberOfThreadsX / numberOfGroupsX
        uint16_t m_threadsPerGroupY = 1; // = m_totalNumberOfThreadsY / numberOfGroupsY
        uint16_t m_threadsPerGroupZ = 1; // = m_totalNumberOfThreadsZ / numberOfGroupsZ
    };

    //! Arguments used when submitting an indirect dispatch call into a CommandList.
    //! The indirect dispatch arguments are the same ones as the indirect draw ones.
    using DeviceDispatchIndirect = DeviceIndirectArguments;

    enum class DispatchType : uint8_t
    {
        Direct = 0, // A dispatch call where the arguments are pass directly to the submit function.
        Indirect    // An indirect dispatch call that uses a buffer that contains the arguments.
    };

    //! Encapsulates the arguments that are specific to a type of dispatch.
    //! It uses a union to be able to store all possible arguments.
    struct DeviceDispatchArguments
    {
        AZ_TYPE_INFO(DeviceDispatchArguments, "0A354A63-D2C5-4C59-B3E0-0800FA7FBA63");

        DeviceDispatchArguments() : DeviceDispatchArguments(DispatchDirect{}) {}

        DeviceDispatchArguments(const DispatchDirect& direct)
            : m_type{ DispatchType::Direct }
            , m_direct{ direct }
        {}

        DeviceDispatchArguments(const DeviceDispatchIndirect& indirect)
            : m_type{ DispatchType::Indirect }
            , m_indirect{ indirect }
        {}

        DispatchType m_type;
        union
        {
            /// Arguments for a direct dispatch.
            DispatchDirect m_direct;
            /// Arguments for an indirect dispatch.
            DeviceDispatchIndirect m_indirect;
        };
    };

    //! Encapsulates all the necessary information for doing a dispatch call.
    //! This includes all common arguments for the different dispatch type, plus
    //! arguments that are specific to a type.
    struct DeviceDispatchItem
    {
        DeviceDispatchItem() = default;

        /// Arguments specific to a dispatch type.
        DeviceDispatchArguments m_arguments;

        /// The number of shader resource groups and inline constants in each array.
        uint8_t m_shaderResourceGroupCount = 0;
        uint8_t m_rootConstantSize = 0;

        /// The pipeline state to bind.
        const DevicePipelineState* m_pipelineState = nullptr;

        /// Array of shader resource groups to bind (count must match m_shaderResourceGroupCount).
        AZStd::array<const DeviceShaderResourceGroup*, Limits::Pipeline::ShaderResourceGroupCountMax> m_shaderResourceGroups = {};

        /// Unique SRG, not shared within the draw packet. This is usually a per-draw SRG, populated with the shader variant fallback key
        const DeviceShaderResourceGroup* m_uniqueShaderResourceGroup = nullptr;

        /// Inline constants data.
        const uint8_t* m_rootConstants = nullptr;
    };
}
