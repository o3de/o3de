/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
/**
 * @file
 * Provides EBus definitions for getting the utility thread tick
 */
#ifndef ONLINE_UTILITY_THREAD_H
#define ONLINE_UTILITY_THREAD_H

#include <GridMate/EBus.h>
#include <AzCore/std/parallel/mutex.h>

/**
  * IMPORTANT NOTE TO SERVICES THAT USE THE UTILITY THREAD:
  * The online service will start ticking the utility thread at construction
  * time, but you have have to let it know when you need to be ticked. This
  * is done two ways: first, send the NotifyOfNewWork event to the
  * OnlineUtilityThreadCommandBus; second, return whether you still have
  * work to do in OnlineUtilityThreadNotificationBus's event
  * IsThereUtilityThreadWork. There, however, caveats the service
  * must be aware of.
  *  - For those that derive from OnlineUtilityNotificationBus::Handler,
  *    do your BusConnect and BusDisConnect calls in your Init and Shutdown
  *    calls, instead of at construction and destruction time. You shouldn't
  *    be trying to use this utility thread outside of the time between these
  *    calls to your service anyway, so this shouldn't cause any amount of
  *    headache to conform to.
  *  - When you call BusConnect and BusDiscconect, the online manager may
  *    already be ticking that event - you may or may not receive your first
  *    and/or last tick events the way you might expect, so be careful about
  *    how you do you initialization and shutdown procedures.
  *  - Your Init call should do as little work as possible. Set yourself up for
  *    being ready to do actual initialization the first time you receive the
  *    OnUtilityThreadTick event instead of doing it all in Init and blocking
  *    the main thread.
  *  - Your Shutdown call should abort any pending operations, including ones
  *    it's already in the middle of.
  *  - In your OnUtilityThreadTick event response, make sure you haven't already
  *    been told to shut down. This is because the Shutdown call may have been
  *    made soon after the OnUtilityThreadTick event was fired, and other
  *    services took up a fair amount of time before the event got to you (with
  *    the Shutdown call to your service being made between event-firing and
  *    when the event reached you).
  *  - Be VERY careful about Shutdown getting called before you finish
  *    initializing in the utility thread (or even get a change to)! If you
  *    use this utility thread, be sure to test whether you can shutdown
  *    immediately after being initialized without breaking anything.
  */
namespace GridMate
{
    //-------------------------------------------------------------------------
    // For ticking services that need a separate thread (outbound)
    //  - BusConnect to OnlineUtilityThreadNotificationBus::Handler to receive OnUtilityThreadTick
    //  - Return whether you have work left to do in IsThereWork
    //-------------------------------------------------------------------------
    class OnlineUtilityThreadNotifications
        : public GridMateEBusTraits
    {
    public:
        virtual ~OnlineUtilityThreadNotifications() {}

        // Called on each iteration of the online manager's utility thread loop
        virtual void OnUtilityThreadTick() = 0;

        // Return whether there's work left to do here to keep the thread from doing busy waiting
        virtual bool IsThereUtilityThreadWork() = 0;
    };
    typedef AZ::EBus<OnlineUtilityThreadNotifications> OnlineUtilityThreadNotificationBus;
    //-------------------------------------------------------------------------

    //-------------------------------------------------------------------------
    // For services that need a separate thread (inbound)
    //  - Fire the NotifyOfNewWork event to notify the thread that you have a new
    //    request you'd like to take care of
    //-------------------------------------------------------------------------
    class OnlineUtilityThreadCommands
        : public GridMateEBusTraits
    {
    public:
        virtual ~OnlineUtilityThreadCommands() {}

        virtual void NotifyOfNewWork() = 0;
    };
    typedef AZ::EBus<OnlineUtilityThreadCommands> OnlineUtilityThreadCommandBus;
    //-------------------------------------------------------------------------
} // namespace GridMate

#endif // ONLINE_UTILITY_THREAD_H
