/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/unordered_map.h>

#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Types/SceneMemberComponentSaveData.h>

namespace GraphCanvas
{
    class PersistentIdRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = PersistentGraphMemberId;

        virtual AZ::EntityId MapToEntityId() const = 0;
    };

    using PersistentIdRequestBus = AZ::EBus<PersistentIdRequests>;

    class PersistentIdNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = EditorId;

        virtual void OnPersistentIdsRemapped(const AZStd::unordered_map< PersistentGraphMemberId, PersistentGraphMemberId >& persistentIdRemapping) = 0;
    };

    using PersistentIdNotificationBus = AZ::EBus<PersistentIdNotifications>;
    
    class PersistentMemberRequests 
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        
        //! If the persistent graph member was remapped(such as during a copy). This will return the original
        //! value.
        virtual PersistentGraphMemberId GetPreviousGraphMemberId() const = 0;

        virtual PersistentGraphMemberId GetPersistentGraphMemberId() const = 0;
    };
    
    using PersistentMemberRequestBus = AZ::EBus<PersistentMemberRequests>;

    class PersistentIdComponentSaveData;
    AZ_TYPE_INFO_SPECIALIZE(PersistentIdComponentSaveData, "{B1F49A35-8408-40DA-B79E-F1E3B64322CE}");

    class PersistentIdComponentSaveData
        : public SceneMemberComponentSaveData<PersistentIdComponentSaveData>
    {
        friend class PersistentIdComponent;
    public:
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_CLASS_ALLOCATOR(PersistentIdComponentSaveData, AZ::SystemAllocator);

        PersistentIdComponentSaveData()
            : m_persistentId(PersistentGraphMemberId::CreateRandom())
        {
        }

        bool RequiresSave() const override
        {
            return true;
        }

        PersistentGraphMemberId m_persistentId;

    private:
        void RemapId()
        {
            m_persistentId = PersistentGraphMemberId::CreateRandom();
            SignalDirty();
        }
    };
}
