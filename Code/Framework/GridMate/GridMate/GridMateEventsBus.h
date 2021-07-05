/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GRIDMATEEVENTSBUS_H
#define GRIDMATEEVENTSBUS_H

#include <GridMate/Memory.h>
#include <AzCore/EBus/EBus.h>

namespace GridMate
{
    class IGridMate;
    class GridMateService;

    /*
    * GridMate callbacks
    * These callbacks are thrown on main GridMate thread(thread where GridMate's tick is pumped on)
    */
    class GridMateEvents
        : public AZ::EBusTraits
    {
    public:
        // Called after gridmate is initialized
        virtual void OnGridMateInitialized(IGridMate* gridMate) { (void)gridMate; }

        // Called on GridMate tick
        virtual void OnGridMateUpdate(IGridMate* gridMate) { (void)gridMate; }

        // Called when gridmate is shutting down, GridMate reference is still valid inside this call, but should not be used afterwards
        virtual void OnGridMateShutdown(IGridMate* gridMate) { (void)gridMate; }

        // Called when new service is added to GridMate
        virtual void OnGridMateServiceAdded(IGridMate* gridMate, GridMateService* service) { (void)gridMate; (void)service; }

        // Called when service is about to be deleted, service cannot be used afterwards.
        virtual void OnGridMateServiceDelete(IGridMate* gridMate, GridMateService* service) { (void)gridMate; (void)service; }

        // EBus Traits
        AZ_CLASS_ALLOCATOR(GridMateEvents, GridMateAllocator, 0);

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::MultipleAndOrdered; ///< Events are ordered, each handler may set its priority
        typedef IGridMate* BusIdType; ///< Use the GridMate instance as an ID
        bool Compare(const GridMateEvents* another) const { return GetPriority() > another->GetPriority(); }

    protected:
        static const unsigned int k_defaultPriority = 100; ///< priority that will be used for callback ordering (the smaller - the earler handler will be in events queue), default is 100
        virtual unsigned int GetPriority() const { return k_defaultPriority; }
    };

    typedef AZ::EBus<GridMateEvents> GridMateEventsBus;
}

#endif
