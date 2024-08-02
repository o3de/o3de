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
#if defined(CARBONATED)
            m_FrameTimeLock.lock();
            
            // calculate pervious frame GPU time if it is ready, because we do not calculate it on command buffer completion
            if (m_frameCommandIndex >= 0)
            {
                FrameCommands& fc = m_frameCommands[m_frameCommandIndex];
                if (fc.m_commands.size() == 0)  // if all the buffers are completed
                {
                    m_FrameGPUTime = fc.CalculateTime();
                    m_FrameGPUSumTime = fc.m_sumTime;
                    m_FrameGPUWaitTime = fc.m_waitTime;
                    m_FrameGPUWaitAvgTime = fc.m_waitTime / fc.m_numBuffers;
                    m_FrameGPUEndMaxTime = fc.m_endMaxTime;
                    /*{
                        const double t = double(clock_gettime_nsec_np(CLOCK_UPTIME_RAW)) / 1000000000.0;
                        AZ_Info("GPUtime", "Get GPU data at %f on frame CHANGE for frame %u as %f / %f", t, fc.m_frameNumber, m_FrameGPUTime, m_FrameGPUSumTime);
                    }*/
                }
            }

            m_frameCounter++;
            m_frameCommandIndex = (m_frameCommandIndex + 1) % m_frameCommands.array_size;
            m_frameCommands[m_frameCommandIndex].Init(m_frameCounter);
            /*{
                const double t = double(clock_gettime_nsec_np(CLOCK_UPTIME_RAW)) / 1000000000.0;
                AZ_Info("GPUtime", "begin frame at %f, frame num %u", t, m_frameCounter);
            }*/
            m_FrameTimeLock.unlock();
#endif
            return BeginFrameInternal();
        }
        return ResultCode::InvalidOperation;
    }

