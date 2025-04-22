/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    class ReflectContext;
    class BehaviorContext;
}

namespace AZ::Metrics
{
    enum class EventLoggerId : AZ::u32;
    inline namespace EventPhaseNamespace
    {
        enum class EventPhase : char;
    }
    inline namespace InstantEventScopeNamespace
    {
        enum class InstantEventScope : char;
    }
    struct EventValue;
    struct EventField;

    AZ_TYPE_INFO_SPECIALIZE(EventLoggerId, "{C7D72622-922A-4A9E-8216-12B0B8149A64}");
    // Add AzTypeInfo to the EventPhase enum to allow it to be reflected
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME(EventPhase, "{FBB53668-D422-4787-B8B3-527D8F4C649A}", "EventPhase");
    AZ_TYPE_INFO_SPECIALIZE_WITH_NAME(InstantEventScope, "{F396C6ED-225D-44C4-B2C3-AF5A0D06797F}", "InstantEventScope");

    AZ_TYPE_INFO_SPECIALIZE(EventValue, "{7651AE39-D9F3-4F4B-9907-CEA6AD1DE7EC}");
    AZ_TYPE_INFO_SPECIALIZE(EventField, "{195BA233-37A9-40E1-B9BA-0282F802E4A4}");

    // Utility namespace contains functions/class to simplify calls to record events
    // It contains functions that can perform a lookup of an EventLogger and a record command in single calls
    inline namespace Script
    {
        //! Entry point for reflecting the EventLogger API to the reflection context
        void Reflect(AZ::ReflectContext* context);

        // Reflects the Event Logger API for Scripting
        // Users will interact with the Event logger using an API similiar
        // to the EventLoggerUtils functions
        void ReflectScript(AZ::BehaviorContext& behaviorContext);
    } // inline namespace Utility
} // namespace AZ::Metrics
