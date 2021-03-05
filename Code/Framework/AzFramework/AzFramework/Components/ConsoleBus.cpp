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

#include "ConsoleBus.h"
#include <AzCore/RTTI/BehaviorContext.h>

namespace AzFramework
{
    void ConsoleRequests::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ConsoleRequestBus>("ConsoleRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Utilities")
                ->Event("ExecuteConsoleCommand", &ConsoleRequestBus::Events::ExecuteConsoleCommand)
                ;
        }
    }

    /**
    * Behavior Context forwarder
    */
    class ConsoleNotificationBusBehaviorHandler : public ConsoleNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(ConsoleNotificationBusBehaviorHandler, "{EE90D2DA-9339-4CE6-AF98-AF81E00E2AB3}", AZ::SystemAllocator, OnConsoleCommandExecuted);

        void OnConsoleCommandExecuted(const char* command) override
        {
            Call(FN_OnConsoleCommandExecuted, command);
        }
    };

    void ConsoleNotifications::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ConsoleNotificationBus>("ConsoleNotificationBus")
                ->Attribute(AZ::Script::Attributes::Category, "Utilities")
                ->Handler<ConsoleNotificationBusBehaviorHandler>()
                ;
        }
    }
}
