/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Driller/Driller.h>
#include <AzCore/Debug/TraceMessagesDrillerBus.h>

namespace AZ
{
    namespace Debug
    {
        /**
         * Trace messages driller class
         */
        class TraceMessagesDriller
            : public Driller
            , public TraceMessageDrillerBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(TraceMessagesDriller, OSAllocator, 0)

        protected:
            //////////////////////////////////////////////////////////////////////////
            // Driller
            virtual const char* GroupName() const { return "SystemDrillers"; }
            virtual const char* GetName() const { return "TraceMessagesDriller"; }
            virtual const char* GetDescription() const { return "Handles all system messages like Assert, Exception, Error, Warning, Printf, etc."; }
            virtual void Start(const Param* params = NULL, int numParams = 0);
            virtual void Stop();
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // TraceMessagesDrillerBus
            /// Triggered when a AZ_Assert failed. This is terminating event! (the code will break, crash).
            virtual void OnAssert(const char* message);
            virtual void OnException(const char* message);
            virtual void OnError(const char* window, const char* message);
            virtual void OnWarning(const char* window, const char* message);
            virtual void OnPrintf(const char* window, const char* message);
            //////////////////////////////////////////////////////////////////////////
        };
    } // namespace Debug
} // namespace AZ
