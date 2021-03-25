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

namespace ScriptCanvas
{
    namespace Execution
    {
        class PerformanceStatisticsBus
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            virtual void ClearSnaphotStatistics() = 0;
            virtual void TrackAccumulatedStart(AZ::s32 tickCount = -1) = 0;
            virtual void TrackAccumulatedStop() = 0;
            virtual void TrackPerFrameStart() = 0;
            virtual void TrackPerFrameStop() = 0;
        };

        using PerformanceStatisticsEBus = AZ::EBus<PerformanceStatisticsBus>;
    }
}
