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

        //! The number of rays to cast
        void SetDimensions(uint32_t width, uint32_t height, uint32_t depth)
        {
            for (auto& [deviceIndex, dispatchRaysItem] : m_deviceDispatchRaysItems)
            {
                dispatchRaysItem.m_width = width;
                dispatchRaysItem.m_height = height;
                dispatchRaysItem.m_depth = depth;
            }
        }

        //! Ray tracing pipeline state
        void SetRayTracingPipelineState(const MultiDeviceRayTracingPipelineState* rayTracingPipelineState)
        {
            for (auto& [deviceIndex, dispatchRaysItem] : m_deviceDispatchRaysItems)
            {
                dispatchRaysItem.m_rayTracingPipelineState = rayTracingPipelineState->GetDevicePipelineState(deviceIndex).get();
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
    };
} // namespace AZ::RHI
