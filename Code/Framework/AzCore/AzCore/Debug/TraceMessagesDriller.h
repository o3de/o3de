/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            const char* GroupName() const override { return "SystemDrillers"; }
            const char* GetName() const override { return "TraceMessagesDriller"; }
            const char* GetDescription() const override { return "Handles all system messages like Assert, Exception, Error, Warning, Printf, etc."; }
            void Start(const Param* params = NULL, int numParams = 0) override;
            void Stop() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // TraceMessagesDrillerBus
            /// Triggered when a AZ_Assert failed. This is terminating event! (the code will break, crash).
            void OnAssert(const char* message) override;
            void OnException(const char* message) override;
            void OnError(const char* window, const char* message) override;
            void OnWarning(const char* window, const char* message) override;
            void OnPrintf(const char* window, const char* message) override;
            //////////////////////////////////////////////////////////////////////////
        };
    } // namespace Debug
} // namespace AZ
