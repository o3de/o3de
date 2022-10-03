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
        //! Invokes the supplied visitor for each non-nullptr event logger
        virtual void VisitEventLoggers(const VisitEventLoggerInterfaceCallback&) const = 0;

        //! Registers event logger and takes ownership of it if registration is successful
        //! @param loggerId Unique id to associate with EventLogger
        //! @param eventLogger Event logger to register
        //! @return Success outcome if the Event logger was successfully registered
        //! If the logger could not be registered, a failure outcome with the event logger is returned
        virtual AZ::Outcome<void, AZStd::unique_ptr<IEventLogger>> RegisterEventLogger(EventLoggerId loggerId, AZStd::unique_ptr<IEventLogger>) = 0;

        //! Registers event logger, but does not take ownership of it
        //! @param loggerId Unique id to associate with EventLogger
        //! @param eventLogger Event logger to register
        //! @return true if the IEventLogger was successfully registered
        virtual bool RegisterEventLogger(EventLoggerId loggerId, IEventLogger&) = 0;

        //! Unregisters the EventLogger Interface with the specified id
        //! @param loggerId Id to lookup the event logger
        //! @return true if the unregistration is successful
        virtual bool UnregisterEventLogger(EventLoggerId loggerId) = 0;

        //! Find event logger registered with the specified id
        //! @param loggerId EventLoggerId to locate a registered event logger
        //! @return pointer to the EventLogger
        [[nodiscard]] virtual IEventLogger* FindEventLogger(EventLoggerId) const = 0;

        //! Return true if there is an event logger registered with the specified id
        //! @param loggerId EventLoggerId to determine if an event logger is registered
        //! @return bool indicating if there is an event logger with the id registered
        [[nodiscard]] virtual bool IsRegistered(EventLoggerId) const = 0;
    };

    using EventLoggerFactory = AZ::Interface<IEventLoggerFactory>;

} // namespace AZ::Metrics
