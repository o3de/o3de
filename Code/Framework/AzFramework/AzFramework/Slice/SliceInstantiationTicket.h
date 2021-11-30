/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/string/string.h>

namespace AzFramework
{
    using EntityContextId = AZ::Uuid;

    /**
     * Identifies an asynchronous slice instantiation request.
     * This can be used to get the results of a slice instantiation request.
     */
    class SliceInstantiationTicket
    {
    public:
        /**
         * Enables this class to be identified across modules and serialized into
         * different contexts.
         */
        AZ_TYPE_INFO(SliceInstantiationTicket, "{E6E7C0C5-07C9-44BB-A38C-930431948667}");

        /**
         * Creates a slice instantiation ticket.
         * @param contextId The ID of the entity context to create the slice in.
         * @param requestId An ID for the slice instantiation ticket.
         */
        explicit SliceInstantiationTicket(const EntityContextId& contextId = AZ::Uuid::CreateNull(), AZ::u64 requestId = 0);

        /**
         * Overloads the equality operator to indicate that two slice instantiation
         * tickets are equal if they have the same entity context ID and request ID.
         * @param rhs The slice instantiation ticket you want to compare to the
         * current ticket.
         * @return Returns true if the entity context ID and the request ID of the
         * slice instantiation tickets are equal.
         */
        bool operator==(const SliceInstantiationTicket& rhs) const;

        /**
         * Overloads the inequality operator to indicate that two slice instantiation
         * tickets are not equal do not have the same entity context ID and request ID.
         * @param rhs The slice instantiation ticket you want to compare to the
         * current ticket.
         * @return Returns true if the entity context ID and the request ID of the
         * slice instantiation tickets are not equal.
         */
        bool operator!=(const SliceInstantiationTicket& rhs) const;

        /**
         * Overloads the boolean operator to indicate that a slice instantiation
         * ticket is true if its request ID is valid.
         * @return Returns true if the request ID is not equal to zero.
         */
        bool IsValid() const;

        AZStd::string ToString() const;

        const EntityContextId& GetContextId() const;

        AZ::u64 GetRequestId() const;

    private:
        /**
         * The ID of the entity context to create the slice in.
         */
        EntityContextId m_contextId;

        /**
         * An ID for the slice instantiation ticket.
         */
        AZ::u64 m_requestId;
    };
}

namespace AZStd
{
    /**
     * Enables slice instantiation tickets to be keys in hashed data structures.
     */
    template<>
    struct hash<AzFramework::SliceInstantiationTicket>
    {
        inline size_t operator()(const AzFramework::SliceInstantiationTicket& value) const
        {
            size_t result = value.GetContextId().GetHash();
            hash_combine(result, value.GetRequestId());
            return result;
        }
    };
}
