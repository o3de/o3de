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
    template <class DeviceResource, class DeviceResourcePool>
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
    template <class DeviceResource, class DeviceResourcePool>
    class MultiDeviceResource : public MultiDeviceObject<DeviceResource>
    {
        friend class FrameAttachment;
        friend class MultiDeviceResourcePool<DeviceResource, DeviceResourcePool>;

    public:
        AZ_RTTI((MultiDeviceResource, "{613AED98-48FD-4453-98F8-6956D2133489}", DeviceResource, DeviceResourcePool), MultiDeviceObject<DeviceResource>);
        virtual ~MultiDeviceResource();

        //! Returns whether the resource is currently an attachment on a frame graph.
        bool IsAttachment() const;

        //! Shuts down the resource by detaching it from its parent pool.
        virtual void Shutdown() override = 0;

        //! Returns the parent pool this resource is registered on. Since resource creation is
        //! separate from initialization, this will be null until the resource is registered on a pool.
        const MultiDeviceResourcePool<DeviceResource, DeviceResourcePool>* GetPool() const;
        MultiDeviceResourcePool<DeviceResource, DeviceResourcePool>* GetPool();

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
        void SetPool(MultiDeviceResourcePool<DeviceResource, DeviceResourcePool>* pool);

        //! Called by the frame attachment at frame building time.
        void SetFrameAttachment(FrameAttachment* frameAttachment);

        //! The parent pool this resource is registered with.
        MultiDeviceResourcePool<DeviceResource, DeviceResourcePool>* m_mdPool = nullptr;

        //! The current frame attachment registered on this resource.
        FrameAttachment* m_frameAttachment = nullptr;

        //! The version is monotonically incremented any time the backing resource is changed.
        uint32_t m_version = 0;
    };

    template <class DeviceResource, class DeviceResourcePool>
    MultiDeviceResource<DeviceResource, DeviceResourcePool>::~MultiDeviceResource()
    {
        AZ_Assert(
            GetPool() == nullptr,
            "MultiDeviceResource '%s' is still registered on pool. %s",
            this->GetName().GetCStr(),
            GetPool()->GetName().GetCStr());
    }

    template <class DeviceResource, class DeviceResourcePool>
    bool MultiDeviceResource<DeviceResource, DeviceResourcePool>::IsAttachment() const
    {
        return m_frameAttachment != nullptr;
    }

    template <class DeviceResource, class DeviceResourcePool>
    uint32_t MultiDeviceResource<DeviceResource, DeviceResourcePool>::GetVersion() const
    {
        return m_version;
    }

    template <class DeviceResource, class DeviceResourcePool>
    bool MultiDeviceResource<DeviceResource, DeviceResourcePool>::IsFirstVersion() const
    {
        return m_version == 0;
    }

    template <class DeviceResource, class DeviceResourcePool>
    void MultiDeviceResource<DeviceResource, DeviceResourcePool>::SetPool(MultiDeviceResourcePool<DeviceResource, DeviceResourcePool> *bufferPool)
    {
        m_mdPool = bufferPool;

        const bool isValidPool = bufferPool != nullptr;
        if (isValidPool)
        {
            // Only invalidate the resource if it has dependent views. It
            // can't have any if this is the first initialization.
            if (!IsFirstVersion())
            {
                InvalidateViews();
            }
        }

        ++m_version;
    }

    template <class DeviceResource, class DeviceResourcePool>
    const MultiDeviceResourcePool<DeviceResource, DeviceResourcePool> *MultiDeviceResource<DeviceResource, DeviceResourcePool>::GetPool() const
    {
        return m_mdPool;
    }

    template <class DeviceResource, class DeviceResourcePool>
    MultiDeviceResourcePool<DeviceResource, DeviceResourcePool> *MultiDeviceResource<DeviceResource, DeviceResourcePool>::GetPool()
    {
        return m_mdPool;
    }

    template <class DeviceResource, class DeviceResourcePool>
    void MultiDeviceResource<DeviceResource, DeviceResourcePool>::SetFrameAttachment(FrameAttachment* frameAttachment)
    {
        if (Validation::IsEnabled())
        {
            // The frame attachment has tight control over lifecycle here.
            [[maybe_unused]] const bool isAttach = (!m_frameAttachment && frameAttachment);
            [[maybe_unused]] const bool isDetach = (m_frameAttachment && !frameAttachment);
            AZ_Assert(isAttach || isDetach, "The frame attachment for resource '%s' was not assigned properly.", this->GetName().GetCStr());
        }

        m_frameAttachment = frameAttachment;
    }

    template <class DeviceResource, class DeviceResourcePool>
    const FrameAttachment* MultiDeviceResource<DeviceResource, DeviceResourcePool>::GetFrameAttachment() const
    {
        return m_frameAttachment;
    }

    template <class DeviceResource, class DeviceResourcePool>
    void MultiDeviceResource<DeviceResource, DeviceResourcePool>::Shutdown()
    {
        // Shutdown is delegated to the parent pool if this resource is registered on one.
        if (m_mdPool)
        {
            AZ_Error(
                "MultiDeviceResource",
                m_frameAttachment == nullptr,
                "The resource is currently attached on a frame graph. It is not valid "
                "to shutdown a resource while it is being used as an Attachment. The "
                "behavior is undefined.");

            m_mdPool->ShutdownResource(this);
        }
        MultiDeviceObject<DeviceResource>::Shutdown();
    }
} // namespace AZ::RHI
