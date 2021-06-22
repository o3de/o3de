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

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/RTTI/TypeSafeIntegral.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AzNetworking
{
    class ISerializer;

    AZ_TYPE_SAFE_INTEGRAL(PacketType, uint16_t);

    //! @class IPacket
    //! @brief Base class for all packets.
    //!
    //! IPacket defines an abstract interface that all packets transmitted using AzNetworking must conform to. While there are
    //! a number of core packets used internally by AzNetworking, it is fully possible for end-users to define their own custom
    //! packets using this interface. PacketType should be distinct, and should be greater than
    //! AzNetworking::CorePackets::MAX. The Serialize method allows the IPacket to be used by an
    //! ISerializer to move data between hosts safely and efficiently.
    //!
    //! For more information on the packet format and best practices for extending the packet system, read
    //! [Networking Packets](http://docs.o3de.org/docs/user-guide/networking/packets) on the O3DE documentation site.
    class IPacket
    {
    public:

        AZ_TYPE_INFO(IPacket, "{1B33BFE8-8A4B-44E3-8C8D-3B924093227C}");

        virtual ~IPacket() = default;

        //! Returns the packet type.
        //! @return packet type
        virtual PacketType GetPacketType() const = 0;

        //! Returns an identical copy of the current packet instance, caller assumes ownership.
        //! @return copy of the current packet instance, caller assumes ownership
        virtual AZStd::unique_ptr<IPacket> Clone() const = 0;

        //! Base serialize method for all serializable structures or classes to implement.
        //! @param serializer ISerializer instance to use for serialization
        //! @return boolean true for success, false for serialization failure
        virtual bool Serialize(ISerializer& serializer) = 0;
    };
}

AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(AzNetworking::PacketType);
