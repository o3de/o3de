/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Archive/ArchiveTypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace Archive
{
    class ArchiveRequests
    {
    public:
        AZ_RTTI(ArchiveRequests, ArchiveRequestsTypeId);
        virtual ~ArchiveRequests() = default;
        // Put your public methods here
    };

    class ArchiveBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using ArchiveRequestBus = AZ::EBus<ArchiveRequests, ArchiveBusTraits>;
    using ArchiveInterface = AZ::Interface<ArchiveRequests>;

} // namespace Archive
