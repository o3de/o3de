/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzFramework/Windowing/WindowBus.h>

namespace AZ::RHI
{
    //! Bus used to make general requests to a Graphics Profiler.
    class GraphicsProfilerRequestsInterface
        : public AZ::EBusTraits
    {
    public:
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::Single;
        using MutexType = AZStd::mutex;

        //! Starts a GPU capture for a Native Window. If window is NULL, the current one will be used.
        virtual void StartCapture(const AzFramework::NativeWindowHandle window) = 0;
        //! Ends a GPU capture for a Native Window. If window is NULL, the current one will be used.
        virtual bool EndCapture(const AzFramework::NativeWindowHandle window) = 0;
        //! Triggers a GPU capture.The capture will be taken from the next frame presented to whichever window is considered current.
        virtual void TriggerCapture() = 0;
    };

    using GraphicsProfilerBus = AZ::EBus<GraphicsProfilerRequestsInterface>;
}
