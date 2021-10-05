/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/ObjectCollector.h>
#include <Atom/RHI.Reflect/DeviceDescriptor.h>
#include <Atom/RHI.Reflect/DeviceFeatures.h>
#include <Atom/RHI.Reflect/DeviceLimits.h>
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/MemoryEnums.h>
#include <Atom/RHI.Reflect/SwapChainDescriptor.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <Atom/RHI/PhysicalDevice.h>
#include <Atom/RHI/ResourcePoolDatabase.h>

#include <AzCore/std/chrono/types.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    namespace RHI
    {
        //! The Device is a context for managing GPU state and memory on a physical device. The user creates
        //! a device instance from a PhysicalDevice. Each device has its own capabilities and limits, and can
        //! be configured to buffer a specific number of frames.
        //!
        //! Certain objects in the RHI associate to a single device. This includes resource pools (and their
        //! associated resources), pipeline states, and frame scheduler support classes. It is valid to create
        //! multiple device instances, but they currently don't have a way to interact with each other. It is
        //! invalid to use an object associated with one device in a rendering operation associated
        //! with another device.
        //!
        //! Multi-Device interop support is planned in the future and will likely require API changes.
        class Device
            : public Object
        {
        public:
            AZ_RTTI(Device, "{C7E70BE4-3AA5-4214-91E6-52A8ECC31A34}", Object);
            virtual ~Device() = default;
            
            //! Returns whether the device is initialized.
            bool IsInitialized() const;
            
            //! Initializes just the native device using the provided physical device. The
            //! device must be initialized before it can be used. Explicit shutdown is not exposed
            //! due to the number of dependencies. Instead, the device is reference counted by child
            //! objects.
            //!
            //! If initialization fails. The device is left in an uninitialized state (as if Init had never
            //! been called), and an error code is returned.
            ResultCode Init(PhysicalDevice& physicalDevice);
            
            //! Begins execution of a frame. The device internally manages a set of command queues. This
            //! method will synchronize the CPU with the GPU according to the number of in-light frames
            //! configured on the device. This means you should make sure any manipulation of N-buffered
            //! CPU data read by the GPU occurs *after* this method. This method must be called prior
            //! to FrameGraph execution.
            //!
            //! If the call fails, an error code is returned and the device is left in a state as if
            //! the call had never occurred.
            ResultCode BeginFrame();

            //! Ends execution of a frame. This flushes all CPU state to the GPU. All FrameGraphExecuter execution
            //! phases must be complete before calling this method. If the call fails, an error code is returned and
            //! the device is left in a state as if the call had never occurred.
            ResultCode EndFrame();

            //! Flushes all GPU work and waits for idle on the CPU. Naturally, this is a synchronous command and
            //! will block the CPU for an extended amount of time. However, it may be necessary in certain
            //! circumstances where memory must be reclaimed immediately. For example, any resource released
            //! on a pool is deferred until the GPU has completed processing of the frame. This method will
            //! ensure that all memory is reclaimed.
            //!
            //! An important constraint on this method is that it cannot be called during execution of a
            //! frame (i.e. within the BeginFrame / EndFrame scope). Attempting to do so will return
            //! ResultCode::InvalidOperation.
            ResultCode WaitForIdle();

            //! Fills the provided data structure with memory usage statistics specific to this device. This
            //! method can only be called on an initialized device, and outside of the BeginFrame / EndFrame
            //! scope. Otherwise, an error code is returned.
            ResultCode CompileMemoryStatistics(MemoryStatistics& memoryStatistics, MemoryStatisticsReportFlags reportFlags);

            //! Pushes internally recorded timing statistics upwards into the global stats profiler, under the RHI section.
            //! This method can only be called on an initialized device, and outside of the BeginFrame / EndFrame scope.
            //! Otherwise, an error code is returned.
            ResultCode UpdateCpuTimingStatistics() const;

            //! Returns the physical device associated with this device.
            const PhysicalDevice& GetPhysicalDevice() const;

            //! Returns the descriptor associated with the device.
            const DeviceDescriptor& GetDescriptor() const;

            //! Returns the set of features supported by this device.
            const DeviceFeatures& GetFeatures() const;

            //! Returns the set of hardware limits for this device.
            const DeviceLimits& GetLimits() const;

            //! Returns the resource pool database.
            const ResourcePoolDatabase& GetResourcePoolDatabase() const;

            //! Returns the mutable resource pool database.
            ResourcePoolDatabase& GetResourcePoolDatabase();

            //! Returns a union of all capabilities of a specific format.
            FormatCapabilities GetFormatCapabilities(Format format) const;

            //! Return the nearest supported format for this device.
            Format GetNearestSupportedFormat(Format requestedFormat, FormatCapabilities requestedCapabilities) const;

            //! Small API to support getting supported/working swapchain formats for a window.
            //! Returns the set of supported formats for swapchain images.
            virtual AZStd::vector<Format> GetValidSwapChainImageFormats(const WindowHandle& windowHandle) const;

            //! Converts a GPU timestamp to microseconds
            virtual AZStd::chrono::microseconds GpuTimestampToMicroseconds(uint64_t gpuTimestamp, HardwareQueueClass queueClass) const = 0;

            //! Called before the device is going to be shutdown. This lets the device release any resources
            //! that also hold on to a Ptr to Device.
            virtual void PreShutdown() = 0;

            //! Get the memory requirements for allocating an image resource.
            virtual ResourceMemoryRequirements GetResourceMemoryRequirements(const ImageDescriptor& descriptor) = 0;
            //! Get the memory requirements for allocating a buffer resource.
            virtual ResourceMemoryRequirements GetResourceMemoryRequirements(const BufferDescriptor& descriptor) = 0;

            //! Notifies after all objects currently in the platform release queue are released
            virtual void ObjectCollectionNotify(RHI::ObjectCollectorNotifyFunction notifyFunction) = 0;

            //! Allows the back-ends to compact SRG related memory if applicable
            virtual RHI::ResultCode CompactSRGMemory()
            {
                return RHI::ResultCode::Success;
            };

        protected:
            DeviceFeatures m_features;
            DeviceLimits m_limits;
            ResourcePoolDatabase m_resourcePoolDatabase;

            DeviceDescriptor m_descriptor;

            using FormatCapabilitiesList = AZStd::array<FormatCapabilities, static_cast<uint32_t>(Format::Count)>;

        private:
            ///////////////////////////////////////////////////////////////////
            // RHI::Object
            void Shutdown() override final;
            ///////////////////////////////////////////////////////////////////

            bool ValidateIsInitialized() const;
            bool ValidateIsInFrame() const;
            bool ValidateIsNotInFrame() const;

            ///////////////////////////////////////////////////////////////////
            // Platform API

            //! Called when just the device is being initialized.
            virtual ResultCode InitInternal(PhysicalDevice& physicalDevice) = 0;

            //! Called when the device is being shutdown.
            virtual void ShutdownInternal() = 0;

            //! Called when the device is beginning a frame for processing.
            virtual void BeginFrameInternal() = 0;

            //! Called when the device is ending a frame for processing.
            virtual void EndFrameInternal() = 0;

            //! Called when the device is flushing all GPU operations and waiting for idle.
            virtual void WaitForIdleInternal() = 0;

            //! Called when the device is reporting memory usage statistics.
            virtual void CompileMemoryStatisticsInternal(MemoryStatisticsBuilder& builder) = 0;

            //! Called when the device is reporting cpu timing statistics.
            virtual void UpdateCpuTimingStatisticsInternal() const = 0;

            //! Fills the capabilities for each format.
            virtual void FillFormatsCapabilitiesInternal(FormatCapabilitiesList& formatsCapabilities) = 0;

            //! Initialize limits and resources associated with them.
            virtual ResultCode InitializeLimits() = 0;
            ///////////////////////////////////////////////////////////////////

            void CalculateDepthStencilNearestSupportedFormats();

            //! Fills the remainder of nearest supported formats map so that formats that have not yet been set point to themselves
            //! All platform specific format mappings should be executed before this function is called
            void FillRemainingSupportedFormats();

            // The physical device backing this logical device instance.
            Ptr<PhysicalDevice> m_physicalDevice;

            // Tracks whether the device is in the BeginFrame / EndFrame scope.
            bool m_isInFrame = false;

            AZStd::array<Format, static_cast<uint32_t>(Format::Count)> m_nearestSupportedFormats;

            FormatCapabilitiesList m_formatsCapabilities;
        };
    }
}
