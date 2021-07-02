/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Driller/DrillerBus.h>

namespace AZ
{
    namespace Debug
    {
        /**
         * Trace messages event handle.
         * All messages are optional (they have default implementation) and you can handle only one at a time.
         * Driller messages are similar to TraceMessages, but do not provide a return value,
         * as we only care about collecting driller messages, not operating on them.
         *
         * We use a driller bus so all messages are sending in exclusive matter no other driller messages
         * can be triggered at that moment, so we already preserve the calling order. You can assume
         * all access code in the driller framework in guarded. You can manually lock the driller mutex are you
         * use by using \ref AZ::Debug::DrillerEBusMutex.
         */
        class TraceMessageDrillerEvents
            : public DrillerEBusTraits
        {
        public:
            virtual ~TraceMessageDrillerEvents() {}

            /// Triggered when a AZ_Assert failed. This is terminating event! (the code will break, crash).
            virtual void OnPreAssert(const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) {}
            virtual void OnAssert(const char* /*message*/) {}
            virtual void OnException(const char* /*message*/) {}
            virtual void OnPreError(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) {}
            virtual void OnError(const char* /*window*/, const char* /*message*/) {}
            virtual void OnPreWarning(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) {}
            virtual void OnWarning(const char* /*window*/, const char* /*message*/) {}
            virtual void OnPrintf(const char* /*window*/, const char* /*message*/) {}
            /**
             * All trace functions you output to anything. So if you want to handle all the output this is the place.
             * You are not given the choice to disable the system output as if you listen at that level you can't make
             * that decision. Otherwise we can trigger an assert without even one line of message send to the console/debugger.
             */
            virtual void OnOutput(const char* /*window*/, const char* /*message*/) {}
        };

        typedef AZ::EBus<TraceMessageDrillerEvents> TraceMessageDrillerBus;
    } // namespace Debug
} // namespace AZ
