/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/SingleDeviceDispatchRaysItem.h>
#include <Atom/RHI/MultiDeviceDispatchRaysIndirectBuffer.h>
#include <Atom/RHI/MultiDeviceRayTracingAccelerationStructure.h>
#include <Atom/RHI/MultiDeviceRayTracingPipelineState.h>
#include <Atom/RHI/MultiDeviceRayTracingShaderTable.h>
#include <Atom/RHI/MultiDeviceShaderResourceGroup.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <AzCore/std/containers/array.h>

namespace AZ::RHI
{
    class MultiDeviceRayTracingPipelineState;
    class MultiDeviceRayTracingShaderTable;
    class MultiDeviceShaderResourceGroup;
    class SingleDeviceImageView;
    class SingleDeviceBufferView;

    struct MultiDeviceDispatchRaysIndirect : public MultiDeviceIndirectArguments
    {
        MultiDeviceDispatchRaysIndirect() = default;

        MultiDeviceDispatchRaysIndirect(
            uint32_t maxSequenceCount,
            const MultiDeviceIndirectBufferView& indirectBuffer,
            uint64_t indirectBufferByteOffset,
            MultiDeviceDispatchRaysIndirectBuffer* dispatchRaysIndirectBuffer)
            : MultiDeviceDispatchRaysIndirect(
                  maxSequenceCount, indirectBuffer, indirectBufferByteOffset, dispatchRaysIndirectBuffer, nullptr, 0)
        {
        }

        MultiDeviceDispatchRaysIndirect(
            uint32_t maxSequenceCount,
            const MultiDeviceIndirectBufferView& indirectBuffer,
            uint64_t indirectBufferByteOffset,
            MultiDeviceDispatchRaysIndirectBuffer* dispatchRaysIndirectBuffer,
            const MultiDeviceBuffer* countBuffer,
            uint64_t countBufferByteOffset)
            : MultiDeviceIndirectArguments(maxSequenceCount, indirectBuffer, indirectBufferByteOffset, countBuffer, countBufferByteOffset)
            , m_dispatchRaysIndirectBuffer(dispatchRaysIndirectBuffer)
        {
        }

        MultiDeviceDispatchRaysIndirectBuffer* m_dispatchRaysIndirectBuffer = nullptr;
    };

    //! Encapsulates the arguments that are specific to a type of dispatch.
    //! It uses a union to be able to store all possible arguments.
    struct MultiDeviceDispatchRaysArguments
    {
        AZ_TYPE_INFO(MultiDeviceDispatchRaysArguments, "1080A8F2-0BDE-497E-9CBD-C55575623AFD");

        MultiDeviceDispatchRaysArguments()
            : MultiDeviceDispatchRaysArguments(DispatchRaysDirect{})
        {
        }

        MultiDeviceDispatchRaysArguments(const DispatchRaysDirect& direct)
            : m_type{ DispatchRaysType::Direct }
            , m_direct{ direct }
        {
        }

        MultiDeviceDispatchRaysArguments(const MultiDeviceDispatchRaysIndirect& indirect)
            : m_type{ DispatchRaysType::Indirect }
            , m_mdIndirect{ indirect }
        {
        }

        //! Returns the device-specific SingleDeviceDispatchArguments for the given index
        SingleDeviceDispatchRaysArguments GetDeviceDispatchRaysArguments(int deviceIndex) const
        {
            switch (m_type)
            {
            case DispatchRaysType::Direct:
                return SingleDeviceDispatchRaysArguments(m_direct);
            case DispatchRaysType::Indirect:
                return SingleDeviceDispatchRaysArguments(DispatchRaysIndirect{
                    m_mdIndirect.m_maxSequenceCount,
                    m_mdIndirect.m_indirectBufferView->GetDeviceIndirectBufferView(deviceIndex),
                    m_mdIndirect.m_indirectBufferByteOffset,
                    m_mdIndirect.m_dispatchRaysIndirectBuffer
                        ? m_mdIndirect.m_dispatchRaysIndirectBuffer->GetDeviceDispatchRaysIndirectBuffer(deviceIndex).get()
                        : nullptr,
                    m_mdIndirect.m_countBuffer ? m_mdIndirect.m_countBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr,
                    m_mdIndirect.m_countBufferByteOffset });
            default:
                return SingleDeviceDispatchRaysArguments();
            }
        }

        DispatchRaysType m_type;
        union {
            //! Arguments for a direct dispatch.
            DispatchRaysDirect m_direct;
            //! Arguments for an indirect dispatch.
            MultiDeviceDispatchRaysIndirect m_mdIndirect;
        };
    };

