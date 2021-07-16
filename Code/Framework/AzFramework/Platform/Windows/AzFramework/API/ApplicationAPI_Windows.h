/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/EBus/EBus.h>

namespace AzFramework
{
    class WindowsLifecycleEvents
        : public AZ::EBusTraits
    {
    public:
        // Bus Configuration
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ~WindowsLifecycleEvents() {}

        using Bus = AZ::EBus<WindowsLifecycleEvents>;

        virtual void OnMinimized() {}
        virtual void OnMaximized() {}

        virtual void OnRestored() {}

        virtual void OnKillFocus() {}
        virtual void OnSetFocus() {}
    };
} // namespace AzFramework
