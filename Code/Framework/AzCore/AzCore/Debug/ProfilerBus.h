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

#pragma once

#include <AzCore/EBus/EBus.h>

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

            virtual void OnProfileSystemInitialized() = 0;
        };
        using ProfilerNotificationBus = AZ::EBus<ProfilerNotifications>;

        enum class ProfileFrameAdvanceType
        {
            Game,
            Render,
            Default = Game
        };

        /**
        * ProfilerRequests provides an interface for making profiling system requests
        */
        class ProfilerRequests
            : public AZ::EBusTraits
        {
        public:
            // Allow multiple threads to concurrently make requests
            using MutexType = AZStd::mutex;

            virtual ~ProfilerRequests() = default;

            virtual bool IsActive() = 0;
            virtual void FrameAdvance(ProfileFrameAdvanceType type) = 0;
        };
        using ProfilerRequestBus = AZ::EBus<ProfilerRequests>;
    }
}
