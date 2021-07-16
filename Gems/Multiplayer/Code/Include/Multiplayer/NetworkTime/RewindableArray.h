/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/array.h>
#include <AzNetworking/DataStructures/IBitset.h>
#include <AzNetworking/Serialization/ISerializer.h>

#include <Multiplayer/NetworkTime/RewindableObject.h>

namespace Multiplayer
{
    //! @class RewindableArray
    //! @brief Data structure that has a compile-time upper bound, provides array semantics and supports network serialization
    template <typename TYPE, uint32_t SIZE>
    class RewindableArray
        : public AZStd::array<RewindableObject<TYPE, Multiplayer::RewindHistorySize>, SIZE>
    {
    public:
        //! Serialization method for array contained rewindable objects
        //! @param serializer ISerializer instance to use for serialization
        //! @return bool true for success, false for serialization failure
        bool Serialize(AzNetworking::ISerializer& serializer);

        //! Serialization method for array contained rewindable objects
        //! @param serializer ISerializer instance to use for serialization
        //! @param deltaRecord Bitset delta record used to detect state change during reconciliation
        //! @return bool true for success, false for serialization failure
        bool Serialize(AzNetworking::ISerializer& serializer, AzNetworking::IBitset &deltaRecord);
    };
}

#include <Multiplayer/NetworkTime/RewindableArray.inl>