#if defined(CARBONATED)
    void Device::RegisterCommandBuffer(const void* buffer)
    {
        if (!m_statsEnabled)
        {
            return;
        }

        m_FrameTimeLock.lock();
        
        if (m_frameCommandIndex >= 0)  // if frames started
        {
            m_frameCommands[m_frameCommandIndex].RegisterCommandBuffer(buffer);
            /*{
                const double t = double(clock_gettime_nsec_np(CLOCK_UPTIME_RAW)) / 1000000000.0;
                AZ_Info("GPUtime", "Register command buffer %p for frame %u at %f", buffer, m_frameCommands[m_frameCommandIndex].m_frameNumber, t);
            }*/
        }
        m_FrameTimeLock.unlock();
    }

    void Device::MarkCommandBufferCommit(const void* buffer)
    {
        if (!m_statsEnabled)
        {
            return;
        }

        m_FrameTimeLock.lock();
        
        if (m_frameCommandIndex >= 0)  // if frames started
        {
            bool found = false;
            for (int i = m_frameCommandIndex, j = 0; j < m_frameCommands.array_size; j++, i = (i + m_frameCommands.array_size - 1) % m_frameCommands.array_size)
            {
                FrameCommands& fc = m_frameCommands[i];
                for (int ib = 0; ib < fc.m_commands.size(); ib++)
                {
                    if (buffer == fc.m_commands[ib].m_buffer)
                    {
                        found = true;

                        fc.m_commands[ib].m_commitTime = TimeInterval::GetTimeSec();
                        break;
                    }
                }
                if (found)
                {
                    break;
                }
            }
            if (!found)
            {
                AZ_Error("GPUtime", false, "Command buffer %p is not found to commit", buffer);
            }
        }
                
        m_FrameTimeLock.unlock();
    }

    void Device::CommandBufferCompleted(const void* buffer, double begin, double end)
    {
        if (!m_statsEnabled)
        {
            return;
        }

        m_FrameTimeLock.lock();
        
        if (m_frameCommandIndex >= 0)  // if frames started
        {
            bool found = false;
            for (int i = m_frameCommandIndex, j = 0; j < m_frameCommands.array_size; j++, i = (i + m_frameCommands.array_size - 1) % m_frameCommands.array_size)
            {
                FrameCommands& fc = m_frameCommands[i];
                for (int ib = 0; ib < fc.m_commands.size(); ib++)
                {
                    if (buffer == fc.m_commands[ib].m_buffer)
                    {
                        found = true;
                        if (end - begin > 0.0000005)  // there are zero execution time buffers, which we ignore
                        {
                            //AZ_Info("GPUtime", "Add interval %f %f (%f) to frame %u", begin, end, end - begin, fc.m_frameNumber);
                            fc.RegisterInterval(fc.m_commands[ib].m_commitTime, begin, end);
                        }
                        fc.m_commands.erase(fc.m_commands.begin() + ib);  // the same address can be used in another frame
                        if (fc.m_commands.size() == 0)  // if all the buffers are completed
                        {
                            if (i != m_frameCommandIndex)   // do not calculate time for the current frame yet because there might be more buffers created later
                            {
                                m_FrameGPUTime = fc.CalculateTime();
                                m_FrameGPUSumTime = fc.m_sumTime;
                                m_FrameGPUWaitTime = fc.m_waitTime;
                                m_FrameGPUWaitAvgTime = fc.m_waitTime / fc.m_numBuffers;
                                m_FrameGPUEndMaxTime = fc.m_endMaxTime;
                                /*{
                                    const double t = double(clock_gettime_nsec_np(CLOCK_UPTIME_RAW)) / 1000000000.0;
                                    AZ_Info("GPUtime", "Get GPU data at %f for frame %u as %f / %f", t, fc.m_frameNumber, m_FrameGPUTime, m_FrameGPUSumTime);
                                }*/
                            }
                            /*else
                            {
                                const double t = double(clock_gettime_nsec_np(CLOCK_UPTIME_RAW)) / 1000000000.0;
                                AZ_Info("GPUtime", "Do NOT get GPU data at %f for frame %u because frame is likely incomplete", t, fc.m_frameNumber);
                            }*/
                        }
                        break;
                    }
                    if (found)
                    {
                        break;
                    }
                }
            }
            if (!found)
            {
                AZ_Error("GPUtime", false, "Command buffer %p is not found to complete", buffer);
            }
        }
        
        m_FrameTimeLock.unlock();
    }

    double Device::GetGPUFrameTime()
    {
        if (!m_statsEnabled)
        {
            return 0.0;
        }
        m_FrameTimeLock.lock();
        const double result = m_FrameGPUTime;
        m_FrameTimeLock.unlock();
        return result;
    }
    double Device::GetGPUSumFrameTime()
    {
        if (!m_statsEnabled)
        {
            return 0.0;
        }
        m_FrameTimeLock.lock();
        const double result = m_FrameGPUSumTime;
        m_FrameTimeLock.unlock();
        return result;
    }
    double Device::GetGPUWaitFrameTime()
    {
        if (!m_statsEnabled)
        {
            return 0.0;
        }
        m_FrameTimeLock.lock();
        const double result = m_FrameGPUWaitTime;
        m_FrameTimeLock.unlock();
        return result;
    }
    double Device::GetGPUWaitAvgFrameTime()
    {
        if (!m_statsEnabled)
        {
            return 0.0;
        }
        m_FrameTimeLock.lock();
        const double result = m_FrameGPUWaitAvgTime;
        m_FrameTimeLock.unlock();
        return result;
    }
    double Device::GetGPUEndMaxFrameTime()
    {
        if (!m_statsEnabled)
        {
            return 0.0;
        }
        m_FrameTimeLock.lock();
        const double result = m_FrameGPUEndMaxTime;
        m_FrameTimeLock.unlock();
        return result;
    }
    bool Device::GetFrameCommandMetrics(FrameCommandMetrics& frameCommandMetrics)
    {
        const int frameIndex = m_frameCommandIndex;
        frameCommandMetrics.Init();
        if (frameIndex < 0 || frameIndex >= 4)
        {
            return false;
        }
        m_FrameTimeLock.lock();
        const FrameCommands& frameCommand = m_frameCommands[frameIndex];
        for (int iv = 0; iv < frameCommand.m_intervals.size(); iv++)
        {
            FrameCommandMetrics::FrameInterval interval;
            interval.m_begin = frameCommand.m_intervals[iv].m_begin;
            interval.m_end = frameCommand.m_intervals[iv].m_end;
            frameCommandMetrics.m_intervals.push_back(interval);
        }
        for (int iv = 0; iv < frameCommand.m_rawIntervals.size(); iv++)
        {
            FrameCommandMetrics::FrameInterval rawInterval;
            rawInterval.m_begin = frameCommand.m_rawIntervals[iv].m_begin;
            rawInterval.m_end = frameCommand.m_rawIntervals[iv].m_end;
            frameCommandMetrics.m_rawIntervals.push_back(rawInterval);
        }
        m_FrameTimeLock.unlock();
        return true;
    }
    void Device::EnableGatheringStats()
    {
        m_statsEnabled = true;  // assume all the variables are reset in the constructor or via DisableGatheringStats
    }
    void Device::DisableGatheringStats()
    {
        if (!m_statsEnabled)
        {
            return;
        }
        m_statsEnabled = false;
        
        // reset all the variables
        m_frameCounter = 0;
        m_frameCommandIndex = -1;
        m_FrameGPUTime = 0.0;
        m_FrameGPUSumTime = 0.0;
        m_FrameGPUWaitTime = 0.0;
        m_FrameGPUWaitAvgTime = 0.0;
        m_FrameGPUEndMaxTime = 0.0;
    }
#endif

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
