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

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzFramework/Windowing/WindowBus.h>

namespace AZ
{
    namespace RPI
    {
        class WindowContextNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = AzFramework::NativeWindowHandle;

            virtual void OnViewportResized(uint32_t width, uint32_t height) = 0;
        };

        using WindowContextNotificationBus = AZ::EBus<WindowContextNotifications>;
    } // namespace RPI
} // namespace AZ
