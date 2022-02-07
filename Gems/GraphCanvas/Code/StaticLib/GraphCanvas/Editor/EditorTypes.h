/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/unordered_set.h>

namespace GraphCanvas
{
    typedef AZ::Crc32 EditorId;
    typedef AZ::EntityId GraphId;

    typedef AZ::EntityId ViewId;

    typedef AZ::EntityId SlotId;
    typedef AZ::EntityId NodeId;
    typedef AZ::EntityId ConnectionId;
    typedef AZ::EntityId BookmarkId;

    typedef AZ::EntityId DockWidgetId;

    typedef AZ::EntityId GraphicsEffectId;

    typedef AZ::Uuid PersistentGraphMemberId;

    typedef AZ::Crc32 ExtenderId;

    namespace Styling
    {
        // Style of curve for connection lines
        enum class ConnectionCurveType : AZ::u32
        {
            Straight,
            Curved
        };
    }

    enum class DataSlotType
    {
        Unknown,

        // These are options that can be used on most DataSlots
        Value,
        Reference
    };

    enum class DataValueType
    {
        Unknown,
        
        Primitive,

        // Container types
        Container
    };

    // Used to signal a drag/drop state
    enum class DragDropState
    {
        Unknown,
        Idle,
        Valid,
        Invalid
    };

    // Signals out which side of the connection is attempting to be moved.
    enum class ConnectionMoveType
    {
        Unknown,
        Source,
        Target
    };

    enum class ListingType
    {
        Unknown,
        InclusiveList,
        ExclusiveList
    };

    template<typename T>
    class TypeListingConfiguration
    {
    public:
        TypeListingConfiguration() = default;
        ~TypeListingConfiguration() = default;

        bool AllowsType(const T& type) const
        {
            size_t count = m_listing.count(type);

            return (count > 0 && m_listingType == ListingType::ExclusiveList) || (count == 0 && m_listingType == ListingType::InclusiveList);
        }

        ListingType             m_listingType  = ListingType::ExclusiveList;
        AZStd::unordered_set<T> m_listing;

    private:
    };

    struct ConnectionValidationTooltip
    {
        bool operator()() const
        {
            return m_isValid;
        }

        bool m_isValid;

        NodeId m_nodeId;
        SlotId m_slotId;

        AZStd::string m_failureReason;
    };

    typedef AZ::Outcome<void, AZStd::string> CanHandleMimeEventOutcome;
}
