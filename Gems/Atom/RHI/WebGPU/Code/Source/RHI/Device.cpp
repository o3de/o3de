/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Device.h>
#include <RHI/Instance.h>
#include <RHI/Conversions.h>
#include <RHI/PhysicalDevice.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI.Reflect/WebGPU/PlatformLimitsDescriptor.h>

namespace AZ::WebGPU
{
    RHI::Ptr<Device> Device::Create()
    {
        return aznew Device();
    }

    Device::Device()
    {
        RHI::Ptr<PlatformLimitsDescriptor> platformLimitsDescriptor = aznew PlatformLimitsDescriptor();
        platformLimitsDescriptor->LoadPlatformLimitsDescriptor(RHI::Factory::Get().GetName().GetCStr());
        m_descriptor.m_platformLimitsDescriptor = RHI::Ptr<RHI::PlatformLimitsDescriptor>(platformLimitsDescriptor);
    }

    RHI::ResultCode Device::InitInternal(RHI::PhysicalDevice& physicalDeviceBase)
    {
        PhysicalDevice& physicalDevice = static_cast<PhysicalDevice&>(physicalDeviceBase);
        // Set device callbacks
        m_wgpuDeviceDesc = {};
        m_wgpuDeviceDesc.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous, Device::DeviceLostCallback);
        m_wgpuDeviceDesc.SetUncapturedErrorCallback(&Device::ErrorCallback);

            // Synchronously create the device

        auto& instance = Instance::GetInstance().GetNativeInstance();
        m_wgpuDevice = nullptr;
        instance.WaitAny(
            physicalDevice.GetNativeAdapter().RequestDevice(
                &m_wgpuDeviceDesc,
                wgpu::CallbackMode::WaitAnyOnly,
                [this](wgpu::RequestDeviceStatus status, wgpu::Device device, const char* message)
                {
                    if (status != wgpu::RequestDeviceStatus::Success)
                    {
                        AZ_Assert(false, "Failed to get a device: %s", message);
                        return;
                    }
                    m_wgpuDevice = std::move(device);
                }),
            UINT64_MAX);

        if (!m_wgpuDevice)
        {
            return RHI::ResultCode::Fail;
        }

        return RHI::ResultCode::Success;
    }

    RHI::ResultCode Device::BeginFrameInternal()
    {
        return RHI::ResultCode::Success;
    }

    void Device::EndFrameInternal()
    {
        m_frameCommandLists.clear();
    }

    const wgpu::Device& Device::GetNativeDevice() const
    {
        return m_wgpuDevice;
    }

    wgpu::Device& Device::GetNativeDevice()
    {
        return m_wgpuDevice;
    }

    CommandList* Device::AcquireCommandList([[maybe_unused]] RHI::HardwareQueueClass hardwareQueueClass)
    {
        RHI::Ptr<CommandList> commandList = CommandList::Create();
        m_frameCommandLists.push_back(commandList);
        return commandList.get();
    }

    CommandQueueContext& Device::GetCommandQueueContext()
    {
        return m_commandQueueContext;
    }

    void Device::FillFormatsCapabilitiesInternal(FormatCapabilitiesList& formatsCapabilities)
    {
        formatsCapabilities.fill(static_cast<RHI::FormatCapabilities>(~0));
    }

    RHI::ResultCode Device::InitializeLimits()
    {
        constexpr uint32_t maxValue = static_cast<uint32_t>(-1);

        m_limits.m_maxImageDimension1D = maxValue;
        m_limits.m_maxImageDimension2D = maxValue;
        m_limits.m_maxImageDimension3D = maxValue;
        m_limits.m_maxImageDimensionCube = maxValue;
        m_limits.m_maxImageArraySize = maxValue;
        m_limits.m_minConstantBufferViewOffset = RHI::Alignment::Constant;
        m_limits.m_maxIndirectDrawCount = maxValue;
        m_limits.m_maxIndirectDispatchCount = maxValue;
        m_limits.m_maxConstantBufferSize = maxValue;
        m_limits.m_maxBufferSize = maxValue;

        m_features.m_resourceAliasing = false;

        m_commandQueueContext.Init(*this);

        return RHI::ResultCode::Success;
    }

    void Device::PreShutdown()
    {
        m_commandQueueContext.Shutdown();
    }

    void Device::ObjectCollectionNotify(RHI::ObjectCollectorNotifyFunction notifyFunction)
    {
        notifyFunction();
    }

    RHI::ShadingRateImageValue Device::ConvertShadingRate([[maybe_unused]] RHI::ShadingRate rate) const
    {
        return RHI::ShadingRateImageValue{};
    }

    void Device::DeviceLostCallback([[maybe_unused]] const wgpu::Device& wgpuDevice, wgpu::DeviceLostReason reason, const char* message)
    {
        AZ_Error("WebGPU", false, "Device lost because of %s : %s", ToString(reason), message);
    }

    void Device::ErrorCallback([[maybe_unused]] const wgpu::Device& wgpuDevice, wgpu::ErrorType type, const char* message)
    {
        AZ_Error("WebGPU", false, "Device error of type %s : %s", ToString(type), message);
    }
}
