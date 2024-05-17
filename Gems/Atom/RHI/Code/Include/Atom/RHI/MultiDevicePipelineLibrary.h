/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI/MultiDeviceObject.h>
#include <Atom/RHI/SingleDevicePipelineLibrary.h>

#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI.Reflect/PipelineLibraryData.h>

namespace AZ::RHI
{
    //! A handle typed to the pipeline library. Used by the PipelineStateCache to abstract access.
    using MultiDevicePipelineLibraryHandle = Handle<uint32_t, class MultiDevicePipelineLibrary>;

    //! A descriptor struct containing a map of device-specific PipelineLibraryDescriptors, used in
    //! MultiDevicePipelineLibrary
    struct MultiDevicePipelineLibraryDescriptor
    {
        void Init(
            MultiDevice::DeviceMask deviceMask,
            const AZStd::unordered_map<int, ConstPtr<RHI::PipelineLibraryData>>& serializedData,
            const AZStd::unordered_map<int, AZStd::string>& filePaths)
        {
            int deviceCount = RHI::RHISystemInterface::Get()->GetDeviceCount();

            for (auto deviceIndex { 0 }; deviceIndex < deviceCount; ++deviceIndex)
            {
                if (CheckBit(AZStd::to_underlying(deviceMask), static_cast<u8>(deviceIndex)))
                {
                    m_devicePipelineLibraryDescriptors[deviceIndex] = {
                        serializedData.contains(deviceIndex) ? serializedData.at(deviceIndex) : nullptr,
                        filePaths.contains(deviceIndex) ? filePaths.at(deviceIndex) : AZStd::string("")
                    };
                }
            }
        }

        //! Returns the device-specific SingleDevicePipelineLibraryDescriptor for the given index
        inline SingleDevicePipelineLibraryDescriptor GetDevicePipelineLibraryDescriptor(int deviceIndex) const
        {
            AZ_Assert(
                m_devicePipelineLibraryDescriptors.find(deviceIndex) != m_devicePipelineLibraryDescriptors.end(),
                "No DevicePipelineLibraryDescriptor found for device index %d\n",
                deviceIndex);

            if (m_devicePipelineLibraryDescriptors.contains(deviceIndex))
            {
                return m_devicePipelineLibraryDescriptors.at(deviceIndex);
            }
            else
            {
                return SingleDevicePipelineLibraryDescriptor{};
            }
        }

        //! A map of all device-specific PipelineLibraryDescriptors, indexed by the device index
        AZStd::unordered_map<int, SingleDevicePipelineLibraryDescriptor> m_devicePipelineLibraryDescriptors;
    };

    //! MultiDevicePipelineLibrary is a multi-device class (representing a SingleDevicePipelineLibrary on multiple devices).
    //! It holds a map of device-specific SingleDevicePipelineLibrary objects, which can be addressed with a device index.
    //! The class is initialized with a device mask (1 bit per device), which initializes one SingleDevicePipelineLibrary
    //! for each bit set and stores them in a map.
    //! The API then forwards all calls to the all device-specific SingleDevicePipelineLibrary objects by iterating over them
    //! and forwarding the call.
    //! A device-specific SingleDevicePipelineLibrary can be accessed by calling GetDevicePipelineLibrary
    //! with the corresponding device index
    class MultiDevicePipelineLibrary : public MultiDeviceObject
    {
    public:
        AZ_CLASS_ALLOCATOR(MultiDevicePipelineLibrary, AZ::SystemAllocator, 0);
        AZ_RTTI(MultiDevicePipelineLibrary, "{B48B6A46-5976-4D7D-AA14-2179D871C567}");
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(PipelineLibrary);
        MultiDevicePipelineLibrary() = default;
        virtual ~MultiDevicePipelineLibrary() = default;

        //! For all devices selected via the deviceMask, a SingleDevicePipelineLibrary is initialized and stored internally
        //! in a map (mapping from device index to a device-specific SingleDevicePipelineLibrary).
        //! A device-specific descriptor (retrieved from MultiDevicePipelineLibraryDescriptor) is passed to the 
        //! respective initialized methods of the device-specific SingleDevicePipelineLibrary.
        //! @param deviceMask A bitmask selecting on which devices a SingleDevicePipelineLibrary should be initialized
        //! @param descriptor The descriptor needed to init the MultiDevicePipelineLibrary.
        ResultCode Init(MultiDevice::DeviceMask deviceMask, const MultiDevicePipelineLibraryDescriptor& descriptor);

        //! Forwards the call to all device-specific PipelineLibraries, for each device-specific SingleDevicePipelineLibrary,
        //! extracting the corresponding SingleDevicePipelineLibrary(ies) from librariesToMerge and passing them on.
        //! @param librariesToMerge A span of libraries to merge into this library
        ResultCode MergeInto(AZStd::span<const MultiDevicePipelineLibrary* const> librariesToMerge);

        //! Serializes the platform-specific data and returns it as a new PipelineLibraryData instance
        //! for a specific device 
        //! @param deviceIndex Denotes from which device the serialized data should be retrieved
        ConstPtr<PipelineLibraryData> GetSerializedData(int deviceIndex = RHI::MultiDevice::DefaultDeviceIndex) const
        {
            if (m_deviceObjects.contains(deviceIndex))
            {
                return GetDevicePipelineLibrary(deviceIndex)->GetSerializedData();
            }
            else
            {
                AZ_Error(
                    "MultiDevicePipelineLibrary",
                    false,
                    "MultiDevicePipelineLibrary is not initialized. This operation is only permitted on an initialized library.");
                return nullptr;
            }
        }

        //! Serializes the platform-specific data and returns it as a new PipelineLibraryData instance
        //! for a specific device
        //! @param deviceIndex Denotes from which device the serialized data should be retrieved
        auto GetSerializedDataMap() const
        {
            AZStd::unordered_map<int, ConstPtr<PipelineLibraryData>> serializedData;

            IterateObjects<SingleDevicePipelineLibrary>(
                [&serializedData]([[maybe_unused]] auto deviceIndex, auto devicePipelineLibrary)
                {
                    serializedData[deviceIndex] = devicePipelineLibrary->GetSerializedData();
                });

            return serializedData;
        }

        //! Saves the platform-specific data to disk using the device-specific filePath provided. This is done through RHI backend drivers
        //! for each device.
        bool SaveSerializedData(const AZStd::unordered_map<int, AZStd::string>& filePaths) const;

        //! Returns whether the current library need to be merged
        //! Returns true if any of the device-specific SingleDevicePipelineLibrary objects needs to be merged
        virtual bool IsMergeRequired() const;

    private:
        bool ValidateIsInitialized() const;

        //! Explicit shutdown is not allowed for this type.
        void Shutdown() override final;
    };
} // namespace AZ::RHI
