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

#include <AzCore/std/containers/array.h>
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
