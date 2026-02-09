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
    //! Entities have a bitmask that represents their activation state, where the entity is only activated if all
    //! bits are set (true by default).   You can use this bus to reserve a bit in the bitmask 
    //! for your system to cause entities to become inactive for your own purposes (LOD, visibilty, etc.)
    class EntityActiveSystemRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Reserves a bit in the bitset for the given name and returns the index of it.
        //! If the name has already been reserved, returns the existing index without registering a new one
        virtual size_t GetActiveTypeIndexByName(AZStd::string typeName) = 0;

        //! Reserves a bit in the bitset for a given name as a Crc32 and returns the index of it.
        //! If the name has already been reserved, returns the existing index without registering a new one.
        virtual size_t GetActiveTypeIndexById(AZ::Crc32 typeNameId) = 0;
    };
    using EntityActiveSystemRequestBus = AZ::EBus<EntityActiveSystemRequests>;
} // namespace AZ
