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
        bool m_compiled = false;
        bool m_running = false;
        bool m_completed = false;
        bool m_latestTestingRound = true;

        AZStd::string m_consoleOutput;

        AZ_INLINE static UnitTestResult AssumeFailure()
        {
            return UnitTestResult();
        }

        AZ_INLINE static UnitTestResult AssumeSuccess()
        {
            UnitTestResult u;
            u.m_compiled = u.m_completed = u.m_running = true;
            return u;
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

        virtual void OnTestStart(const AZ::Uuid&) {}
        virtual void OnTestResult(const AZ::Uuid&, const UnitTestResult&) {}
        virtual void OnCheckStateCountChange(const int) {}
    };
    using UnitTestWidgetNotificationBus = AZ::EBus<UnitTestWidgetNotifications>;
}
