/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/SingleDeviceDispatchItem.h>
#include <Atom/RHI/MultiDeviceIndirectArguments.h>
#include <Atom/RHI/MultiDevicePipelineState.h>
#include <Atom/RHI/MultiDeviceShaderResourceGroup.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/containers/array.h>

namespace AZ::RHI
{
    //! Arguments used when submitting an indirect dispatch call into a CommandList.
    //! The indirect dispatch arguments are the same ones as the indirect draw ones.
    using MultiDeviceDispatchIndirect = MultiDeviceIndirectArguments;

    //! Encapsulates the arguments that are specific to a type of dispatch.
    //! It uses a union to be able to store all possible arguments.
    struct MultiDeviceDispatchArguments
    {
        AZ_TYPE_INFO(MultiDeviceDispatchArguments, "0A354A63-D2C5-4C59-B3E0-0800FA7FBA63");

        MultiDeviceDispatchArguments()
            : MultiDeviceDispatchArguments(DispatchDirect{})
        {
        }

        MultiDeviceDispatchArguments(const DispatchDirect& direct)
            : m_type{ DispatchType::Direct }
            , m_direct{ direct }
        {
        }

        MultiDeviceDispatchArguments(const MultiDeviceDispatchIndirect& indirect)
            : m_type{ DispatchType::Indirect }
            , m_mdIndirect{ indirect }
        {
        }

        //! Returns the device-specific SingleDeviceDispatchArguments for the given index
        SingleDeviceDispatchArguments GetDeviceDispatchArguments(int deviceIndex) const
        {
            switch (m_type)
            {
            case DispatchType::Direct:
                return SingleDeviceDispatchArguments(m_direct);
            case DispatchType::Indirect:
                return SingleDeviceDispatchArguments(DispatchIndirect{m_mdIndirect.m_maxSequenceCount, m_mdIndirect.m_indirectBufferView->GetDeviceIndirectBufferView(deviceIndex), m_mdIndirect.m_indirectBufferByteOffset, m_mdIndirect.m_countBuffer ? m_mdIndirect.m_countBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr, m_mdIndirect.m_countBufferByteOffset});
            default:
                return SingleDeviceDispatchArguments();
            }
        }

        DispatchType m_type;
        union {
            //! Arguments for a direct dispatch.
            DispatchDirect m_direct;
            //! Arguments for an indirect dispatch.
            MultiDeviceDispatchIndirect m_mdIndirect;
        };
    };

    //! Encapsulates all the necessary information for doing a dispatch call.
    //! This includes all common arguments for the different dispatch type, plus
    //! arguments that are specific to a type.
    class MultiDeviceDispatchItem
    {
    public:
        MultiDeviceDispatchItem(MultiDevice::DeviceMask deviceMask)
            : m_deviceMask{ deviceMask }
        {
            auto deviceCount{ RHI::RHISystemInterface::Get()->GetDeviceCount() };

            for (int deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
            {
                if (CheckBitsAll(AZStd::to_underlying(m_deviceMask), 1u << deviceIndex))
                {
                    m_deviceDispatchItems.emplace(deviceIndex, SingleDeviceDispatchItem{});
                }
            }
        }

        //! Returns the device-specific SingleDeviceDispatchItem for the given index
        const SingleDeviceDispatchItem& GetDeviceDispatchItem(int deviceIndex) const
        {
            AZ_Error(
                "MultiDeviceDispatchItem",
                m_deviceDispatchItems.find(deviceIndex) != m_deviceDispatchItems.end(),
                "No DeviceDispatchItem found for device index %d\n",
                deviceIndex);

            return m_deviceDispatchItems.at(deviceIndex);
        }

        //! Retrieve arguments specifing a dispatch type.
        const MultiDeviceDispatchArguments& GetArguments() const
        {
            return m_arguments;
        }

        //! Arguments specific to a dispatch type.
        void SetArguments(const MultiDeviceDispatchArguments& arguments)
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

        void SetPipelineState(const MultiDevicePipelineState* pipelineState)
        {
            for (auto& [deviceIndex, dispatchItem] : m_deviceDispatchItems)
            {
                dispatchItem.m_pipelineState = pipelineState->GetDevicePipelineState(deviceIndex).get();
            }
        }

        //! Array of shader resource groups to bind (count must match m_shaderResourceGroupCount).
        void SetShaderResourceGroups(
            const AZStd::span<const MultiDeviceShaderResourceGroup*> shaderResourceGroups)
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

        //! Unique SRG, not shared within the draw packet. This is usually a per-draw SRG, populated with the shader variant fallback
        //! key
        void SetUniqueShaderResourceGroup(const MultiDeviceShaderResourceGroup* uniqueShaderResourceGroup)
        {
            for (auto& [deviceIndex, dispatchItem] : m_deviceDispatchItems)
            {
                dispatchItem.m_uniqueShaderResourceGroup = uniqueShaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get();
            }
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
        //! A DeviceMask denoting on which devices a device-specific SingleDeviceDispatchItem should be generated
        MultiDevice::DeviceMask m_deviceMask{ MultiDevice::DefaultDevice };
        //! Caching the arguments for the corresponding getter.
        MultiDeviceDispatchArguments m_arguments;
        //! A map of all device-specific SingleDeviceDispatchItem, indexed by the device index
        AZStd::unordered_map<int, SingleDeviceDispatchItem> m_deviceDispatchItems;
    };
} // namespace AZ::RHI
