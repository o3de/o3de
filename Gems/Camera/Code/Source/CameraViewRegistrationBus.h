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

#include <Atom/RPI.Public/Base.h>
#include <AzCore/Component/EntityId.h>

namespace Camera
{
    class CameraViewRegistrationRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void SetViewForEntity([[maybe_unused]] const AZ::EntityId& id, [[maybe_unused]] AZ::RPI::ViewPtr view) {}
        virtual AZ::RPI::ViewPtr GetViewForEntity([[maybe_unused]]const AZ::EntityId& id) { return {}; }
    };

    using CameraViewRegistrationRequestsBus = AZ::EBus<CameraViewRegistrationRequests>;
} //namespace Camera
