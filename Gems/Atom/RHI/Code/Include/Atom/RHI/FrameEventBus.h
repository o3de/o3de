/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceBusTraits.h>

namespace AZ::RHI
{
    //! A bus for frame lifecycle events. The RHI defines a 'Frame' with respect to the
    //! Frame Scheduler's full cycle where it takes control of submitting work items to
    //! the GPU.
    //!
    //! Each device has its own frame lifecycle. Therefore, the raw device pointer is used as
    //! the bus address. Handlers of this bus should be holding Ptr<RHI::Device> references.
    //! This is done implicitly if the class inherits from RHI::DeviceObject.
    class FrameGraph;
    class FrameEventInterface
        : public DeviceBusTraits
    {
    public:
            
        using MutexType = AZStd::recursive_mutex;
            
        //! Called just after the frame scheduler begins a frame.
        virtual void OnFrameBegin() {}

        //! Called just before the frame scheduler compiles the frame graph.
        virtual void OnFrameCompile() {}

        //! Called just after the frame scheduler ends a frame.
        virtual void OnFrameEnd() {}

        //! Called after frame compiles.
        virtual void OnFrameCompileEnd([[maybe_unused]] FrameGraph& frameGraph) {}
    };

    using FrameEventBus = AZ::EBus<FrameEventInterface>;
}
