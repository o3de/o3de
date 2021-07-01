/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#ifdef AZ_PROFILE_TELEMETRY

#include <AzCore/base.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Debug/ProfilerBus.h>
#include <AzCore/EBus/EBus.h>

struct tm_api;

namespace RADTelemetry
{
    class ProfileTelemetryRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ~ProfileTelemetryRequests() = default;

        virtual void ToggleEnabled() = 0;

        virtual void SetAddress(const char* address, AZ::u16 port) = 0;

        virtual void SetCaptureMask(AZ::Debug::ProfileCategoryPrimitiveType mask) = 0;

        virtual void SetFrameAdvanceType(AZ::Debug::ProfileFrameAdvanceType type) = 0;

        virtual AZ::Debug::ProfileCategoryPrimitiveType GetCaptureMask() = 0;

        virtual AZ::Debug::ProfileCategoryPrimitiveType GetDefaultCaptureMask() = 0;

        virtual tm_api* GetApiInstance() = 0;
    };

    using ProfileTelemetryRequestBus = AZ::EBus<ProfileTelemetryRequests>;
}

#endif
