/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/NetworkTime/INetworkTime.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Console/ILogger.h>

namespace Multiplayer
{
    //! @class RewindableObject
    //! @brief A simple serializable data container that keeps a history of previous values, and can fetch those old values on request.
    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    class RewindableObject
    {
    public:

        RewindableObject() = default;

        //! Constructor.
        //! @param value base type value to construct from
        RewindableObject(const BASE_TYPE& value);

        //! Copy construct from underlying base type.
        //! @param value base type value to construct from
        //! @param owningConnectionId the entity id of the owning object
        explicit RewindableObject(const BASE_TYPE& value, AzNetworking::ConnectionId owningConnectionId);

        //! Copy construct from another rewindable history buffer.
        //! @param rhs rewindable history buffer to construct from
        RewindableObject(const RewindableObject& rhs);

        //! Assignment from underlying base type.
        //! @param rhs base type value to assign from
        RewindableObject& operator = (const BASE_TYPE& rhs);

        //! Assignment from rewindable history buffer.
        //! @param rhs rewindable history buffer to assign from
        RewindableObject& operator = (const RewindableObject& rhs);

        //! Sets the owning connectionId for the given rewindable object instance.
        //! @param owningConnectionId the new connectionId to use as the owning connectionId.
        void SetOwningConnectionId(AzNetworking::ConnectionId owningConnectionId);

        //! Const base type operator.
        //! @return value in const base type form
        operator const BASE_TYPE&() const;

        //! Const base type retriever.
        //! @return value in const base type form
        const BASE_TYPE& Get() const;

        //! Const base type retriever for one host frame behind Get() when contextually appropriate, otherwise identical to Get().
        //! @return value in const base type form
        const BASE_TYPE& GetPrevious() const;

        //! Base type retriever.
        //! @return value in base type form
        BASE_TYPE& Modify();

        //! Equality operator.
        //! @param rhs base type value to compare against
        //! @return boolean true if this == rhs
        bool operator == (const BASE_TYPE& rhs) const;

        //! Inequality operator.
        //! @param rhs base type value to compare against
        //! @return boolean true if this != rhs
        bool operator != (const BASE_TYPE& rhs) const;

        //! Base serialize method for all serializable structures or classes to implement
        //! @param serializer ISerializer instance to use for serialization
        //! @return boolean true for success, false for serialization failure
        bool Serialize(AzNetworking::ISerializer& serializer);

    private:

        //! Returns what the appropriate current time is for this rewindable property.
        //! @return the appropriate current time for this rewindable property
        HostFrameId GetCurrentTimeForProperty() const;

        //! Returns what the appropriate previous time is for this rewindable property.
        //! @return the appropriate previous time for this rewindable property 
        HostFrameId GetPreviousTimeForProperty() const;

        //! Updates the latest value for this object instance, if frameTime represents a current or future time.
        //! Any attempts to set old values on the object will fail
        //! @param value     the new value to set in the object history
        //! @param frameTime the time to set the value for
        void SetValueForTime(const BASE_TYPE& value, HostFrameId frameTime);

        //! Const value accessor, returns the correct value for the provided input time.
        //! @param frameTime the frame time to return the associated value for
        //! @return value given the current input time
        const BASE_TYPE& GetValueForTime(HostFrameId frameTime) const;

        //! Helper method to compute clamped array index values accounting for the offset head index.
        AZStd::size_t GetOffsetIndex(AZStd::size_t absoluteIndex) const;

        AZStd::array<BASE_TYPE, REWIND_SIZE> m_history;
        AzNetworking::ConnectionId m_owningConnectionId = AzNetworking::InvalidConnectionId;
        HostFrameId m_headTime = HostFrameId{0};
        uint32_t m_headIndex = 0;
    };
}

namespace AZ
{
    AZ_TYPE_INFO_TEMPLATE(Multiplayer::RewindableObject, "{B2937B44-FEE1-4277-B1E0-863DE76D363F}", AZ_TYPE_INFO_TYPENAME, AZ_TYPE_INFO_AUTO);
}

#include <Multiplayer/NetworkTime/RewindableObject.inl>
