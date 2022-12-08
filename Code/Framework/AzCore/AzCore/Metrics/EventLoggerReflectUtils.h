/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    class ReflectContext;
    class BehaviorContext;
}

namespace AZ::Metrics
{
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
