/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/MultiDeviceObject.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ::RHI
{
    class MultiDeviceResourcePool;
    class FrameAttachment;
    class MemoryStatisticsBuilder;
    class ResourceView;
    class ImageView;
    class BufferView;
    struct ImageViewDescriptor;
    struct BufferViewDescriptor;

    //! MultiDeviceResource is a base class for pooled, multi-device RHI resources (MultiDeviceImage / MultiDeviceBuffer /
    //! MultiDeviceShaderResourceGroup, etc). It provides some common lifecycle management semantics. MultiDeviceResource creation is
    //! separate from initialization. Resources are created separate from any pool, but its backing platform data is associated at
    //! initialization time on a specific pool.
    class MultiDeviceResource : public MultiDeviceObject
    {
        friend class FrameAttachment;
        friend class MultiDeviceResourcePool;

    public:
        AZ_RTTI(MultiDeviceResource, "{613AED98-48FD-4453-98F8-6956D2133489}", MultiDeviceObject);
        virtual ~MultiDeviceResource();

        //! Returns whether the resource is currently an attachment on a frame graph.
        bool IsAttachment() const;

        //! Shuts down the resource by detaching it from its parent pool.
        virtual void Shutdown() override = 0;

        //! Returns the parent pool this resource is registered on. Since resource creation is
        //! separate from initialization, this will be null until the resource is registered on a pool.
        const MultiDeviceResourcePool* GetPool() const;
        MultiDeviceResourcePool* GetPool();

        //! Returns the version number. This number is monotonically increased anytime
        //! new platform memory is assigned to the resource. Any dependent resource is
        //! valid so long as the version numbers match.
        uint32_t GetVersion() const;

        //! Returns the frame attachment associated with this image (if it exists).
        const FrameAttachment* GetFrameAttachment() const;

        //! Invalidates all views referencing this resource. Invalidation is handled implicitly
        //! on a Shutdown / Init cycle from the pool. For example, it is safe to create a resource,
        //! create a view to that resource, and then Shutdown / Re-Init the resource. InvalidateViews is
        //! called to synchronize views (and shader resource groups which hold them) to the new data.
        //!
        //! Platform back-ends which invalidate GPU-specific data on the resource without an explicit
        //! shutdown / re-initialization will need to call this method explicitly.
        virtual void InvalidateViews() = 0;

    protected:
        MultiDeviceResource() = default;

    private:
        //! Returns whether this resource has been initialized before.
        bool IsFirstVersion() const;

        //! Called by the parent pool at initialization time.
        void SetPool(MultiDeviceResourcePool* pool);

        //! Called by the frame attachment at frame building time.
        void SetFrameAttachment(FrameAttachment* frameAttachment);

        //! The parent pool this resource is registered with.
        MultiDeviceResourcePool* m_mdPool = nullptr;

        //! The current frame attachment registered on this resource.
        FrameAttachment* m_frameAttachment = nullptr;

        //! The version is monotonically incremented any time the backing resource is changed.
        uint32_t m_version = 0;
    };
} // namespace AZ::RHI