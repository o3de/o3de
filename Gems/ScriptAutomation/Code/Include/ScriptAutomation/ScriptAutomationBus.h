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

        //! Called before first(passed as command-line parameter) lua script runs
        virtual void AutomationStarted([[maybe_unused]] const AZStd::string& scriptPath) {};

        //! Called after all lua scripts have finished running
        virtual void AutomationFinished() {};

        //! Override this function to reflect custom functions to ScriptAutomation lua scripts
        virtual void CustomReflect([[maybe_unused]] AZ::BehaviorContext* context) {}
        
        //! Called before each call to RunScript() from lua
        virtual void PreScriptExecution([[maybe_unused]] const AZStd::string& scriptPath) {}

        //! Called after each call to RunScript() from lua returns
        virtual void PostScriptExecution([[maybe_unused]] const AZStd::string& scriptPath) {}

        //! Called before each ScriptAutomation Tick() runs its script opertaions loop
        virtual void PreTick() {}

        //! Called after each ScriptAutomation Tick() runs its script opertaions loop
        virtual void PostTick() {}
    };
    using ScriptAutomationEventsBus = AZ::EBus<ScriptAutomationHooks>;
} // namespace ScriptAutomation
