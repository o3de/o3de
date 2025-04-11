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
#include <Atom/RHI/DispatchRaysIndirectBuffer.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <Atom/RHI/RayTracingPipelineState.h>
#include <Atom/RHI/RayTracingShaderTable.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <AzCore/std/containers/array.h>

namespace AZ::RHI
{
    class RayTracingPipelineState;
    class RayTracingShaderTable;
    class ShaderResourceGroup;
    class DeviceImageView;
    class DeviceBufferView;

    struct DispatchRaysIndirect : public IndirectArguments
    {
        DispatchRaysIndirect() = default;

        DispatchRaysIndirect(
            uint32_t maxSequenceCount,
            const IndirectBufferView& indirectBuffer,
            uint64_t indirectBufferByteOffset,
            DispatchRaysIndirectBuffer* dispatchRaysIndirectBuffer)
            : DispatchRaysIndirect(
                  maxSequenceCount, indirectBuffer, indirectBufferByteOffset, dispatchRaysIndirectBuffer, nullptr, 0)
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

    //! Encapsulates the arguments that are specific to a type of dispatch.
    //! It uses a union to be able to store all possible arguments.
    struct DispatchRaysArguments
    {
        AZ_TYPE_INFO(DispatchRaysArguments, "1080A8F2-0BDE-497E-9CBD-C55575623AFD");

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

        //! Returns the device-specific DeviceDispatchArguments for the given index
        DeviceDispatchRaysArguments GetDeviceDispatchRaysArguments(int deviceIndex) const
        {
            switch (m_type)
            {
            case DispatchRaysType::Direct:
                return DeviceDispatchRaysArguments(m_direct);
            case DispatchRaysType::Indirect:
                return DeviceDispatchRaysArguments(DeviceDispatchRaysIndirect{
                    m_indirect.m_maxSequenceCount,
                    m_indirect.m_indirectBufferView->GetDeviceIndirectBufferView(deviceIndex),
                    m_indirect.m_indirectBufferByteOffset,
                    m_indirect.m_dispatchRaysIndirectBuffer
                        ? m_indirect.m_dispatchRaysIndirectBuffer->GetDeviceDispatchRaysIndirectBuffer(deviceIndex).get()
                        : nullptr,
                    m_indirect.m_countBuffer ? m_indirect.m_countBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr,
                    m_indirect.m_countBufferByteOffset });
            default:
                return DeviceDispatchRaysArguments();
            }
        }

        DispatchRaysType m_type;
        union {
            //! Arguments for a direct dispatch.
            DispatchRaysDirect m_direct;
            //! Arguments for an indirect dispatch.
            DispatchRaysIndirect m_indirect;
        };
    };

    //! Encapsulates all the necessary information for doing a ray tracing dispatch call.
    class DispatchRaysItem
    {
    public:
        DispatchRaysItem(MultiDevice::DeviceMask deviceMask)
            : m_deviceMask{ deviceMask }
        {
            MultiDeviceObject::IterateDevices(
                m_deviceMask,
                [this](int deviceIndex)
                {
                    m_deviceDispatchRaysItems.emplace(deviceIndex, DeviceDispatchRaysItem{});
                    return true;
                });
        }

        //! Returns the device-specific DeviceDispatchRaysItem for the given index
        const DeviceDispatchRaysItem& GetDeviceDispatchRaysItem(int deviceIndex) const
        {
            AZ_Error(
                "DispatchItem",
                m_deviceDispatchRaysItems.find(deviceIndex) != m_deviceDispatchRaysItems.end(),
                "No DeviceDispatchItem found for device index %d\n",
                deviceIndex);

            return m_deviceDispatchRaysItems.at(deviceIndex);
        }

        //! Retrieve arguments specifing a dispatch type.
        const DispatchRaysArguments& GetArguments() const
        {
            return m_arguments;
        }

        //! Arguments specific to a dispatch type.
        void SetArguments(const DispatchRaysArguments& arguments)
        {
            m_arguments = arguments;

            for (auto& [deviceIndex, dispatchRaysItem] : m_deviceDispatchRaysItems)
            {
                dispatchRaysItem.m_arguments = arguments.GetDeviceDispatchRaysArguments(deviceIndex);
            }
        }

        //! Ray tracing pipeline state
        void SetRayTracingPipelineState(const RayTracingPipelineState* rayTracingPipelineState)
        {
            for (auto& [deviceIndex, dispatchRaysItem] : m_deviceDispatchRaysItems)
            {
                dispatchRaysItem.m_rayTracingPipelineState = rayTracingPipelineState->GetDeviceRayTracingPipelineState(deviceIndex).get();
            }
        }

        //! Ray tracing shader table
        void SetRayTracingShaderTable(const RayTracingShaderTable* rayTracingShaderTable)
        {
            for (auto& [deviceIndex, dispatchRaysItem] : m_deviceDispatchRaysItems)
            {
                dispatchRaysItem.m_rayTracingShaderTable = rayTracingShaderTable->GetDeviceRayTracingShaderTable(deviceIndex).get();
            }
        }

        //! Shader Resource Groups
        void SetShaderResourceGroups(const ShaderResourceGroup* const* shaderResourceGroups, uint32_t shaderResourceGroupCount)
        {
            for (auto& [deviceIndex, dispatchRaysItem] : m_deviceDispatchRaysItems)
            {
                dispatchRaysItem.m_shaderResourceGroupCount = shaderResourceGroupCount;

                auto [it, insertOK]{ m_deviceShaderResourceGroups.emplace(
                    deviceIndex, AZStd::vector<DeviceShaderResourceGroup*>(shaderResourceGroupCount)) };

                auto& [index, deviceShaderResourceGroup]{ *it };

                for (int i = 0; i < static_cast<int>(shaderResourceGroupCount); ++i)
                {
                    deviceShaderResourceGroup[i] = shaderResourceGroups[i]->GetDeviceShaderResourceGroup(deviceIndex).get();
                }

                dispatchRaysItem.m_shaderResourceGroups = deviceShaderResourceGroup.data();
            }
        }

        //! Global shader pipeline state
        void SetPipelineState(const PipelineState* globalPipelineState)
        {
            for (auto& [deviceIndex, dispatchRaysItem] : m_deviceDispatchRaysItems)
            {
                dispatchRaysItem.m_globalPipelineState = globalPipelineState->GetDevicePipelineState(deviceIndex).get();
            }
        }

    private:
        //! A DeviceMask denoting on which devices a device-specific DeviceDispatchRaysItem should be generated
        MultiDevice::DeviceMask m_deviceMask{ MultiDevice::DefaultDevice };
        //! A map of all device-specific DeviceDispatchRaysItem, indexed by the device index
        AZStd::unordered_map<int, DeviceDispatchRaysItem> m_deviceDispatchRaysItems;
        //! A map of all device-specific ShaderResourceGroups, indexed by the device index
        AZStd::unordered_map<int, AZStd::vector<DeviceShaderResourceGroup*>> m_deviceShaderResourceGroups;
        //! Caching the arguments for the corresponding getter.
        DispatchRaysArguments m_arguments;
    };
} // namespace AZ::RHI
