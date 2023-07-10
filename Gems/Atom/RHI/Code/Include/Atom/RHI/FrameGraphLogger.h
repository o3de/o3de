/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/FrameSchedulerEnums.h>

namespace AZ::RHI
{
    class FrameGraph;

    class FrameGraphLogger
    {
    public:
        //! Logs the graph to the output console, with the specified verbosity.
        static void Log(const FrameGraph& frameGraph, FrameSchedulerLogVerbosity logVerbosity);
        
        //! Dumps a graph-vis file of the current frame graph to the logs folder.
        static void DumpGraphVis(const FrameGraph& frameGraph);
    };
}
