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

#include <Atom/RHI/DeviceBusTraits.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * A bus for frame lifecycle events. The RHI defines a 'Frame' with respect to the
         * Frame Scheduler's full cycle where it takes control of submitting work items to
         * the GPU.
         *
         * Each device has its own frame lifecycle. Therefore, the raw device pointer is used as
         * the bus address. Handlers of this bus should be holding Ptr<RHI::Device> references.
         * This is done implicitly if the class inherits from RHI::DeviceObject.
         */
        class FrameGraph;
        class FrameEventInterface
            : public DeviceBusTraits
        {
        public:
            
            using MutexType = AZStd::recursive_mutex;
            
            /**
             * Called just after the frame scheduler begins a frame.
             */
            virtual void OnFrameBegin() {}

            /**
             * Called just before the frame scheduler compiles the frame graph.
             */
            virtual void OnFrameCompile() {}

            /**
             * Called just after the frame scheduler ends a frame.
             */
            virtual void OnFrameEnd() {}


            /**
             * Called after frame compiles.
             */
            virtual void OnFrameCompileEnd([[maybe_unused]] FrameGraph& frameGraph) {}
        };

        using FrameEventBus = AZ::EBus<FrameEventInterface>;
    }
}
