/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_THREAD_BUS_H
#define AZSTD_THREAD_BUS_H 1

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/EBus/EBus.h>

namespace AZStd
{
    //! ThreadEvents are used by user-code.
    //! You can connect to the ThreadEvents bus, and then be told whenever a new thread is spawned
    //! and when it dies.
    class ThreadEvents : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;

        virtual ~ThreadEvents() {}

        /// Called when we enter a thread, optional thread_desc is provided when the use provides one.
        virtual void OnThreadEnter(const AZStd::thread::id& id, const AZStd::thread_desc* desc) = 0;
        /// Called when we exit a thread.
        virtual void OnThreadExit(const AZStd::thread::id& id) = 0;
    };

    typedef AZ::EBus<ThreadEvents> ThreadEventBus;
}

#endif // AZSTD_THREAD_BUS_H
#pragma once
