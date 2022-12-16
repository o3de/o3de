/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI/DevicePipelineLibrary.h>
#include <Atom/RHI/MultiDeviceObject.h>

#include <Atom/RHI.Reflect/PipelineLibraryData.h>

namespace AZ
{
    namespace RHI
    {
        struct PipelineLibraryDescriptor
        {
            // A map of DevicePipelineLibraryDescriptor for each device where available.
            AZStd::unordered_map<int, DevicePipelineLibraryDescriptor> m_devicePipelineLibraryDescriptors;
        };

        //! PipelineState initialization is an expensive operation on certain platforms. If multiple pipeline states
        //! are created with little variation between them, the contents are still duplicated. This class is an allocation
        //! context for pipeline states, provided at PipelineState::Init, which will perform de-duplication of
        //! internal pipeline state components and cache the results.
        //!
        //! Practically speaking, if many pipeline states are created with shared data between them (e.g. permutations
        //! of the same shader), then providing a PipelineLibrary instance will reduce the memory footprint and cost
        //! of compilation.
        //!
        //! Additionally, the PipelineLibrary is able to serialize the internal driver-contents to and from an opaque
        //! data blob. This enables building up a pipeline state cache on disk, which can dramatically reduce pipeline
        //! state compilation cost when run from a pre-warmed cache.
        //!
        //! PipelineLibrary is thread-safe, in the sense that it will take a lock during compilation. It is possible
        //! to initialize pipeline states across threads using the same PipelineLibrary instance, but this will
        //! result in the two calls serializing on the mutex. Instead, see PipelineStateCache which stores
        //! a PipelineLibrary instance per thread to avoid this contention.
        class PipelineLibrary : public MultiDeviceObject
        {
        public:
            AZ_CLASS_ALLOCATOR(PipelineLibrary, AZ::SystemAllocator, 0);
            AZ_RTTI(PipelineLibrary, "{805513DE-9851-43EB-8E7C-15286D18C844}");
            PipelineLibrary() = default;
            virtual ~PipelineLibrary() = default;

            inline Ptr<DevicePipelineLibrary> GetDevicePipelineLibrary(int deviceIndex) const
            {
                AZ_Assert(
                    m_devicePipelineLibraries.find(deviceIndex) != m_devicePipelineLibraries.end(),
                    "No DevicePipelineLibrary found for device index %d\n",
                    deviceIndex);
                return m_devicePipelineLibraries.at(deviceIndex);
            }

            //! Initializes the pipeline library from a platform-specific data payload. This data is generated
            //! by calling GetSerializedData in a previous run of the application. When run for the first
            //! time, the serialized data should be empty. When the application completes, the library can be
            //! serialized and the contents saved to disk. Subsequent loads will experience much faster pipeline
            //! state creation times (on supported platforms). On success, the library is transitioned to the
            //! initialized state. On failure, the library remains uninitialized.
            //! @param descriptor The descriptor needed to init the PipelineLibrary.
            ResultCode Init(DeviceMask deviceMask, const PipelineLibraryDescriptor& descriptor);

            //! Merges the contents of other libraries into this library. This method must be called
            //! on an initialized library. A common use case for this method is to construct thread-local
            //! libraries and merge them into a single unified library. The serialized data can then be
            //! extracted from the unified library. An error code is returned on failure and the behavior
            //! is as if the method was never called.
            ResultCode MergeInto(AZStd::span<const PipelineLibrary* const> librariesToMerge);

            //! Serializes the platform-specific data and returns it as a new PipelineLibraryData instance.
            //! The data is opaque to the user and can only be used to re-initialize the library. Use
            //! this method to extract serialized data prior to application shutdown, save it to disk, and
            //! use it when initializing on subsequent runs.
            ConstPtr<PipelineLibraryData> GetSerializedData() const;
            
            //! Saves the platform-specific data to disk using the filePath provided. This is done through RHI backend drivers.
            bool SaveSerializedData(const AZStd::string& filePath) const;

            //! Returns whether the current library need to be merged
            bool IsMergeRequired() const;

        private:
            bool ValidateIsInitialized() const;

            // Explicit shutdown is not allowed for this type.
            void Shutdown() override final;

            AZStd::unordered_map<int, Ptr<DevicePipelineLibrary>> m_devicePipelineLibraries;
        };
    }
}
