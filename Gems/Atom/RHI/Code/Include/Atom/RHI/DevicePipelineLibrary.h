/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI/DeviceObject.h>

#include <Atom/RHI.Reflect/PipelineLibraryData.h>

namespace AZ::RHI
{
    /// A handle typed to the pipeline library. Used by the PipelineStateCache to abstract access.
    using DevicePipelineLibraryHandle = Handle<uint32_t, class DevicePipelineLibrary>;

    struct DevicePipelineLibraryDescriptor
    {
        //Serialized data with which to init the DevicePipelineLibrary
        ConstPtr<PipelineLibraryData> m_serializedData = nullptr;
        //The file path name associated with serialized data. It can be passed
        //to the RHI backend to do load/save operation via the drivers.
        AZStd::string m_filePath;
    };
            
    //! DevicePipelineState initialization is an expensive operation on certain platforms. If multiple pipeline states
    //! are created with little variation between them, the contents are still duplicated. This class is an allocation
    //! context for pipeline states, provided at DevicePipelineState::Init, which will perform de-duplication of
    //! internal pipeline state components and cache the results.
    //!
    //! Practically speaking, if many pipeline states are created with shared data between them (e.g. permutations
    //! of the same shader), then providing a DevicePipelineLibrary instance will reduce the memory footprint and cost
    //! of compilation.
    //!
    //! Additionally, the DevicePipelineLibrary is able to serialize the internal driver-contents to and from an opaque
    //! data blob. This enables building up a pipeline state cache on disk, which can dramatically reduce pipeline
    //! state compilation cost when run from a pre-warmed cache.
    //!
    //! DevicePipelineLibrary is thread-safe, in the sense that it will take a lock during compilation. It is possible
    //! to initialize pipeline states across threads using the same DevicePipelineLibrary instance, but this will
    //! result in the two calls serializing on the mutex. Instead, see PipelineStateCache which stores
    //! a DevicePipelineLibrary instance per thread to avoid this contention.
    class DevicePipelineLibrary : public DeviceObject
    {
    public:
        AZ_RTTI(DevicePipelineLibrary, "{843579BE-57E4-4527-AB00-C0217885AEA9}");
        virtual ~DevicePipelineLibrary() = default;

        //! Initializes the pipeline library from a platform-specific data payload. This data is generated
        //! by calling GetSerializedData in a previous run of the application. When run for the first
        //! time, the serialized data should be empty. When the application completes, the library can be
        //! serialized and the contents saved to disk. Subsequent loads will experience much faster pipeline
        //! state creation times (on supported platforms). On success, the library is transitioned to the
        //! initialized state. On failure, the library remains uninitialized.
        //! @param descriptor The descriptor needed to init the DevicePipelineLibrary.
        ResultCode Init(Device& device, const DevicePipelineLibraryDescriptor& descriptor);

        //! Merges the contents of other libraries into this library. This method must be called
        //! on an initialized library. A common use case for this method is to construct thread-local
        //! libraries and merge them into a single unified library. The serialized data can then be
        //! extracted from the unified library. An error code is returned on failure and the behavior
        //! is as if the method was never called.
        ResultCode MergeInto(AZStd::span<const DevicePipelineLibrary* const> librariesToMerge);

        //! Serializes the platform-specific data and returns it as a new PipelineLibraryData instance.
        //! The data is opaque to the user and can only be used to re-initialize the library. Use
        //! this method to extract serialized data prior to application shutdown, save it to disk, and
        //! use it when initializing on subsequent runs.
        ConstPtr<PipelineLibraryData> GetSerializedData() const;

        //! Saves the platform-specific data to disk using the filePath provided. This is done through RHI backend drivers.
        bool SaveSerializedData(const AZStd::string& filePath) const;

        //! Returns whether the current library need to be merged
        virtual bool IsMergeRequired() const;

    private:
        bool ValidateIsInitialized() const;

        // Explicit shutdown is not allowed for this type.
        void Shutdown() override final;

        //////////////////////////////////////////////////////////////////////////
        // Platform API

        /// Called when the library is being created.
        virtual ResultCode InitInternal(Device& device, const DevicePipelineLibraryDescriptor& descriptor) = 0;

        /// Called when the library is being shutdown.
        virtual void ShutdownInternal() = 0;

        /// Called when libraries are being merged into this one.
        virtual ResultCode MergeIntoInternal(AZStd::span<const DevicePipelineLibrary* const> libraries) = 0;

        /// Called when the library is serializing out platform-specific data.
        virtual ConstPtr<PipelineLibraryData> GetSerializedDataInternal() const = 0;

        /// Called when we want the RHI backend to save out the Pipeline Library via the drivers
        virtual bool SaveSerializedDataInternal(const AZStd::string& filePath) const = 0;

        //////////////////////////////////////////////////////////////////////////
    };
}
