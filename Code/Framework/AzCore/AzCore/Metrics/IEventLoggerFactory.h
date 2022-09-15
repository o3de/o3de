/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/function/function_fwd.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ::Metrics
{
    class IEventLogger;

    enum class EventLoggerId : AZ::u32 {};

    AZ_DEFINE_ENUM_RELATIONAL_OPERATORS(EventLoggerId);

    class IEventLoggerFactory
    {
    public:
        AZ_RTTI(IEventLoggerFactory, "{3E98C565-3A1E-460E-B692-4FAE783952CC}");
        virtual ~IEventLoggerFactory() = default;

        //! Callback function that is invoked for every registered event logger
        //! return true to continue visiting
        using VisitEventLoggerInterfaceCallback = AZStd::function<bool(IEventLogger&)>;
        //! Invokes the supplied visitor for each register compression interface that is non-nullptr
        virtual void VisitEventLoggers(const VisitEventLoggerInterfaceCallback&) const = 0;

        //! Registers event logger and takes ownership of it if registration is successful
        //! @param eventLogger Event logger to register
        //! @return true if the IEventLogger was successfully registered
        //! If the logger was successfully registered a unique_ptr containing it is returned instead
        virtual AZ::Outcome<void, AZStd::unique_ptr<IEventLogger>> RegisterEventLogger(EventLoggerId loggerId, AZStd::unique_ptr<IEventLogger>) = 0;

        //! Registers event logger, but does not take ownership of it
        //! @param eventLogger Unique id to use to register the Event Logger
        //! @param eventLogger Event logger to register
        //! @return true if the IEventLogger was successfully registered
        virtual bool RegisterEventLogger(EventLoggerId loggerId, IEventLogger&) = 0;

        //! Unregisters the EventLogger Interface with the specified id
        //! @param eventLogger Id to lookup the event logger
        //! @return true if the IEventLoggerwas unregistered
        virtual bool UnregisterEventLogger(EventLoggerId loggerId) = 0;

        //! Find event logger registered with the specified id
        //! @param eventLogger Id to lookup the event logger
        //! @return pointer to the EventLogger
        virtual IEventLogger* FindEventLogger(EventLoggerId) const = 0;

        //! Return true if there is an event logger registered with the specified id
        //! @param eventLogger Id to lookup the event logger
        //! @return bool indicating if there is an event logger with the id registered
        virtual bool IsRegistered(EventLoggerId) const = 0;
    };

    using EventLoggerFactory = AZ::Interface<IEventLoggerFactory>;

} // namespace AZ::Event::Metrics

