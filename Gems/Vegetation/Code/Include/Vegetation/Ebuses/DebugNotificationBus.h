/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/ComponentBus.h>
#include <Vegetation/InstanceData.h>

namespace Vegetation
{
    using TimePoint = AZStd::chrono::steady_clock::time_point;
    using TimeSpan = AZStd::sys_time_t;
    using FilterReasonCount = AZStd::unordered_map<AZStd::string_view, AZ::u32>;

    class DebugNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        static const bool EnableEventQueue = true;
        static const bool EventQueueingActiveByDefault = false;

        // fill reporting

        virtual void FillSectorStart([[maybe_unused]] int sectorX, [[maybe_unused]] int sectorY, [[maybe_unused]] TimePoint timePoint) {};
        virtual void FillSectorEnd([[maybe_unused]] int sectorX, [[maybe_unused]] int sectorY, [[maybe_unused]] TimePoint timePoint, [[maybe_unused]] AZ::u32 unusedClaimPointCount) {};

        virtual void FillAreaStart([[maybe_unused]] AZ::EntityId areaId, [[maybe_unused]] TimePoint timePoint) {};
        virtual void MarkAreaRejectedByMask([[maybe_unused]] AZ::EntityId areaId) {};
        virtual void FillAreaEnd([[maybe_unused]] AZ::EntityId areaId, [[maybe_unused]] TimePoint timePoint, [[maybe_unused]] AZ::u32 unusedClaimPointCount) {};

        virtual void FilterInstance([[maybe_unused]] AZ::EntityId areaId, [[maybe_unused]] AZStd::string_view filterReason) {};
        virtual void CreateInstance([[maybe_unused]] InstanceId instanceId, [[maybe_unused]] AZ::Vector3 position, [[maybe_unused]] AZ::EntityId areaId) {};
        virtual void DeleteInstance([[maybe_unused]] InstanceId instanceId) {};
        virtual void DeleteAllInstances() {};

        // input requests

        virtual void ExportCurrentReport() {}; // writes out the current report to disk, helper for cvars
        virtual void ToggleVisualization() {}; // toggles the 3D visualization in the 3D client, helper for cvars.
    };
    using DebugNotificationBus = AZ::EBus<DebugNotifications>;
}
