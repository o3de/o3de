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
        AZ_RTTI(((SceneMemberComponentSaveData<DataType>), "{2DF9A652-DF5D-43B1-932F-B6A838E36E97}", DataType), ComponentSaveData);
        AZ_CLASS_ALLOCATOR(SceneMemberComponentSaveData, AZ::SystemAllocator, 0);

        SceneMemberComponentSaveData()
        {
        }

        ~SceneMemberComponentSaveData() = default;

        void Activate(const AZ::EntityId& memberId)
        {
            SceneMemberNotificationBus::Handler::BusConnect(memberId);
            EntitySaveDataRequestBus::Handler::BusConnect(memberId);
        }

        // SceneMemberNotificationBus::Handler
        void OnSceneSet(const AZ::EntityId& graphId)
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
                (*azrtti_cast<DataType*>(this)) = (*saveData);
            }
        }
        ////

    protected:
        virtual bool RequiresSave() const = 0;
    };    
}