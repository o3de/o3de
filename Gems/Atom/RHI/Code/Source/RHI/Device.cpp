/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Device.h>
#include <Atom/RHI/MemoryStatisticsBus.h>
#include <Atom/RHI/RHISystem.h>
#include <Atom/RHI/RHIUtils.h>

#include <AzCore/std/sort.h>

namespace AZ::RHI
{
    bool Device::IsInitialized() const
    {
        return m_physicalDevice != nullptr;
    }

    bool Device::ValidateIsInitialized() const
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized() == false)
            {
                AZ_Error("Device", false, "Device is not initialized. This operation is invalid on an uninitialized device.");
                return false;
            }
        }
        return true;
    }

    bool Device::ValidateIsInFrame() const
    {
        if (Validation::IsEnabled())
        {
            if (m_isInFrame == false)
            {
                AZ_Error("Device", false, "Device is not currently processing a frame. This operation is only allowed within a BeginFrame / EndFrame scope.");
                return false;
            }
        }
        return true;
    }

    bool Device::ValidateIsNotInFrame() const
    {
        if (Validation::IsEnabled())
        {
            if (m_isInFrame)
            {
                AZ_Error("Device", false, "Device is currently processing a frame. This operation is only allowed outside of a BeginFrame / EndFrame scope.");
                return false;
            }
        }
        return true;
    }

    ResultCode Device::Init(int deviceIndex, PhysicalDevice& physicalDevice)
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized())
            {
                AZ_Error("Device", false, "Device is already initialized.");
                return ResultCode::InvalidOperation;
            }
        }

        m_formatsCapabilities.fill(FormatCapabilities::None);
        m_nearestSupportedFormats.fill(Format::Unknown);
            
        m_physicalDevice = &physicalDevice;
        m_deviceIndex = deviceIndex;

        RHI::ResultCode resultCode = InitInternal(physicalDevice);

        if (resultCode == ResultCode::Success)
        {
            FillFormatsCapabilitiesInternal(m_formatsCapabilities);

            // Fill supported format mapping for depth formats
            CalculateDepthStencilNearestSupportedFormats();

            // Any other supported format mapping should go here

            // Assume all formats that haven't been mapped yet are supported and map to themselves
            FillRemainingSupportedFormats();

            // Initialize limits and resources that are associated with them
            resultCode = InitializeLimits();
        }
        else
        {
            m_physicalDevice = nullptr;
        }

        return resultCode;
    }

    void Device::Shutdown()
    {
        if (IsInitialized())
        {
            ShutdownInternal();
            m_physicalDevice = nullptr;
        }
    }

    bool Device::WasDeviceRemoved()
    {
        return m_wasDeviceRemoved;
    }

    void Device::SetDeviceRemoved()
    {
        m_wasDeviceRemoved = true;

        // set notification
        RHISystemNotificationBus::Broadcast(&RHISystemNotificationBus::Events::OnDeviceRemoved, this);
    }

    ResultCode Device::BeginFrame()
    {
        AZ_PROFILE_FUNCTION(RHI);

        if (ValidateIsInitialized() && ValidateIsNotInFrame())
        {
            m_isInFrame = true;
            return BeginFrameInternal();
        }
        return ResultCode::InvalidOperation;
    }

    ResultCode Device::EndFrame()
    {
        if (ValidateIsInitialized() && ValidateIsInFrame())
        {
            AZ_PROFILE_SCOPE(RHI, "Device: EndFrame");
            EndFrameInternal();
            m_isInFrame = false;
            return ResultCode::Success;
        }
        return ResultCode::InvalidOperation;
    }

    ResultCode Device::WaitForIdle()
    {
        if (ValidateIsInitialized() && ValidateIsNotInFrame())
        {
            WaitForIdleInternal();
            return ResultCode::Success;
        }
        return ResultCode::InvalidOperation;
    }

    ResultCode Device::CompileMemoryStatistics(MemoryStatistics& memoryStatistics, MemoryStatisticsReportFlags reportFlags)
    {
        if (ValidateIsInitialized() && ValidateIsNotInFrame())
        {
            AZ_PROFILE_SCOPE(RHI, "Device: CompileMemoryStatistics");
            MemoryStatisticsBuilder builder;
            builder.Begin(memoryStatistics, reportFlags);
            CompileMemoryStatisticsInternal(builder);
            MemoryStatisticsEventBus::Event(this, &MemoryStatisticsEventBus::Events::ReportMemoryUsage, builder);
            builder.End();
            return ResultCode::Success;
        }
        return ResultCode::InvalidOperation;
    }

    ResultCode Device::UpdateCpuTimingStatistics() const
    {
        if (ValidateIsNotInFrame())
        {
            UpdateCpuTimingStatisticsInternal();
            return ResultCode::Success;
        }
        return ResultCode::InvalidOperation;
    }

    const PhysicalDevice& Device::GetPhysicalDevice() const
    {
        return *m_physicalDevice;
    }

    int Device::GetDeviceIndex() const
    {
        return m_deviceIndex;
    }

    const DeviceDescriptor& Device::GetDescriptor() const
    {
        return m_descriptor;
    }

    const DeviceFeatures& Device::GetFeatures() const
    {
        return m_features;
    }

    const DeviceLimits& Device::GetLimits() const
    {
        return m_limits;
    }

    const ResourcePoolDatabase& Device::GetResourcePoolDatabase() const
    {
        return m_resourcePoolDatabase;
    }

    ResourcePoolDatabase& Device::GetResourcePoolDatabase()
    {
        return m_resourcePoolDatabase;
    }

    FormatCapabilities Device::GetFormatCapabilities(Format format) const
    {
        return m_formatsCapabilities[static_cast<uint32_t>(format)];
    }        
      
    // Return the nearest supported format for this device.     
    Format Device::GetNearestSupportedFormat(Format requestedFormat, FormatCapabilities requestedCapabilities) const
    {
        Format nearestFormat = m_nearestSupportedFormats[aznumeric_caster(requestedFormat)];
        if (nearestFormat == Format::Unknown)
        {
            AZ_Assert(false, "The requested format [%s] has not been added to the mapping of device supported formats.", ToString(requestedFormat));
            return Format::Unknown;
        }            

        if (!RHI::CheckBitsAll(GetFormatCapabilities(nearestFormat), requestedCapabilities))
        {
            AZ_Assert(false, "The nearest found format [%s] does not support the requested format capabilities.", ToString(nearestFormat));
            return Format::Unknown;
        }

        return nearestFormat;
    }

    AZStd::vector<Format> Device::GetValidSwapChainImageFormats([[maybe_unused]] const WindowHandle& windowHandle) const
    {
        // [GFX TODO][ATOM-1125] Implement this method for every platform.  
        // After it, make this method pure virtual.
        return AZStd::vector<Format>{Format::R8G8B8A8_UNORM};
    }

    void Device::CalculateDepthStencilNearestSupportedFormats()
    {
        auto fillNearestFunc = [this](const AZStd::vector<Format>& formats)
        {
            for (int i = 0; i < formats.size(); ++i)
            {
                // We search for formats that have at least the same bit depth.
                for (int j = i; j < formats.size(); ++j)
                {
                    if (RHI::CheckBitsAll(GetFormatCapabilities(formats[j]), FormatCapabilities::DepthStencil))
                    {
                        m_nearestSupportedFormats[aznumeric_caster(formats[i])] = formats[j];
                        break;
                    }
                }

                // Check if we found the nearest format. If not, then look for the closer format
                // that may have less bit depth.
                if (m_nearestSupportedFormats[aznumeric_caster(formats[i])] == Format::Unknown)
                {
                    for (int j = i - 1; j >=0; --j)
                    {
                        if (RHI::CheckBitsAll(GetFormatCapabilities(formats[j]), FormatCapabilities::DepthStencil))
                        {
                            m_nearestSupportedFormats[aznumeric_caster(formats[i])] = formats[j];
                            break;
                        }
                    }
                }
            }
        };

        const AZStd::vector<Format> depthFormats =
        {
            Format::D16_UNORM,
            Format::D32_FLOAT
        };

        const AZStd::vector<Format> depthStencilFormats =
        {
            Format::D16_UNORM_S8_UINT,
            Format::D24_UNORM_S8_UINT,
            Format::D32_FLOAT_S8X24_UINT
        };

        fillNearestFunc(depthFormats);
        fillNearestFunc(depthStencilFormats);
    }

    void Device::FillRemainingSupportedFormats()
    {
        size_t formatCount = static_cast<size_t>(Format::Count);

        for (size_t i = 0; i < formatCount; ++i)
        {
            if (m_nearestSupportedFormats[i] == Format::Unknown)
            {
                m_nearestSupportedFormats[i] = static_cast<Format>(i);
            }
        }
    }

    void Device::SetLastExecutingScope(const AZStd::string_view scopeName)
    {
        m_lastExecutingScope = scopeName;
    }

    AZStd::string_view Device::GetLastExecutingScope() const
    {
        return m_lastExecutingScope;
    }

    ResultCode Device::InitBindlessSrg(RHI::Ptr<RHI::ShaderResourceGroupLayout> bindlessSrgLayout)
    {
        BindlessSrgDescriptor bindlessSrgDesc;
        bindlessSrgDesc.m_bindlesSrgBindingSlot = bindlessSrgLayout->GetBindingSlot();

        // Cache indices associated with each bindless resource type. But first check if Unbounded arrays are supported
        bool isUnboundedArraySupported = GetFeatures().m_unboundedArrays;
        if(isUnboundedArraySupported)
        {
            for (const RHI::ShaderInputImageUnboundedArrayDescriptor& shaderInputImageUnboundedArray : bindlessSrgLayout->GetShaderInputListForImageUnboundedArrays())
            {
                if (strstr(shaderInputImageUnboundedArray.m_name.GetCStr(), ToString(BindlessResourceType::m_Texture2D).data()))
                {
                    bindlessSrgDesc.m_roTextureIndex = shaderInputImageUnboundedArray.m_registerId;
                }
                else if (strstr(shaderInputImageUnboundedArray.m_name.GetCStr(), ToString(BindlessResourceType::m_RWTexture2D).data()))
                {
                    bindlessSrgDesc.m_rwTextureIndex = shaderInputImageUnboundedArray.m_registerId;
                }
                else if (strstr(shaderInputImageUnboundedArray.m_name.GetCStr(), ToString(BindlessResourceType::m_TextureCube).data()))
                {
                    bindlessSrgDesc.m_roTextureCubeIndex = shaderInputImageUnboundedArray.m_registerId;
                }
            }
                
            for (const RHI::ShaderInputBufferUnboundedArrayDescriptor& shaderInputImageUnboundedArray : bindlessSrgLayout->GetShaderInputListForBufferUnboundedArrays())
            {
                if (strstr(shaderInputImageUnboundedArray.m_name.GetCStr(), ToString(BindlessResourceType::m_ByteAddressBuffer).data()))
                {
                    bindlessSrgDesc.m_roBufferIndex = shaderInputImageUnboundedArray.m_registerId;
                }
                else if (strstr(shaderInputImageUnboundedArray.m_name.GetCStr(), ToString(BindlessResourceType::m_RWByteAddressBuffer).data()))
                {
                    bindlessSrgDesc.m_rwBufferIndex = shaderInputImageUnboundedArray.m_registerId;
                }
            }
            AZ_Assert(
                (bindlessSrgLayout->GetGroupSizeForImageUnboundedArrays() + bindlessSrgLayout->GetGroupSizeForBufferUnboundedArrays()) == static_cast<uint32_t>(BindlessResourceType::Count),
                "Number of resource types supported in the shader mismatches with the BindlessResourceType enum");
        }

        bool isSimulateBindlessUASupported = GetFeatures().m_simulateBindlessUA;
        // Check to see if a RHI back-end simulated unbounded arrays for Bindless SRG
        if (isSimulateBindlessUASupported)
        {
            //If Unbounded array support is not present we can simulate it via bounding the array. This is currently the case for metal backend.
            for (const RHI::ShaderInputImageDescriptor& shaderInputImageUnboundedArray : bindlessSrgLayout->GetShaderInputListForImages())
            {
                if (strstr(shaderInputImageUnboundedArray.m_name.GetCStr(), ToString(BindlessResourceType::m_Texture2D).data()))
                {
                    bindlessSrgDesc.m_roTextureIndex = shaderInputImageUnboundedArray.m_registerId / RHI::Limits::Pipeline::UnboundedArraySize;
                }
                else if (strstr(shaderInputImageUnboundedArray.m_name.GetCStr(), ToString(BindlessResourceType::m_RWTexture2D).data()))
                {
                    bindlessSrgDesc.m_rwTextureIndex = shaderInputImageUnboundedArray.m_registerId / RHI::Limits::Pipeline::UnboundedArraySize;
                }
                else if (strstr(shaderInputImageUnboundedArray.m_name.GetCStr(), ToString(BindlessResourceType::m_TextureCube).data()))
                {
                    bindlessSrgDesc.m_roTextureCubeIndex = shaderInputImageUnboundedArray.m_registerId / RHI::Limits::Pipeline::UnboundedArraySize;
                }
                AZ_Assert(shaderInputImageUnboundedArray.m_count == RHI::Limits::Pipeline::UnboundedArraySize, "The array size needs to match the limit");
            }
                
            for (const RHI::ShaderInputBufferDescriptor& shaderInputBufferUnboundedArray : bindlessSrgLayout->GetShaderInputListForBuffers())
            {
                if (strstr(shaderInputBufferUnboundedArray.m_name.GetCStr(), ToString(BindlessResourceType::m_ByteAddressBuffer).data()))
                {
                    bindlessSrgDesc.m_roBufferIndex = shaderInputBufferUnboundedArray.m_registerId / RHI::Limits::Pipeline::UnboundedArraySize;
                }
                else if (strstr(shaderInputBufferUnboundedArray.m_name.GetCStr(), ToString(BindlessResourceType::m_RWByteAddressBuffer).data()))
                {
                    bindlessSrgDesc.m_rwBufferIndex = shaderInputBufferUnboundedArray.m_registerId / RHI::Limits::Pipeline::UnboundedArraySize;
                }
                AZ_Assert(shaderInputBufferUnboundedArray.m_count == RHI::Limits::Pipeline::UnboundedArraySize, "The array size needs to match the limit");
            }
        }

        if (isUnboundedArraySupported || isSimulateBindlessUASupported)
        {
            AZ_Assert(bindlessSrgDesc.m_roTextureIndex != AZ::RHI::InvalidIndex, "Invalid register id index for bindless read only textures");
            AZ_Assert(bindlessSrgDesc.m_rwTextureIndex != AZ::RHI::InvalidIndex, "Invalid register id index for bindless read write textures");
            AZ_Assert(bindlessSrgDesc.m_roTextureCubeIndex != AZ::RHI::InvalidIndex, "Invalid register id index for bindless read only cube textures");
            AZ_Assert(bindlessSrgDesc.m_roBufferIndex != AZ::RHI::InvalidIndex, "Invalid register id index for bindless read only buffers");
            AZ_Assert(bindlessSrgDesc.m_rwBufferIndex != AZ::RHI::InvalidIndex, "Invalid register id index for bindless read write buffers");
            AZ_Assert(bindlessSrgDesc.m_bindlesSrgBindingSlot != AZ::RHI::InvalidIndex, "Invalid binding slot id for bindless srg");
            return InitInternalBindlessSrg(bindlessSrgDesc);
        }

        return ResultCode::Success;
    }
}
