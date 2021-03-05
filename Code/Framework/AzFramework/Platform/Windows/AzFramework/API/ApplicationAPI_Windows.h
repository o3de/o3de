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
