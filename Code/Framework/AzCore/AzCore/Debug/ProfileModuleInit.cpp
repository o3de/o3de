/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/ProfileModuleInit.h>

#ifdef AZ_PROFILE_TELEMETRY
#   include <RADTelemetry/ProfileTelemetryBus.h>
    // Define the per-module RAD Telemetry instance pointer
    struct tm_api;
    tm_api* g_radTmApi;
#endif


namespace AZ
{
    namespace Debug
    {
        void ProfileModuleInit()
        {
#if defined(AZ_PROFILE_TELEMETRY)
            {
                if (!g_radTmApi)
                {
                    using namespace RADTelemetry;
                    ProfileTelemetryRequestBus::BroadcastResult(g_radTmApi, &ProfileTelemetryRequests::GetApiInstance);
                }
            }
#endif
            // Add additional per-DLL required profiler initialization here
        }


        ProfileModuleInitializer::ProfileModuleInitializer()
        {
            ProfilerNotificationBus::Handler::BusConnect();
        }

        ProfileModuleInitializer::~ProfileModuleInitializer()
        {
            ProfilerNotificationBus::Handler::BusDisconnect();
        }

        void ProfileModuleInitializer::OnProfileSystemInitialized()
        {
            ProfileModuleInit();
        }
    }
}
