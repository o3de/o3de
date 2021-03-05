/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once
#include <AzCore/EBus/EBus.h>
#include <Editor/Framework/ScriptCanvasReporter.h>

namespace ScriptCanvasEditor
{

    struct UnitTestResult
    {   
        bool m_success;
        bool m_running;
        AZStd::string m_consoleOutput;
        bool m_latestTestingRound;

        UnitTestResult()
            : m_success(true)
            , m_running(false)
            , m_latestTestingRound(true)
        {
        }
        
        UnitTestResult(bool newRunning, bool newSuccess, AZStd::string_view newConsoleOutput)
            : m_success(newSuccess)
            , m_running(newRunning)
            , m_consoleOutput(newConsoleOutput)
            , m_latestTestingRound(true)
        {
        }
    };

    class UnitTestVerificationRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual UnitTestResult Verify(ScriptCanvasEditor::Reporter reporter) = 0;
    };
    using UnitTestVerificationBus = AZ::EBus<UnitTestVerificationRequests>;

    class UnitTestWidgetNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        virtual void OnTestStart([[maybe_unused]] const AZ::Uuid& sourceID) {}
        virtual void OnTestResult([[maybe_unused]] const AZ::Uuid& sourceID, UnitTestResult result) {}
        virtual void OnCheckStateCountChange([[maybe_unused]] const int count) {}
    };
    using UnitTestWidgetNotificationBus = AZ::EBus<UnitTestWidgetNotifications>;
}