/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI/ResourceInvalidateBus.h>

namespace AZ::RHI
{
    class DeviceResource;

    //! DeviceResourceView is a base class for views which are dependent on a DeviceResource instance.
    //!
    //! NOTE: While initialization is separate from creation, explicit shutdown is not allowed
    //! for resource views. This is because the cost of dependency tracking with ShaderResourceGroups
    //! is too high. Instead, resource views are reference counted.
    class DeviceResourceView
        : public DeviceObject
        , public ResourceInvalidateBus::Handler
    {
    public:
        // The resource owns a cache of resource views, and it needs access to the refcount
        // of the resource views to prevent threading issues.
        friend class DeviceResource;

        virtual ~DeviceResourceView() = default;

        //! Returns the resource associated with this view.
        const DeviceResource& GetResource() const;

        //! Returns whether this view is stale (i.e. the original image contents have
        //! been shutdown.
        bool IsStale() const;

        //! Returns whether the view covers the entire image (i.e. isn't just a subset).
        virtual bool IsFullView() const = 0;

        AZ_RTTI(DeviceResourceView, "{7F50934E-A2F3-4989-BB8C-F3AFE33BEBDD}", Object);

    protected:

        /// The derived class should call this method at Init time.
        ResultCode Init(const DeviceResource& resource);

    private:
        //////////////////////////////////////////////////////////////////////////
        // RHI::Object
        void Shutdown() final;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ResourceInvalidateBus::Handler
        ResultCode OnResourceInvalidate() override final;
        ResourceEventPriority GetPriority() const override { return High; }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Platform API

        //! Called when the view is being initialized.
        virtual ResultCode InitInternal(Device& device, const DeviceResource& resource) = 0;

        //! Called when the view is shutting down.
        virtual void ShutdownInternal() = 0;

        //! Called when the view is being invalidated.
        virtual ResultCode InvalidateInternal() = 0;

        //////////////////////////////////////////////////////////////////////////

        //This is a smart pointer to make sure a DeviceResource is not destroyed before all
        //the views (for example Srg resource views) are destroyed first.
        ConstPtr<DeviceResource> m_resource = nullptr;

        /// The version number from the resource at view creation time. If the keys differ, the view is stale.
        uint32_t m_version = 0;
    };
}
