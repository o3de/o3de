/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AZ
{
    // EBus for EntityActiveSystemComponent
    class EntityActiveSystemRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual size_t GetActiveTypeIndexByName(AZStd::string typeName) = 0;
        virtual size_t GetActiveTypeIndexById(AZ::Crc32 typeNameId) = 0;
    };
    using EntityActiveSystemRequestBus = AZ::EBus<EntityActiveSystemRequests>;
} // namespace AZ
