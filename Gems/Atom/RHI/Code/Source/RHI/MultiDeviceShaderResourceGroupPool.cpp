/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/ImageView.h>
#include <Atom/RHI/MultiDeviceShaderResourceGroupPool.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <AzCore/Console/IConsole.h>

namespace AZ
{
    namespace RHI
    {
        ResultCode MultiDeviceShaderResourceGroupPool::Init(
            MultiDevice::DeviceMask deviceMask, const ShaderResourceGroupPoolDescriptor& descriptor)
        {
            if (Validation::IsEnabled())
            {
                if (descriptor.m_layout == nullptr)
                {
                    AZ_Error("MultiDeviceShaderResourceGroupPool", false, "ShaderResourceGroupPoolDescriptor::m_layout must not be null.");
                    return ResultCode::InvalidArgument;
                }
            }

            ResultCode resultCode = MultiDeviceResourcePool::Init(
                deviceMask,
                [this, &descriptor]()
                {
                    IterateDevices(
                        [this, &descriptor](int deviceIndex)
                        {
                            auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                            m_deviceShaderResourceGroupPools[deviceIndex] = Factory::Get().CreateShaderResourceGroupPool();
                            m_deviceShaderResourceGroupPools[deviceIndex]->Init(*device, descriptor);

                            return true;
                        });
                    return ResultCode::Success;
                });

            if (resultCode != ResultCode::Success)
            {
                return resultCode;
            }

            m_descriptor = descriptor;
            m_hasBufferGroup = m_descriptor.m_layout->GetGroupSizeForBuffers() > 0;
            m_hasImageGroup = m_descriptor.m_layout->GetGroupSizeForImages() > 0;
            m_hasSamplerGroup = m_descriptor.m_layout->GetGroupSizeForSamplers() > 0;
            m_hasConstants = m_descriptor.m_layout->GetConstantDataSize() > 0;

            return ResultCode::Success;
        }

        ResultCode MultiDeviceShaderResourceGroupPool::InitGroup(MultiDeviceShaderResourceGroup& group)
        {
            ResultCode resultCode = MultiDeviceResourcePool::InitResource(
                &group,
                [this, &group]()
                {
                    ResultCode result = ResultCode::Success;

                    for (auto& [deviceIndex, deviceShaderResourceGroupPool] : m_deviceShaderResourceGroupPools)
                    {
                        group.m_deviceShaderResourceGroups[deviceIndex] = Factory::Get().CreateShaderResourceGroup();
                        result = m_deviceShaderResourceGroupPools[deviceIndex]->InitGroup(*group.m_deviceShaderResourceGroups[deviceIndex]);

                        if (result != ResultCode::Success)
                            break;
                    }

                    return result;
                });

            if (resultCode == ResultCode::Success)
            {
                const ShaderResourceGroupLayout* layout = GetLayout();

                // Pre-initialize the data so that we can build view diffs later.
                group.m_data = MultiDeviceShaderResourceGroupData(GetDeviceMask(), layout);

                // Cache off the binding slot for one less indirection.
                group.m_bindingSlot = layout->GetBindingSlot();
            }

            return resultCode;
        }

        void MultiDeviceShaderResourceGroupPool::CompileGroupsBegin()
        {
            for (auto& [deviceIndex, deviceShaderResourceGroupPool] : m_deviceShaderResourceGroupPools)
            {
                deviceShaderResourceGroupPool->CompileGroupsBegin();
            }
        }

        void MultiDeviceShaderResourceGroupPool::CompileGroupsEnd()
        {
            for (auto& [deviceIndex, deviceShaderResourceGroupPool] : m_deviceShaderResourceGroupPools)
            {
                deviceShaderResourceGroupPool->CompileGroupsEnd();
            }
        }

        uint32_t MultiDeviceShaderResourceGroupPool::GetGroupsToCompileCount() const
        {
            auto groupCount{ 0u };
            for (auto& [deviceIndex, deviceShaderResourceGroupPool] : m_deviceShaderResourceGroupPools)
                groupCount += deviceShaderResourceGroupPool->GetGroupsToCompileCount();

            return groupCount;
        }

        ResultCode MultiDeviceShaderResourceGroupPool::CompileGroup(
            MultiDeviceShaderResourceGroup& shaderResourceGroup, const MultiDeviceShaderResourceGroupData& shaderResourceGroupData)
        {
            ResultCode resultCode = ResultCode::Success;

            for (auto& [deviceIndex, deviceShaderResourceGroup] : shaderResourceGroup.m_deviceShaderResourceGroups)
            {
                resultCode = m_deviceShaderResourceGroupPools[deviceIndex]->CompileGroup(
                    *deviceShaderResourceGroup, shaderResourceGroupData.GetDeviceShaderResourceGroupData(deviceIndex));
                if (resultCode != ResultCode::Success)
                    break;
            }

            return resultCode;
        }

        void MultiDeviceShaderResourceGroupPool::CompileGroupsForInterval(Interval interval)
        {
            auto doOverlap = [](Interval groupInterval, Interval givenInterval)
            {
                return (groupInterval.m_min < givenInterval.m_max) && (groupInterval.m_max > givenInterval.m_min);
            };

            auto currentStart{ 0u };
            for (auto& [deviceIndex, deviceShaderResourceGroupPool] : m_deviceShaderResourceGroupPools)
            {
                auto groupsToCompile{ static_cast<int>(deviceShaderResourceGroupPool->GetGroupsToCompileCount()) };

                if (doOverlap({ currentStart, currentStart + groupsToCompile }, interval))
                {
                    auto startOffset{ static_cast<int>(currentStart) - static_cast<int>(interval.m_min) };
                    auto endOffset{ static_cast<int>(currentStart + groupsToCompile) - static_cast<int>(interval.m_max) };
                    deviceShaderResourceGroupPool->CompileGroupsForInterval(
                        { static_cast<uint32_t>((startOffset <= 0) ? 0 : startOffset),
                          static_cast<uint32_t>((endOffset <= 0) ? groupsToCompile : groupsToCompile - endOffset) });
                }
                currentStart += groupsToCompile;
            }
        }

        const ShaderResourceGroupPoolDescriptor& MultiDeviceShaderResourceGroupPool::GetDescriptor() const
        {
            return m_descriptor;
        }

        const ShaderResourceGroupLayout* MultiDeviceShaderResourceGroupPool::GetLayout() const
        {
            AZ_Assert(m_descriptor.m_layout, "Shader resource group layout is null");
            return m_descriptor.m_layout.get();
        }

        bool MultiDeviceShaderResourceGroupPool::HasConstants() const
        {
            return m_hasConstants;
        }

        bool MultiDeviceShaderResourceGroupPool::HasBufferGroup() const
        {
            return m_hasBufferGroup;
        }

        bool MultiDeviceShaderResourceGroupPool::HasImageGroup() const
        {
            return m_hasImageGroup;
        }

        bool MultiDeviceShaderResourceGroupPool::HasSamplerGroup() const
        {
            return m_hasSamplerGroup;
        }

        void MultiDeviceShaderResourceGroupPool::Shutdown()
        {
            for (auto [_, pool] : m_deviceShaderResourceGroupPools)
                pool->Shutdown();
            MultiDeviceResourcePool::Shutdown();
        }
    } // namespace RHI
} // namespace AZ
