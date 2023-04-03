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
    namespace Render
    {
        using FrameCaptureId = uint32_t;
    }
} // namespace AZ

namespace ScriptAutomation
{
    static constexpr float DefaultPauseTimeout = 10.0f;
    static constexpr AZ::Crc32 AutomationServiceCrc = AZ_CRC_CE("AutomationService");

    class ScriptAutomationRequests
    {
    public:
        using ScriptOperation = AZStd::function<void()>;

        AZ_RTTI(ScriptAutomationRequests, "{403E1E72-5070-4683-BFF8-289364791723}");
        virtual ~ScriptAutomationRequests() = default;

        //! Retrieve the specialized behaviour context used for automation purposes
        virtual AZ::BehaviorContext* GetAutomationContext() = 0;

        //! Load and activate the script, and connect to the tick bus.
        virtual void ActivateScript(const char* scriptPath) = 0;

        //! Deactivate all scripts and disconnect from the tick bus.
        virtual void DeactivateScripts() = 0;

        //! Can be used by sample components to temporarily pause script processing, for
        //! example to delay until some required resources are loaded and initialized.
        virtual void PauseAutomation(float timeout = DefaultPauseTimeout) = 0;
        virtual void ResumeAutomation() = 0;

        //! Set the script automation to idle for the number of frames
        virtual void SetIdleFrames(int numFrames) = 0;

        //! set the script automation to idle for the supplied number of seconds
        virtual void SetIdleSeconds(float numSeconds) = 0;

        //! pass the frame capture id to the automation system so it can listen for capture completion
        virtual void SetFrameCaptureId(AZ::Render::FrameCaptureId frameCaptureId) = 0;

        //! tell the automation system that a profiling capture has started
        virtual void StartProfilingCapture() = 0;

        //! Add an operation into the queue for processing later
        virtual void QueueScriptOperation(ScriptOperation&& action) = 0;
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


    class ScriptAutomationNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~ScriptAutomationNotifications() = default;

        virtual void OnAutomationStarted() = 0;
        virtual void OnAutomationFinished() = 0;
    };
    using ScriptAutomationNotificationBus = AZ::EBus<ScriptAutomationNotifications>;
} // namespace ScriptAutomation
