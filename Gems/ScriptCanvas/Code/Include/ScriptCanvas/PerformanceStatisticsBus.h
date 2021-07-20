/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
