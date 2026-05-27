/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace OpenParticleSystemEditor
{
    class OpenParticleViewportWidgetRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual AzFramework::CameraState CameraState() = 0;
        virtual AzFramework::ViewportId ViewportId() = 0;
    };
    using OpenParticleViewportWidgetRequestsBus = AZ::EBus<OpenParticleViewportWidgetRequests>;
} // namespace OpenParticleSystemEditor
