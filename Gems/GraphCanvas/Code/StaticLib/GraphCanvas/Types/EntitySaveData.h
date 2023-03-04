/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Uuid.h>
#include <AzCore/std/containers/unordered_map.h>

#include <GraphCanvas/Editor/GraphModelBus.h>

namespace GraphCanvas
{
    class ComponentSaveData
    {
    public:
        AZ_RTTI(ComponentSaveData, "{359ACEC7-D0FA-4FC0-8B59-3755BB1A9836}");
        AZ_CLASS_ALLOCATOR(ComponentSaveData, AZ::SystemAllocator);

        ComponentSaveData() = default;
        virtual ~ComponentSaveData() = default;
        
        void operator=(const ComponentSaveData&)
        {
            // Don't want to copy over anything, as the owner/graphid will vary from element to element and we want to maintain
            // whichever one we were created with
        }        

        void RegisterIds(const AZ::EntityId& entityId, const AZ::EntityId& graphId)
        {
            m_ownerId = entityId;
            m_graphId = graphId;
        }

        void SignalDirty()
        {
            if (m_ownerId.IsValid() && m_graphId.IsValid())
            {
                GraphModelRequestBus::Event(m_graphId, &GraphModelRequests::OnSaveDataDirtied, m_ownerId);
            }
        }

    protected:

        const AZ::EntityId& GetOwnerId() const
        {
            return m_ownerId;
        }

        const AZ::EntityId& GetGraphId() const
        {
            return m_graphId;
        }

    private:

        AZ::EntityId m_ownerId;
        AZ::EntityId m_graphId;
    };

    //! This data structure provides a hook for serializing and unserializing whatever data is necessary
    //! For a particular GraphCanvas Entity.
    //!
    //! Used for only writing out pertinent information in saving systems where graphs can be entirely reconstructed
    //! from the saved values.
    class EntitySaveDataContainer
    {
    private:
        friend class GraphCanvasSystemComponent;

    public:
        AZ_TYPE_INFO(EntitySaveDataContainer, "{DCCDA882-AF72-49C3-9AAD-BA601322BFBC}");
        AZ_CLASS_ALLOCATOR(EntitySaveDataContainer, AZ::SystemAllocator);

        template<class DataType>
        static AZ::Uuid GetDataTypeKey()
        {
            return azrtti_typeid<DataType>();
        }

        enum VersionInformation
        {
            NoVersion = -1,
            AddedPersistentId,

            CurrentVersion
        };

        EntitySaveDataContainer()
        {
            
        }
        
        ~EntitySaveDataContainer()
        {
            Clear();
        }

        void Clear()
        {
            for (auto& mapPair : m_entityData)
            {
                delete mapPair.second;
            }
            
            m_entityData.clear();
        }

        template<class DataType>
        DataType* CreateSaveData()
        {
            DataType* retVal = nullptr;
            AZ::Uuid typeId = GetDataTypeKey<DataType>();

            auto mapIter = m_entityData.find(typeId);

            if (mapIter == m_entityData.end())
            {
                retVal = aznew DataType();
                m_entityData[typeId] = retVal;
            }
            else
            {
                AZ_Error("Graph Canvas", false, "Trying to create two save data sources for KeyType (%s)", typeId.ToString<AZStd::string>().c_str());
            }
            
            return retVal;
        }

        template<class DataType>
        void RemoveSaveData()
        {
            AZ::Uuid typeId = GetDataTypeKey<DataType>();

            auto mapIter = m_entityData.find(typeId);

            if (mapIter != m_entityData.end())
            {
                delete mapIter->second;
                m_entityData.erase(mapIter);
            }
        }
        
        template<class DataType>
        DataType* FindSaveData() const
        {
            auto mapIter = m_entityData.find(GetDataTypeKey<DataType>());
            
            if (mapIter != m_entityData.end())
            {
                return azrtti_cast<DataType*>(mapIter->second);
            }
            
            return nullptr;
        }
        
        template<class DataType>
        DataType* FindSaveDataAs() const
        {
            return FindSaveData<DataType>();
        }

        template<class DataType>
        DataType* FindCreateSaveData()
        {
            DataType* dataType = FindSaveDataAs<DataType>();

            if (dataType == nullptr)
            {
                dataType = CreateSaveData<DataType>();
            }

            return dataType;
        }

        bool IsEmpty() const
        {
            return m_entityData.empty();
        }

        void RemoveAll(const AZStd::unordered_set<AZ::Uuid>& exceptionTypes)
        {
            auto mapIter = m_entityData.begin();
            
            while (mapIter != m_entityData.end())
            {
                if (exceptionTypes.find(mapIter->first) == exceptionTypes.end())
                {
                    delete mapIter->second;
                    mapIter = m_entityData.erase(mapIter);
                }
                else
                {
                    ++mapIter;
                }
            }
        }
        
    private:
        AZStd::unordered_map< AZ::Uuid, ComponentSaveData* > m_entityData;
    };
} // namespace GraphCanvas
