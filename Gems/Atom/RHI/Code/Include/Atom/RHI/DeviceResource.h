/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceObject.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>


namespace AZ::RHI
{
    class DeviceResourcePool;
    class FrameAttachment;
    class MemoryStatisticsBuilder;
    class DeviceResourceView;
    class DeviceImageView;
    class DeviceBufferView;
    struct ImageViewDescriptor;
    struct BufferViewDescriptor;
    
        
    //! DeviceResource is a base class for pooled RHI resources (DeviceImage / DeviceBuffer / DeviceShaderResourceGroup, etc). It provides
    //! some common lifecycle management semantics. DeviceResource creation is separate from initialization. Resources are
    //! created separate from any pool, but its backing platform data is associated at initialization time on a specific pool.
    class DeviceResource
        : public DeviceObject
    {
        friend class Resource;
        friend class DeviceResourcePool;
    public:
        AZ_RTTI(DeviceResource, "{9D02CDAC-80EB-4B77-8E62-849AC6E69206}", DeviceObject);
        virtual ~DeviceResource();

        /// Returns whether the resource is currently an attachment on a frame graph.
        bool IsAttachment() const;

        /// Shuts down the buffer by detaching it from its parent pool.
        void Shutdown() override final;

        //! Returns the parent pool this resource is registered on. Since resource creation is
        //! separate from initialization, this will be null until the resource is registered on a pool.
        const DeviceResourcePool* GetPool() const;
        DeviceResourcePool* GetPool();

        //! Returns the version number. This number is monotonically increased anytime
        //! new platform memory is assigned to the resource. Any dependent resource is
        //! valid so long as the version numbers match.
        uint32_t GetVersion() const;

        /// Reports memory usage of this image to the memory statistics builder.
        virtual void ReportMemoryUsage(MemoryStatisticsBuilder& builder) const = 0;

        /// Returns the frame attachment associated with this image (if it exists).
        const FrameAttachment* GetFrameAttachment() const;
            
        //! Invalidates all views referencing this resource. Invalidation is handled implicitly
        //! on a Shutdown / Init cycle from the pool. For example, it is safe to create a resource,
        //! create a view to that resource, and then Shutdown / Re-Init the resource. InvalidateViews is
        //! called to synchronize views (and shader resource groups which hold them) to the new data.
        //!
        //! Platform back-ends which invalidate GPU-specific data on the resource without an explicit
        //! shutdown / re-initialization will need to call this method explicitly.
        void InvalidateViews();
            
        //! Returns true if the DeviceResourceView is in the cache
        bool IsInResourceCache(const ImageViewDescriptor& imageViewDescriptor);
        bool IsInResourceCache(const BufferViewDescriptor& bufferViewDescriptor);
            
        //! Removes the provided DeviceResourceView from the cache
        void EraseResourceView(DeviceResourceView* resourceView) const;
                                    
    protected:
        DeviceResource() = default;

        //! Returns view based on the descriptor
        Ptr<DeviceImageView> GetResourceView(const ImageViewDescriptor& imageViewDescriptor) const;
        Ptr<DeviceBufferView> GetResourceView(const BufferViewDescriptor& bufferViewDescriptor) const;

    private:
        /// Returns whether this resource has been initialized before.
        bool IsFirstVersion() const;

        /// Called by the parent pool at initialization time.
        void SetPool(DeviceResourcePool* pool);

        /// Called by the frame attachment at frame building time.
        void SetFrameAttachment(FrameAttachment* frameAttachment);

        /// Called by GetResourceView to insert a new image view
        Ptr<DeviceImageView> InsertNewImageView(HashValue64 hash, const ImageViewDescriptor& imageViewDescriptor) const;

        /// Called by GetResourceView to insert a new buffer view
        Ptr<DeviceBufferView> InsertNewBufferView(HashValue64 hash, const BufferViewDescriptor& bufferViewDescriptor) const;
                                    
        /// The parent pool this resource is registered with.
        DeviceResourcePool* m_pool = nullptr;

        /// The current frame attachment registered on this resource.
        FrameAttachment* m_frameAttachment = nullptr;

        /// The version is monotonically incremented any time the backing resource is changed.
        uint32_t m_version = 0;

        /// Tracks whether an invalidation request is currently queued on this resource.
        bool m_isInvalidationQueued = false;
            
        // Cache the resourceViews in order to avoid re-creation
        // Since DeviceResourceView has a dependency to DeviceResource this cache holds raw pointers here in order to ensure there
        // is no circular dependency between the resource and it's resourceview.
        mutable AZStd::unordered_map<size_t, DeviceResourceView*> m_resourceViewCache;
        // This should help provide thread safe access to resourceView cache
        mutable AZStd::mutex m_cacheMutex;
    };
}
