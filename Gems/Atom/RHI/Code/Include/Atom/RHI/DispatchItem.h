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
#include <Atom/RHI/IndirectArguments.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/containers/array.h>

namespace AZ::RHI
{
    //! Arguments used when submitting an indirect dispatch call into a CommandList.
    //! The indirect dispatch arguments are the same ones as the indirect draw ones.
    using DispatchIndirect = IndirectArguments;

    //! Encapsulates the arguments that are specific to a type of dispatch.
    //! It uses a union to be able to store all possible arguments.
    struct DispatchArguments
    {
        AZ_TYPE_INFO(DispatchArguments, "0A354A63-D2C5-4C59-B3E0-0800FA7FBA63");

        DispatchArguments()
            : DispatchArguments(DispatchDirect{})
        {
        }

        DispatchArguments(const DispatchDirect& direct)
            : m_type{ DispatchType::Direct }
            , m_direct{ direct }
        {
        }

        DispatchArguments(const DispatchIndirect& indirect)
            : m_type{ DispatchType::Indirect }
            , m_indirect{ indirect }
        {
        }

        //! Returns the device-specific DeviceDispatchArguments for the given index
        DeviceDispatchArguments GetDeviceDispatchArguments(int deviceIndex) const
        {
            switch (m_type)
            {
            case DispatchType::Direct:
                return DeviceDispatchArguments(m_direct);
            case DispatchType::Indirect:
                return DeviceDispatchArguments(DeviceDispatchIndirect{m_indirect.m_maxSequenceCount, m_indirect.m_indirectBufferView->GetDeviceIndirectBufferView(deviceIndex), m_indirect.m_indirectBufferByteOffset, m_indirect.m_countBuffer ? m_indirect.m_countBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr, m_indirect.m_countBufferByteOffset});
            default:
                return DeviceDispatchArguments();
            }
        }

        DispatchType m_type;
        union {
            //! Arguments for a direct dispatch.
            DispatchDirect m_direct;
            //! Arguments for an indirect dispatch.
            DispatchIndirect m_indirect;
        };
    };

    //! Encapsulates all the necessary information for doing a dispatch call.
    //! This includes all common arguments for the different dispatch type, plus
    //! arguments that are specific to a type.
    class DispatchItem
    {
    public:
        DispatchItem(MultiDevice::DeviceMask deviceMask)
            : m_deviceMask{ deviceMask }
        {
            MultiDeviceObject::IterateDevices(
                m_deviceMask,
                [this](int deviceIndex)
                {
                    m_deviceDispatchItems.emplace(deviceIndex, DeviceDispatchItem{});
                    return true;
                });
        }

        //! Returns the device-specific DeviceDispatchItem for the given index
        const DeviceDispatchItem& GetDeviceDispatchItem(int deviceIndex) const
        {
            AZ_Error(
                "DispatchItem",
                m_deviceDispatchItems.find(deviceIndex) != m_deviceDispatchItems.end(),
                "No DeviceDispatchItem found for device index %d\n",
                deviceIndex);

            return m_deviceDispatchItems.at(deviceIndex);
        }

        //! Retrieve arguments specifing a dispatch type.
        const DispatchArguments& GetArguments() const
        {
            return m_arguments;
        }

        //! Arguments specific to a dispatch type.
        void SetArguments(const DispatchArguments& arguments)
        {
            m_arguments = arguments;

            for (auto& [deviceIndex, dispatchItem] : m_deviceDispatchItems)
            {
                dispatchItem.m_arguments = arguments.GetDeviceDispatchArguments(deviceIndex);
            }
        }

        //! The number of inline constants in each array.
        void SetRootConstantSize(uint8_t rootConstantSize)
        {
            for (auto& [deviceIndex, dispatchItem] : m_deviceDispatchItems)
            {
                dispatchItem.m_rootConstantSize = rootConstantSize;
            }
        }

        void SetPipelineState(const PipelineState* pipelineState)
        {
            for (auto& [deviceIndex, dispatchItem] : m_deviceDispatchItems)
            {
                dispatchItem.m_pipelineState = pipelineState->GetDevicePipelineState(deviceIndex).get();
            }
        }

        void SetDevicePipelineState(int deviceIndex, const DevicePipelineState* devicePipelineState)
        {
            auto& dispatchItem = m_deviceDispatchItems.at(deviceIndex);
            dispatchItem.m_pipelineState = devicePipelineState;
        }

        //! Array of shader resource groups to bind (count must match m_shaderResourceGroupCount).
        void SetShaderResourceGroups(
            const AZStd::span<const ShaderResourceGroup*> shaderResourceGroups)
        {
            for (auto& [deviceIndex, dispatchItem] : m_deviceDispatchItems)
            {
                dispatchItem.m_shaderResourceGroupCount = static_cast<uint8_t>(shaderResourceGroups.size());
                for (int i = 0; i < dispatchItem.m_shaderResourceGroupCount; ++i)
                {
                    dispatchItem.m_shaderResourceGroups[i] = shaderResourceGroups[i]->GetDeviceShaderResourceGroup(deviceIndex).get();
                }
            }
        }

        void SetDeviceShaderResourceGroups(int deviceIndex,
            const DeviceShaderResourceGroup* const* shaderResourceGroups,
            uint8_t shaderResourceGroupCount)
        {
            auto& dispatchItem = m_deviceDispatchItems.at(deviceIndex);
            dispatchItem.m_shaderResourceGroupCount = shaderResourceGroupCount;
            for (uint8_t i = 0; i < shaderResourceGroupCount; i++)
            {
                dispatchItem.m_shaderResourceGroups[i] = shaderResourceGroups[i];
            }
        }

        //! Unique SRG, not shared within the draw packet. This is usually a per-draw SRG, populated with the shader variant fallback
        //! key
        void SetUniqueShaderResourceGroup(const ShaderResourceGroup* uniqueShaderResourceGroup)
        {
            for (auto& [deviceIndex, dispatchItem] : m_deviceDispatchItems)
            {
                dispatchItem.m_uniqueShaderResourceGroup = uniqueShaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get();
            }
        }

        void SetUniqueDeviceShaderResourceGroup(int deviceIndex,
            const DeviceShaderResourceGroup* uniqueShaderResourceGroup)
        {
            auto& dispatchItem = m_deviceDispatchItems.at(deviceIndex);
            dispatchItem.m_uniqueShaderResourceGroup = uniqueShaderResourceGroup;
        }

        //! Inline constants data.
        void SetRootConstants(const uint8_t* rootConstants)
        {
            for (auto& [deviceIndex, dispatchItem] : m_deviceDispatchItems)
            {
                dispatchItem.m_rootConstants = rootConstants;
            }
        }

    private:
        //! A DeviceMask denoting on which devices a device-specific DeviceDispatchItem should be generated
        MultiDevice::DeviceMask m_deviceMask{ MultiDevice::DefaultDevice };
        //! Caching the arguments for the corresponding getter.
        DispatchArguments m_arguments;
        //! A map of all device-specific DeviceDispatchItem, indexed by the device index
        AZStd::unordered_map<int, DeviceDispatchItem> m_deviceDispatchItems;
    };
} // namespace AZ::RHI
