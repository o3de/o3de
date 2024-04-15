/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ::RHI
{
    // Class for telling the framegraph that a fence has been signalled from the CPU
    // The SignalUserSemaphore function must be called when a fence is triggered outside the framegraph
    // E.g. when a CPU operation is performed in the middle of a frame render on which another scope waits
    class FenceTracker
    {
    public:
        virtual void SignalUserSemaphore() = 0;
        virtual ~FenceTracker() = default;
    };
} // namespace AZ::RHI
