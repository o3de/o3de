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

#include <GraphCanvas/Types/EntitySaveData.h>
#include <GraphCanvas/Components/EntitySaveDataBus.h>

namespace GraphCanvas
{
    template<typename SaveDataType>
    class ComponentSaveDataInterface
        : public EntitySaveDataRequestBus::Handler
    {
    public:
        AZ_RTTI((ComponentSaveDataInterface, "{0370B379-0C9F-492E-8270-66C9AF0645AF}", SaveDataType));

        void InitSaveDataInterface(const AZ::EntityId& entityId)
        {
            EntitySaveDataRequestBus::Handler::BusConnect(entityId);
        }

        void RegisterIds(const AZ::EntityId& entityId, const GraphId& graphId)
        {
            m_saveData.RegisterIds(entityId, graphId);
        }

        // EntitySaveDataRequestsBus
        void WriteSaveData(EntitySaveDataContainer& saveDataContainer) const
        {
            SaveDataType* saveData = saveDataContainer.FindCreateSaveData<SaveDataType>();

            if (saveData)
            {
                (*saveData) = m_saveData;
            }
        }

        void ReadSaveData(const EntitySaveDataContainer& saveDataContainer)
        {
            SaveDataType* saveData = saveDataContainer.FindSaveData<SaveDataType>();

            if (saveData)
            {
                m_saveData = (*saveData);
            }
        }
        ////

        SaveDataType m_saveData;

    protected:

        virtual void OnSaveDataRead()
        {

        }
    };
}