/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/ShaderResourceGroupPool.h>

#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/DeviceShaderResourceGroupPool.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/ImageView.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <AzCore/Console/IConsole.h>

namespace AZ
{
    namespace RHI
    {
        ShaderResourceGroupPool::ShaderResourceGroupPool() {}

        ShaderResourceGroupPool::~ShaderResourceGroupPool() {}

        ResultCode ShaderResourceGroupPool::Init(DeviceMask deviceMask, const ShaderResourceGroupPoolDescriptor& descriptor)
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
                deviceMask,
                descriptor,
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

        void ShaderResourceGroupPool::ShutdownInternal()
        {
        }

        ResultCode ShaderResourceGroupPool::InitGroup(ShaderResourceGroup& group)
        {
            ResultCode resultCode = ResourcePool::InitResource(
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

            return resultCode;
        }

        void ShaderResourceGroupPool::ShutdownResourceInternal(Resource& resourceBase)
        {
            ShaderResourceGroup& shaderResourceGroup = static_cast<ShaderResourceGroup&>(resourceBase);

            shaderResourceGroup.SetData(ShaderResourceGroupData());
        }

        void ShaderResourceGroupPool::CompileGroupsBegin()
        {
            for (auto& [deviceIndex, deviceShaderResourceGroupPool] : m_deviceShaderResourceGroupPools)
            {
                deviceShaderResourceGroupPool->CompileGroupsBegin();
            }
        }

        void ShaderResourceGroupPool::CompileGroupsEnd()
        {
            for (auto& [deviceIndex, deviceShaderResourceGroupPool] : m_deviceShaderResourceGroupPools)
            {
                deviceShaderResourceGroupPool->CompileGroupsEnd();
            }
        }

        uint32_t ShaderResourceGroupPool::GetGroupsToCompileCount() const
        {
            if (!m_deviceShaderResourceGroupPools.empty())
                return m_deviceShaderResourceGroupPools.begin()->second->GetGroupsToCompileCount();

            return 0;
        }

        ResultCode ShaderResourceGroupPool::CompileGroup(
            ShaderResourceGroup& shaderResourceGroup, const ShaderResourceGroupData& shaderResourceGroupData)
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

        void ShaderResourceGroupPool::CompileGroupsForInterval(Interval interval)
        {
            for (auto& [deviceIndex, deviceShaderResourceGroupPool] : m_deviceShaderResourceGroupPools)
            {
                deviceShaderResourceGroupPool->CompileGroupsForInterval(interval);
            }
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
    }
}
