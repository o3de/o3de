/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GraphCanvas/Components/EntitySaveDataBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/Components/SceneBus.h>

namespace GraphCanvas
{
    template<class DataType>
    class SceneMemberComponentSaveData
        : public ComponentSaveData
        , public SceneMemberNotificationBus::Handler
        , public EntitySaveDataRequestBus::Handler
    {
    public:
        AZ_RTTI((SceneMemberComponentSaveData, "{2DF9A652-DF5D-43B1-932F-B6A838E36E97}", DataType), ComponentSaveData);
        AZ_CLASS_ALLOCATOR(SceneMemberComponentSaveData, AZ::SystemAllocator);

        SceneMemberComponentSaveData()
        {
        }

        ~SceneMemberComponentSaveData()
        {
            SceneMemberNotificationBus::Handler::BusDisconnect();
            EntitySaveDataRequestBus::Handler::BusDisconnect();
        }

        void Activate(const AZ::EntityId& memberId)
        {
            m_entityId = memberId;

            SceneMemberNotificationBus::Handler::BusConnect(m_entityId);
            EntitySaveDataRequestBus::Handler::BusConnect(m_entityId);
        }

        // SceneMemberNotificationBus::Handler
        void OnSceneSet(const AZ::EntityId& graphId) override
        {
            const AZ::EntityId* ownerId = SceneMemberNotificationBus::GetCurrentBusId();

            if (ownerId)
            {
                RegisterIds((*ownerId), graphId);
            }

            SceneMemberNotificationBus::Handler::BusDisconnect();
        }
        ////

        // EntitySaveDataRequestBus
        void WriteSaveData(EntitySaveDataContainer& saveDataContainer) const override
        {
            AZ_Assert(azrtti_cast<const DataType*>(this) != nullptr, "Cannot automatically read/write to non-derived type in SceneMemberComponentSaveData");

            if (RequiresSave())
            {
                DataType* saveData = saveDataContainer.FindCreateSaveData<DataType>();

                if (saveData)
                {
                    (*saveData) = (*azrtti_cast<const DataType*>(this));
                }
            }
            else
            {
                saveDataContainer.RemoveSaveData<DataType>();
            }
        }

        void ReadSaveData(const EntitySaveDataContainer& saveDataContainer) override
        {
            AZ_Assert(azrtti_cast<DataType*>(this) != nullptr, "Cannot automatically read/write to non-derived type in SceneMemberComponentSaveData");

            DataType* saveData = saveDataContainer.FindSaveDataAs<DataType>();

            if (saveData)
            {
                // The copy is going to destroy the bus state. So we want to ensure we are re-registering for this.
                // For some reason this only affects the graph constructs, and not normal nodes[Appears by dropping the persistent id after every over save/re-open]
                AZ::EntityId originalId = m_entityId;

                (*azrtti_cast<DataType*>(this)) = (*saveData);

                if (originalId.IsValid())
                {
                    Activate(originalId);
                }
            }
        }
        ////

    protected:
        virtual bool RequiresSave() const = 0;

        AZ::EntityId m_entityId;
    };    
}
