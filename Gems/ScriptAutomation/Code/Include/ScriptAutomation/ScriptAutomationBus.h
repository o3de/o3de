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
#include <AzCore/std/string/string.h>


namespace AZ
{
    class BehaviorContext;
} // namespace AZ

namespace ScriptAutomation
{
    static constexpr float DefaultPauseTimeout = 5.0f;
    static constexpr AZ::Crc32 AutomationServiceCrc = AZ_CRC_CE("AutomationService");

    class ScriptAutomationRequests
    {
    public:
        using ScriptOperation = AZStd::function<void()>;

        AZ_RTTI(ScriptAutomationRequests, "{403E1E72-5070-4683-BFF8-289364791723}");
        virtual ~ScriptAutomationRequests() = default;

        //! Retrieve the specialized behaviour context used for automation purposes
        virtual AZ::BehaviorContext* GetAutomationContext() = 0;

        //! Can be used by sample components to temporarily pause script processing, for
        //! example to delay until some required resources are loaded and initialized.
        virtual void PauseAutomation(float timeout = DefaultPauseTimeout) = 0;
        virtual void ResumeAutomation() = 0;

        //! Can be used to set number of frame/seconds for which the script is paused
        virtual void SetIdleFrames(int numFrames) = 0;
        virtual void SetIdleSeconds(float numSeconds) = 0;

        //! Add an operation into the queue for processing later
        virtual void QueueScriptOperation(ScriptOperation&& action) = 0;

        //! Runs a lua script at the provided path
        virtual void ExecuteScript(const AZStd::string& scriptFilePath) = 0;
    };

    class ScriptAutomationRequestsBusTraits
        : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    using ScriptAutomationRequestBus = AZ::EBus<ScriptAutomationRequests, ScriptAutomationRequestsBusTraits>;
    using ScriptAutomationInterface = AZ::Interface<ScriptAutomationRequests>;


    class ScriptAutomationHooks
        : public AZ::EBusTraits
    {
    public:
        virtual ~ScriptAutomationHooks() = default;

        virtual void AutomationStarted([[maybe_unused]] const AZStd::string& scriptPath) {};
        virtual void AutomationFinished() {};

        virtual void CustomReflect([[maybe_unused]] AZ::BehaviorContext* context) {}
        
        virtual void PreScriptExecution([[maybe_unused]] const AZStd::string& scriptPath) {}
        virtual void PostScriptExecution([[maybe_unused]] const AZStd::string& scriptPath) {}

        virtual void PreTick() {}
        virtual void PostTick() {}
    };
    using ScriptAutomationEventsBus = AZ::EBus<ScriptAutomationHooks>;
} // namespace ScriptAutomation
