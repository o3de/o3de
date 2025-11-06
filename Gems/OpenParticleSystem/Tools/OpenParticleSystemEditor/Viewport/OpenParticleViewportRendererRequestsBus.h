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
    class OpenParticleViewportRendererRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void SetViewportDisplayInfo(const AZStd::string& displayInfo) = 0;
        virtual void ClearViewportDisplayInfo() = 0;
    };

    using OpenParticleViewportRendererRequestsBus = AZ::EBus<OpenParticleViewportRendererRequests>;
} // namespace OpenParticleSystemEditor
