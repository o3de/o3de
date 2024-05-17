/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceResource.h>
#include <AzCore/EBus/EBus.h>

namespace AZ::RHI
{
    enum ResourceEventPriority : int32_t
    {
        Low = 0,
        Default = 1,
        High = 2
    };

    //! This bus is used as a queue for controlling DeviceResourceView invalidations during
    //! the compilation phase of FrameScheduler. Essentially, when a resource
    //! invalidates (via a call to DeviceResource::InvalidateViews), the resource queues
    //! an operation on this queue. The queue is then flushed by the FrameScheduler.
    //!
    //! Downstream systems that need to rebuild platform-specific view information
    //! (e.g. DeviceShaderResourceGroupPool) listen on this bus and perform those updates
    //! when the queue is flushed.
    //!
    //! This bus is for INTERNAL use only.
    //!
    //! NOTE: This bus is currently a singleton. That effectively forces FrameScheduler
    //! to be one as well. See [LY-83241] for more information.
    class ResourceEventInterface
        : public AZ::EBusTraits
    {
    public:
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::MultipleAndOrdered;
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        using MutexType = AZStd::mutex;
        using BusIdType = const DeviceResource*;
        static const bool LocklessDispatch = true;
        static const bool EnableEventQueue = true;

        //! Access to the priority of the input notification handler (sorted from highest to lowest)
        //! \return Priority of the input notification handler
        virtual ResourceEventPriority GetPriority() const { return Default; }

        //! Compare function required by BusHandlerOrderCompare = BusHandlerCompareDefault
        //! \param[in] other Another instance of the class to compare
        //! \return True if the priority of this handler is greater than the other, false otherwise
        inline bool Compare(const ResourceEventInterface* other) const
        {
            return GetPriority() > other->GetPriority();
        }

        //! Called when the resource invalidates due to a version change.
        virtual ResultCode OnResourceInvalidate() = 0;
    };

    using ResourceInvalidateBus = AZ::EBus<ResourceEventInterface>;
}
