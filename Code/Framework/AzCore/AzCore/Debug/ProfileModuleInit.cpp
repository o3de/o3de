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
