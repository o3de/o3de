/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 #pragma once

 #include <Atom/RHI/MultiDeviceObject.h>
 #include <Atom/RHI/DeviceResourceView.h>

 namespace AZ::RHI
{
    class Resource;

    //! ResourceView is a base class for multi-device buffer and image views for
    //! polymorphic usage of views in a generic way.
    //! As the handling of the device-specific ResourceViews is more elaborate,
    //! this does not inherit from MultiDeviceObject but manages the DeviceResourceViews on its own
    class ResourceView : public Object
    {
    public:
        // The resource owns a cache of resource views, and it needs access to the refcount
        // of the resource views to prevent threading issues.
        friend class Resource;

        AZ_RTTI(ResourceView, "{D7442960-531D-4DCC-B60D-FD26FF75BE51}", Object);

        ResourceView(const RHI::Resource* resource, MultiDevice::DeviceMask deviceMask)
            : m_resource{ resource }
            , m_deviceMask{ deviceMask }
        {
        }
        virtual ~ResourceView() = default;

        //! Returns the resource associated with this view.
        const Resource* GetResource() const;
        //! Interface for the two derived classes to return a DeviceResourceView
        virtual const DeviceResourceView* GetDeviceResourceView(int deviceIndex) const = 0;

    protected:
        //! Templated method for both DeviceImageView and DeviceBufferView that either creates and caches
        //! or return the corresponding DeviceResourceView
        template<typename View, typename ViewDescriptor>
        const RHI::Ptr<View> GetDeviceResourceView(int deviceIndex, const ViewDescriptor& viewDescriptor) const;

    private:
        //////////////////////////////////////////////////////////////////////////
        //! RHI::Object
        void Shutdown() final;

        //! A ConstPtr pointer to a Resource which extends its lifetime
        ConstPtr<RHI::Resource> m_resource;
        //! The device mask of the Resource stored for comparison to figure out when cache entries need to be freed.
        mutable MultiDevice::DeviceMask m_deviceMask{ MultiDevice::AllDevices};
        //! Safe-guard access to DeviceResourceView objects during parallel access
        mutable AZStd::mutex m_resourceViewMutex;
        //! DeviceResourceView cache
        //! This cache is necessary as the caller receives raw pointers from the ResourceCache, 
        //! which now, with multi-device objects in use, need to be held in memory as long as
        //! the multi-device view is held.
        mutable AZStd::unordered_map<int, Ptr<RHI::DeviceResourceView>> m_cache;
    };
}