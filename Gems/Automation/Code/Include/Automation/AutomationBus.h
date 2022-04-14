/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/string/string_view.h>


namespace AZ
{
    class BehaviorContext;
} // namespace AZ

namespace Automation
{
    static constexpr float DefaultPauseTimeout = 5.0f;
    static constexpr AZ::Crc32 AutomationServiceCrc = AZ_CRC_CE("AutomationService");

    class AutomationRequests
    {
    public:
        using ScriptOperation = AZStd::function<void()>;

        AZ_RTTI(AutomationRequests, "{403E1E72-5070-4683-BFF8-289364791723}");
        virtual ~AutomationRequests() = default;

        //! Retrieve the specialized behaviour context used for automation purposes
        virtual AZ::BehaviorContext* GetAutomationContext() = 0;

        //! Can be used by sample components to temporarily pause script processing, for
        //! example to delay until some required resources are loaded and initialized.
        virtual void PauseAutomation(float timeout = DefaultPauseTimeout) = 0;
        virtual void ResumeAutomation() = 0;

        //! Add an operation into the queue for processing later
        virtual void QueueScriptOperation(ScriptOperation&& action) = 0;
    };

    class AutomationRequestsBusTraits
        : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    using AutomationRequestBus = AZ::EBus<AutomationRequests, AutomationRequestsBusTraits>;
    using AutomationInterface = AZ::Interface<AutomationRequests>;


    class AutomationNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~AutomationNotifications() = default;

        virtual void OnAutomationStarted() = 0;
        virtual void OnAutomationFinished() = 0;
    };
    using AutomationNotificationBus = AZ::EBus<AutomationNotifications>;
} // namespace Automation
