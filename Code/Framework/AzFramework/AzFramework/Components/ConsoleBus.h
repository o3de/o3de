#pragma once
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
#ifndef AZFRAMEWORK_CONSOLE_BUS_H
#define AZFRAMEWORK_CONSOLE_BUS_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/string.h>

namespace AzFramework
{

    /**
    * Event bus that can be used to request commands be executed by the console
    * Only one console can exist at a time, which is why this bus
    * supports only one listener.
    */
    class ConsoleRequests
        : public AZ::EBusTraits
    {
    public:

        virtual ~ConsoleRequests() {}

        //////////////////////////////////////////////////////////////////////////
        /**
        * Overrides the default AZ::EBusTraits handler policy to allow one
        * listener only, because only one console can exist at a time.
        */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void ExecuteConsoleCommand(const char* command) = 0;

        virtual void ResetCVarsToDefaults() = 0;

        static void Reflect(AZ::ReflectContext* context);
    };

    typedef AZ::EBus<ConsoleRequests>  ConsoleRequestBus;

    /**
    * Event bus that can be used to request commands be executed by the console
    * Only one console can exist at a time, which is why this bus
    * supports only one listener.
    */
    class ConsoleNotifications
        : public AZ::EBusTraits
    {
    public:

        virtual ~ConsoleNotifications() {}

        virtual void OnConsoleCommandExecuted(const char* command) = 0;

        static void Reflect(AZ::ReflectContext* context);
    };

    typedef AZ::EBus<ConsoleNotifications>  ConsoleNotificationBus;

} // namespace AzFramework

#endif // AZFRAMEWORK_CONSOLE_BUS_H