    //! Encapsulates all the necessary information for doing a ray tracing dispatch call.
    class MultiDeviceDispatchRaysItem
    {
    public:
        MultiDeviceDispatchRaysItem(MultiDevice::DeviceMask deviceMask)
            : m_deviceMask{ deviceMask }
        {
            auto deviceCount{ RHI::RHISystemInterface::Get()->GetDeviceCount() };

            for (int deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
            {
                if (CheckBitsAll(AZStd::to_underlying(m_deviceMask), 1u << deviceIndex))
                {
                    m_deviceDispatchRaysItems.emplace(deviceIndex, SingleDeviceDispatchRaysItem{});
                }
            }
        }

        //! Returns the device-specific SingleDeviceDispatchRaysItem for the given index
        const SingleDeviceDispatchRaysItem& GetDeviceDispatchRaysItem(int deviceIndex) const
        {
            AZ_Error(
                "MultiDeviceDispatchItem",
                m_deviceDispatchRaysItems.find(deviceIndex) != m_deviceDispatchRaysItems.end(),
                "No DeviceDispatchItem found for device index %d\n",
                deviceIndex);

            return m_deviceDispatchRaysItems.at(deviceIndex);
        }

        //! Retrieve arguments specifing a dispatch type.
        const MultiDeviceDispatchRaysArguments& GetArguments() const
        {
            return m_arguments;
        }

        //! Arguments specific to a dispatch type.
        void SetArguments(const MultiDeviceDispatchRaysArguments& arguments)
        {
            m_arguments = arguments;

            for (auto& [deviceIndex, dispatchRaysItem] : m_deviceDispatchRaysItems)
            {
                dispatchRaysItem.m_arguments = arguments.GetDeviceDispatchRaysArguments(deviceIndex);
            }
        }

        //! Ray tracing pipeline state
        void SetRayTracingPipelineState(const MultiDeviceRayTracingPipelineState* rayTracingPipelineState)
        {
            for (auto& [deviceIndex, dispatchRaysItem] : m_deviceDispatchRaysItems)
            {
                dispatchRaysItem.m_rayTracingPipelineState = rayTracingPipelineState->GetDeviceRayTracingPipelineState(deviceIndex).get();
            }
        }

        //! Ray tracing shader table
        void SetRayTracingShaderTable(const MultiDeviceRayTracingShaderTable* rayTracingShaderTable)
        {
            for (auto& [deviceIndex, dispatchRaysItem] : m_deviceDispatchRaysItems)
            {
                dispatchRaysItem.m_rayTracingShaderTable = rayTracingShaderTable->GetDeviceRayTracingShaderTable(deviceIndex).get();
            }
        }

        //! Shader Resource Groups
        void SetShaderResourceGroups(const MultiDeviceShaderResourceGroup* const* shaderResourceGroups, uint32_t shaderResourceGroupCount)
        {
            for (auto& [deviceIndex, dispatchRaysItem] : m_deviceDispatchRaysItems)
            {
                dispatchRaysItem.m_shaderResourceGroupCount = shaderResourceGroupCount;

                auto [it, insertOK]{ m_deviceShaderResourceGroups.emplace(
                    deviceIndex, AZStd::vector<SingleDeviceShaderResourceGroup*>(shaderResourceGroupCount)) };

                auto& [index, deviceShaderResourceGroup]{ *it };

                for (int i = 0; i < shaderResourceGroupCount; ++i)
                {
                    deviceShaderResourceGroup[i] = shaderResourceGroups[i]->GetDeviceShaderResourceGroup(deviceIndex).get();
                }

                dispatchRaysItem.m_shaderResourceGroups = deviceShaderResourceGroup.data();
            }
        }

        //! Global shader pipeline state
        void SetPipelineState(const MultiDevicePipelineState* globalPipelineState)
        {
            for (auto& [deviceIndex, dispatchRaysItem] : m_deviceDispatchRaysItems)
            {
                dispatchRaysItem.m_globalPipelineState = globalPipelineState->GetDevicePipelineState(deviceIndex).get();
            }
        }

    private:
        //! A DeviceMask denoting on which devices a device-specific SingleDeviceDispatchRaysItem should be generated
        MultiDevice::DeviceMask m_deviceMask{ MultiDevice::DefaultDevice };
        //! A map of all device-specific SingleDeviceDispatchRaysItem, indexed by the device index
        AZStd::unordered_map<int, SingleDeviceDispatchRaysItem> m_deviceDispatchRaysItems;
        //! A map of all device-specific ShaderResourceGroups, indexed by the device index
        AZStd::unordered_map<int, AZStd::vector<SingleDeviceShaderResourceGroup*>> m_deviceShaderResourceGroups;
        //! Caching the arguments for the corresponding getter.
        MultiDeviceDispatchRaysArguments m_arguments;
    };
} // namespace AZ::RHI
