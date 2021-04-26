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

#include <Atom/RHI.Reflect/FrameSchedulerEnums.h>

namespace AZ
{
    namespace RHI
    {
        class FrameGraph;

        class FrameGraphLogger
        {
        public:
            /// Logs the graph to the output console, with the specified verbosity.
            static void Log(const FrameGraph& frameGraph, FrameSchedulerLogVerbosity logVerbosity);

            /// Dumps a graph-vis file of the current frame graph to the logs folder.
            static void DumpGraphVis(const FrameGraph& frameGraph);
        };
    }
}
