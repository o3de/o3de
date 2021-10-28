/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace Debug
    {
        //! settings registry entry for specifying where to output profiler captures
        static constexpr const char* RegistryKey_ProfilerCaptureLocation = "/O3DE/AzCore/Debug/Profiler/CaptureLocation";

        //! fallback value in the event the settings registry isn't ready or doesn't contain the key
        static constexpr const char* ProfilerCaptureLocationFallback = "@user@/Profiler";

        /**
         * ProfilerNotifications provides a profiler event interface that can be used to update listeners on profiler status
         */
        class ProfilerNotifications
            : public AZ::EBusTraits
        {
        public:
            virtual ~ProfilerNotifications() = default;

            //! Notify when the current profiler capture is finished
            //! @param result Set to true if it's finished successfully
            //! @param info The output file path or error information which depends on the return.
            virtual void OnCaptureFinished(bool result, const AZStd::string& info) = 0;
        };
        using ProfilerNotificationBus = AZ::EBus<ProfilerNotifications>;

        /**
        * ProfilerRequests provides an interface for making profiling system requests
        */
        class ProfilerRequests
        {
        public:
            AZ_RTTI(ProfilerRequests, "{90AEC117-14C1-4BAE-9704-F916E49EF13F}");
            virtual ~ProfilerRequests() = default;

            //! Getter/setter for the profiler active state
            virtual bool IsActive() const = 0;
            virtual void SetActive(bool active) = 0;

            //! Capture a single frame of profiling data
            virtual bool CaptureFrame(const AZStd::string& outputFilePath) = 0;

            //! Starting/ending a multi-frame capture of profiling data
            virtual bool StartCapture(AZStd::string outputFilePath) = 0;
            virtual bool EndCapture() = 0;
        };

        using ProfilerSystemInterface = AZ::Interface<ProfilerRequests>;

        //! helper function for getting the profiler capture location from the settings registry that
        //! includes fallback handing in the event the registry value can't be determined
        AZ::IO::FixedMaxPathString GetProfilerCaptureLocation();
    } // namespace Debug
} // namespace AZ
