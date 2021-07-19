/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Data/DataRegistry.h>

namespace ScriptCanvas
{
    static AZ::EnvironmentVariable<DataRegistry> s_dataRegistry;
    
    static void RegisterSCTypeTraits(DataRegistry& dataRegistry);

    void InitDataRegistry()
    {
        s_dataRegistry = AZ::Environment::CreateVariable<DataRegistry>(s_dataRegistryName);
        RegisterSCTypeTraits(*s_dataRegistry);
    }

    void ResetDataRegistry()
    {
        s_dataRegistry.Reset();
    }

    AZ::EnvironmentVariable<DataRegistry> GetDataRegistry()
    {
        return AZ::Environment::FindVariable<DataRegistry>(s_dataRegistryName);
    }

    void RegisterSCTypeTraits(DataRegistry& dataRegistry)
    {
        auto it = dataRegistry.m_typeIdTraitMap.emplace(Data::eType::Boolean, Data::MakeTypeErasedTraits<Data::eType::Boolean>());
        AZ_Error("Script Canvas", it.second, "Cannot register a second Trait struct with the same ScriptCanvas type(%u)", it.first->first);
        dataRegistry.m_creatableTypes.insert(it.first->second.m_dataTraits.GetSCType());

        it = dataRegistry.m_typeIdTraitMap.emplace(Data::eType::EntityID, Data::MakeTypeErasedTraits<Data::eType::EntityID>());
        AZ_Error("Script Canvas", it.second, "Cannot register a second Trait struct with the same ScriptCanvas type(%u)", it.first->first);
        dataRegistry.m_creatableTypes.insert(it.first->second.m_dataTraits.GetSCType());

        it = dataRegistry.m_typeIdTraitMap.emplace(Data::eType::Number, Data::MakeTypeErasedTraits<Data::eType::Number>());
        AZ_Error("Script Canvas", it.second, "Cannot register a second Trait struct with the same ScriptCanvas type(%u)", it.first->first);
        dataRegistry.m_creatableTypes.insert(it.first->second.m_dataTraits.GetSCType());

        it = dataRegistry.m_typeIdTraitMap.emplace(Data::eType::String, Data::MakeTypeErasedTraits<Data::eType::String>());
        AZ_Error("Script Canvas", it.second, "Cannot register a second Trait struct with the same ScriptCanvas type(%u)", it.first->first);
        dataRegistry.m_creatableTypes.insert(it.first->second.m_dataTraits.GetSCType());

        it = dataRegistry.m_typeIdTraitMap.emplace(Data::eType::Quaternion, Data::MakeTypeErasedTraits<Data::eType::Quaternion>());
        AZ_Error("Script Canvas", it.second, "Cannot register a second Trait struct with the same ScriptCanvas type(%u)", it.first->first);
        dataRegistry.m_creatableTypes.insert(it.first->second.m_dataTraits.GetSCType());

        it = dataRegistry.m_typeIdTraitMap.emplace(Data::eType::Transform, Data::MakeTypeErasedTraits<Data::eType::Transform>());
        AZ_Error("Script Canvas", it.second, "Cannot register a second Trait struct with the same ScriptCanvas type(%u)", it.first->first);
        dataRegistry.m_creatableTypes.insert(it.first->second.m_dataTraits.GetSCType());

        it = dataRegistry.m_typeIdTraitMap.emplace(Data::eType::Vector2, Data::MakeTypeErasedTraits<Data::eType::Vector2>());
        AZ_Error("Script Canvas", it.second, "Cannot register a second Trait struct with the same ScriptCanvas type(%u)", it.first->first);
        dataRegistry.m_creatableTypes.insert(it.first->second.m_dataTraits.GetSCType());

        it = dataRegistry.m_typeIdTraitMap.emplace(Data::eType::Vector3, Data::MakeTypeErasedTraits<Data::eType::Vector3>());
        AZ_Error("Script Canvas", it.second, "Cannot register a second Trait struct with the same ScriptCanvas type(%u)", it.first->first);
        dataRegistry.m_creatableTypes.insert(it.first->second.m_dataTraits.GetSCType());

        it = dataRegistry.m_typeIdTraitMap.emplace(Data::eType::Vector4, Data::MakeTypeErasedTraits<Data::eType::Vector4>());
        AZ_Error("Script Canvas", it.second, "Cannot register a second Trait struct with the same ScriptCanvas type(%u)", it.first->first);
        dataRegistry.m_creatableTypes.insert(it.first->second.m_dataTraits.GetSCType());

        it = dataRegistry.m_typeIdTraitMap.emplace(Data::eType::AABB, Data::MakeTypeErasedTraits<Data::eType::AABB>());
        AZ_Error("Script Canvas", it.second, "Cannot register a second Trait struct with the same ScriptCanvas type(%u)", it.first->first);
        dataRegistry.m_creatableTypes.insert(it.first->second.m_dataTraits.GetSCType());

        it = dataRegistry.m_typeIdTraitMap.emplace(Data::eType::Color, Data::MakeTypeErasedTraits<Data::eType::Color>());
        AZ_Error("Script Canvas", it.second, "Cannot register a second Trait struct with the same ScriptCanvas type(%u)", it.first->first);
        dataRegistry.m_creatableTypes.insert(it.first->second.m_dataTraits.GetSCType());

        it = dataRegistry.m_typeIdTraitMap.emplace(Data::eType::CRC, Data::MakeTypeErasedTraits<Data::eType::CRC>());
        AZ_Error("Script Canvas", it.second, "Cannot register a second Trait struct with the same ScriptCanvas type(%u)", it.first->first);
        dataRegistry.m_creatableTypes.insert(it.first->second.m_dataTraits.GetSCType());

        it = dataRegistry.m_typeIdTraitMap.emplace(Data::eType::Matrix3x3, Data::MakeTypeErasedTraits<Data::eType::Matrix3x3>());
        AZ_Error("Script Canvas", it.second, "Cannot register a second Trait struct with the same ScriptCanvas type(%u)", it.first->first);
        dataRegistry.m_creatableTypes.insert(it.first->second.m_dataTraits.GetSCType());

        it = dataRegistry.m_typeIdTraitMap.emplace(Data::eType::Matrix4x4, Data::MakeTypeErasedTraits<Data::eType::Matrix4x4>());
        AZ_Error("Script Canvas", it.second, "Cannot register a second Trait struct with the same ScriptCanvas type(%u)", it.first->first);
        dataRegistry.m_creatableTypes.insert(it.first->second.m_dataTraits.GetSCType());

        it = dataRegistry.m_typeIdTraitMap.emplace(Data::eType::Plane, Data::MakeTypeErasedTraits<Data::eType::Plane>());
        AZ_Error("Script Canvas", it.second, "Cannot register a second Trait struct with the same ScriptCanvas type(%u)", it.first->first);
        dataRegistry.m_creatableTypes.insert(it.first->second.m_dataTraits.GetSCType());

        it = dataRegistry.m_typeIdTraitMap.emplace(Data::eType::OBB, Data::MakeTypeErasedTraits<Data::eType::OBB>());
        AZ_Error("Script Canvas", it.second, "Cannot register a second Trait struct with the same ScriptCanvas type(%u)", it.first->first);
        dataRegistry.m_creatableTypes.insert(it.first->second.m_dataTraits.GetSCType());

        // BehaviorContext traits is slightly different than built traits
        it = dataRegistry.m_typeIdTraitMap.emplace(Data::eType::BehaviorContextObject, Data::MakeTypeErasedTraits<Data::eType::BehaviorContextObject>());
        AZ_Error("Script Canvas", it.second, "Cannot register a second Trait struct with the same ScriptCanvas type(%u)", it.first->first);
    }

