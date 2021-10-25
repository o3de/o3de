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
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace Debug
    {
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
            : public AZ::EBusTraits
        {
        public:
            // EBusTraits overrides
            static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

            // Allow multiple threads to concurrently make requests
            using MutexType = AZStd::mutex;

            virtual ~ProfilerRequests() = default;

            //! Getter/setter for the profiler active state
            virtual bool IsActive() const = 0;
            virtual void SetActive(bool active) = 0;

            //! Capture a single frame of profiling data
            virtual bool CaptureFrame(const AZStd::string& outputFilePath) = 0;

            //! Starting/ending a multi-frame capture of profiling data
            virtual bool StartCapture(const AZStd::string& outputFilePath) = 0;
            virtual bool EndCapture() = 0;
        };
        using ProfilerRequestBus = AZ::EBus<ProfilerRequests>;
    } // namespace Debug
} // namespace AZ
