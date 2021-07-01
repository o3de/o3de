/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_VOICECHATSERVICEBUS_H
#define GM_VOICECHATSERVICEBUS_H

#include <GridMate/Memory.h>
#include <AzCore/EBus/EBus.h>
#include <GridMate/String/string.h>

namespace GridMate
{
    class IGridMate;
    class GridMember;

    /*
    * Voice chat service generic bus.
    */
    class VoiceChatServiceInterface
        : public AZ::EBusTraits
    {
    public:
        virtual ~VoiceChatServiceInterface() {}

        // Add chat participant
        virtual void RegisterMember(GridMember* member) = 0;

        // Remove chat participant
        virtual void UnregisterMember(GridMember* member) = 0;

        // EBus Traits
        AZ_CLASS_ALLOCATOR(VoiceChatServiceInterface, GridMateAllocator, 0);

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; ///< Only one service may provide this interface
        typedef IGridMate* BusIdType; ///< Use the GridMate instance as an ID
    };

    typedef AZ::EBus<VoiceChatServiceInterface> VoiceChatServiceBus;
}

#endif
