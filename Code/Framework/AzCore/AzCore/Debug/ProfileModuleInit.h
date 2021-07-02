/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/ProfilerBus.h>

namespace AZ
{
    namespace Debug
    {
        //! Perform any required per-module initialization of the current profiler
        void ProfileModuleInit();


        /*!
        * ProfileModuleInitializer
        * Helper class that calls ProfileModuleInit when OnProfileSystemInitialized is fired.
        */
        class ProfileModuleInitializer
            : private AZ::Debug::ProfilerNotificationBus::Handler
        {
        public:
            ProfileModuleInitializer();
            ~ProfileModuleInitializer() override;

        private:
            void OnProfileSystemInitialized() override;
        };
    }
}