    void DataRegistry::RegisterType(const AZ::TypeId& typeId, TypeProperties typeProperties, Createability registration)
    {
        Data::Type behaviorContextType = Data::FromAZType(typeId);
        if (behaviorContextType.GetType() == Data::eType::BehaviorContextObject && !behaviorContextType.GetAZType().IsNull())
        {
            if (registration == Createability::SlotAndVariable)
            {
                if (m_creatableTypes.find(behaviorContextType) == m_creatableTypes.end())
                {
                    m_creatableTypes[behaviorContextType] = typeProperties;
                }
            }
            else if (registration == Createability::SlotOnly)
            {
                if (m_slottableTypes.find(behaviorContextType) == m_slottableTypes.end())
                {
                    m_slottableTypes[behaviorContextType] = typeProperties;
                }
            }
        }
    }

    void DataRegistry::UnregisterType(const AZ::TypeId& typeId)
    {
        Data::Type behaviorContextType = Data::FromAZType(typeId);
        if (behaviorContextType.GetType() == Data::eType::BehaviorContextObject && !behaviorContextType.GetAZType().IsNull())
        {
            m_creatableTypes.erase(behaviorContextType);
        }
    }

    bool DataRegistry::IsUseableInSlot(const Data::Type& scType) const
    {
        return m_creatableTypes.contains(scType) || m_slottableTypes.contains(scType);
    }

    bool DataRegistry::IsUseableInSlot(const AZ::TypeId& typeId) const
    {
        Data::Type scType = Data::FromAZType(typeId);
        return IsUseableInSlot(scType);
    }
}
