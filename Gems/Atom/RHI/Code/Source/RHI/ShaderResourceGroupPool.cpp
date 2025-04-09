/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/DeviceBufferView.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RHI/ShaderResourceGroupPool.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <AzCore/Console/IConsole.h>

namespace AZ::RHI
{
    ResultCode ShaderResourceGroupPool::Init(const ShaderResourceGroupPoolDescriptor& descriptor)
    {
        if (Validation::IsEnabled())
        {
            if (descriptor.m_layout == nullptr)
            {
                AZ_Error("ShaderResourceGroupPool", false, "ShaderResourceGroupPoolDescriptor::m_layout must not be null.");
                return ResultCode::InvalidArgument;
            }
        }

        ResultCode resultCode = ResourcePool::Init(
            descriptor.m_deviceMask,
            [this, &descriptor]()
            {
                IterateDevices(
                    [this, &descriptor](int deviceIndex)
                    {
                        auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                        m_deviceObjects[deviceIndex] = Factory::Get().CreateShaderResourceGroupPool();

                        GetDeviceShaderResourceGroupPool(deviceIndex)->Init(*device, descriptor);

                        return true;
                    });
                return ResultCode::Success;
            });

        if (resultCode != ResultCode::Success)
        {
            // Reset already initialized device-specific ShaderResourceGroupPools and set deviceMask to 0
            m_deviceObjects.clear();
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
            return resultCode;
        }

        m_descriptor = descriptor;
        m_hasBufferGroup = m_descriptor.m_layout->GetGroupSizeForBuffers() > 0;
        m_hasImageGroup = m_descriptor.m_layout->GetGroupSizeForImages() > 0;
        m_hasSamplerGroup = m_descriptor.m_layout->GetGroupSizeForSamplers() > 0;
        m_hasConstants = m_descriptor.m_layout->GetConstantDataSize() > 0;

        return ResultCode::Success;
    }

    ResultCode ShaderResourceGroupPool::InitGroup(ShaderResourceGroup& group)
    {
        ResultCode resultCode = ResourcePool::InitResource(
            &group,
            [this, &group]()
            {
                return IterateObjects<DeviceShaderResourceGroupPool>([this, &group](auto deviceIndex, [[maybe_unused]] auto deviceShaderResourceGroupPool)
                {
                    group.m_deviceObjects[deviceIndex] = Factory::Get().CreateShaderResourceGroup();

                    return GetDeviceShaderResourceGroupPool(deviceIndex)->InitGroup(*group.GetDeviceShaderResourceGroup(deviceIndex));
                });
            });

        if (resultCode == ResultCode::Success)
        {
            const ShaderResourceGroupLayout* layout = GetLayout();

            // Pre-initialize the data so that we can build view diffs later.
            group.m_data = ShaderResourceGroupData(GetDeviceMask(), layout);

            // Cache off the binding slot for one less indirection.
            group.m_bindingSlot = layout->GetBindingSlot();
        }
        else
        {
            // Reset already initialized device-specific ShaderResourceGroupPools and set deviceMask to 0
            m_deviceObjects.clear();
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
        }

        return resultCode;
    }

    void ShaderResourceGroupPool::CompileGroupsBegin()
    {
        IterateObjects<DeviceShaderResourceGroupPool>([]([[maybe_unused]] auto deviceIndex, auto deviceShaderResourceGroupPool)
        {
            deviceShaderResourceGroupPool->CompileGroupsBegin();
        });
    }

    void ShaderResourceGroupPool::CompileGroupsEnd()
    {
        IterateObjects<DeviceShaderResourceGroupPool>([]([[maybe_unused]] auto deviceIndex, auto deviceShaderResourceGroupPool)
        {
            deviceShaderResourceGroupPool->CompileGroupsEnd();
        });
    }

    uint32_t ShaderResourceGroupPool::GetGroupsToCompileCount() const
    {
        auto groupCount{ 0u };
        IterateObjects<DeviceShaderResourceGroupPool>([&groupCount]([[maybe_unused]] auto deviceIndex, auto deviceShaderResourceGroupPool)
        {
            groupCount += deviceShaderResourceGroupPool->GetGroupsToCompileCount();
        });

        return groupCount;
    }

    ResultCode ShaderResourceGroupPool::CompileGroup(
        ShaderResourceGroup& shaderResourceGroup, const ShaderResourceGroupData& shaderResourceGroupData)
    {
        return shaderResourceGroup.IterateObjects<DeviceShaderResourceGroup>([this, &shaderResourceGroupData](auto deviceIndex, auto deviceShaderResourceGroup)
        {
            return GetDeviceShaderResourceGroupPool(deviceIndex)->CompileGroup(
                *deviceShaderResourceGroup, shaderResourceGroupData.GetDeviceShaderResourceGroupData(deviceIndex));
        });
    }

    void ShaderResourceGroupPool::CompileGroupsForInterval(Interval interval)
    {
        auto doOverlap = [](Interval groupInterval, Interval givenInterval)
        {
            return (groupInterval.m_min < givenInterval.m_max) && (groupInterval.m_max > givenInterval.m_min);
        };

        auto currentStart{ 0u };
        IterateObjects<DeviceShaderResourceGroupPool>([&doOverlap, &interval, &currentStart]([[maybe_unused]] auto deviceIndex, auto deviceShaderResourceGroupPool)
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
        });
    }

    const ShaderResourceGroupPoolDescriptor& ShaderResourceGroupPool::GetDescriptor() const
    {
        return m_descriptor;
    }

    const ShaderResourceGroupLayout* ShaderResourceGroupPool::GetLayout() const
    {
        AZ_Assert(m_descriptor.m_layout, "Shader resource group layout is null");
        return m_descriptor.m_layout.get();
    }

    bool ShaderResourceGroupPool::HasConstants() const
    {
        return m_hasConstants;
    }

    bool ShaderResourceGroupPool::HasBufferGroup() const
    {
        return m_hasBufferGroup;
    }

    bool ShaderResourceGroupPool::HasImageGroup() const
    {
        return m_hasImageGroup;
    }

    bool ShaderResourceGroupPool::HasSamplerGroup() const
    {
        return m_hasSamplerGroup;
    }

    void ShaderResourceGroupPool::Shutdown()
    {
        ResourcePool::Shutdown();
    }
} // namespace AZ::RHI
