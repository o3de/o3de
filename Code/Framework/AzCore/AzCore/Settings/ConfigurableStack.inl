/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    //
    // ConfigurableStack
    //

    template<typename StackBaseType>
    TypeId ConfigurableStack<StackBaseType>::GetNodeType() const
    {
        return azrtti_typeid<NodeValue>();
    }

    template<typename StackBaseType>
    auto ConfigurableStack<StackBaseType>::begin() -> Iterator
    {
        return m_nodes.begin();
    }

    template<typename StackBaseType>
    auto ConfigurableStack<StackBaseType>::end() -> Iterator
    {
        return m_nodes.end();
    }

    template<typename StackBaseType>
    auto ConfigurableStack<StackBaseType>::begin() const -> ConstIterator
    {
        return m_nodes.begin();
    }

    template<typename StackBaseType>
    auto ConfigurableStack<StackBaseType>::end() const -> ConstIterator
    {
        return m_nodes.end();
    }

    template<typename StackBaseType>
    bool ConfigurableStack<StackBaseType>::empty() const
    {
        return m_nodes.empty();
    }

    template<typename StackBaseType>
    size_t ConfigurableStack<StackBaseType>::size() const
    {
        return m_nodes.size();
    }

    template<typename StackBaseType>
    void ConfigurableStack<StackBaseType>::clear()
    {
        return m_nodes.clear();
    }

    template<typename StackBaseType>
    void* ConfigurableStack<StackBaseType>::AddNode(AZStd::string name)
    {
        Node& result = m_nodes.emplace_back(AZStd::move(name));
        return &result.second;
    }

    template<typename StackBaseType>
    void* ConfigurableStack<StackBaseType>::AddNode(AZStd::string name, AZStd::string_view target, InsertPosition position)
    {
        auto end = m_nodes.end();
        for (auto it = m_nodes.begin(); it != end; ++it)
        {
            if (it->first == target)
            {
                if (position == InsertPosition::After)
                {
                    ++it;
                }
                auto result = m_nodes.insert(it, Node(AZStd::move(name), {}));
                return &result->second;
            }
        }
        return nullptr;
    }



    //
    // SerializeGenericTypeInfo
    //


    template<typename StackBaseType>
    SerializeGenericTypeInfo<ConfigurableStack<StackBaseType>>::GenericConfigurableStackInfo::GenericConfigurableStackInfo()
        : m_classData{ SerializeContext::ClassData::Create<ConfigurableStackType>(
              "AZ::ConfigurableStack", GetSpecializedTypeId(), Internal::NullFactory::GetInstance(), nullptr, nullptr) }
    {
    }

    template<typename StackBaseType>
    SerializeContext::ClassData* SerializeGenericTypeInfo<ConfigurableStack<StackBaseType>>::GenericConfigurableStackInfo::GetClassData()
    {
        return &m_classData;
    }

    template<typename StackBaseType>
    size_t SerializeGenericTypeInfo<ConfigurableStack<StackBaseType>>::GenericConfigurableStackInfo::GetNumTemplatedArguments()
    {
        return 1;
    }

    template<typename StackBaseType>
    AZ::TypeId SerializeGenericTypeInfo<ConfigurableStack<StackBaseType>>::GenericConfigurableStackInfo::GetTemplatedTypeId(
        [[maybe_unused]] size_t element)
    {
        return SerializeGenericTypeInfo<StackBaseType>::GetClassTypeId();
    }

    template<typename StackBaseType>
    AZ::TypeId SerializeGenericTypeInfo<ConfigurableStack<StackBaseType>>::GenericConfigurableStackInfo::GetSpecializedTypeId() const
    {
        return azrtti_typeid<ConfigurableStackType>();
    }

    template<typename StackBaseType>
    AZ::TypeId SerializeGenericTypeInfo<ConfigurableStack<StackBaseType>>::GenericConfigurableStackInfo::GetGenericTypeId() const
    {
        return TYPEINFO_Uuid();
    }

    template<typename StackBaseType>
    void SerializeGenericTypeInfo<ConfigurableStack<StackBaseType>>::GenericConfigurableStackInfo::Reflect(SerializeContext* serializeContext)
    {
        if (serializeContext)
        {
            serializeContext->RegisterGenericClassInfo(GetSpecializedTypeId(), this, &AnyTypeInfoConcept<ConfigurableStackType>::CreateAny);
            if (serializeContext->FindClassData(azrtti_typeid<AZStd::shared_ptr<StackBaseType>>()) == nullptr)
            {
                serializeContext->RegisterGenericType<AZStd::shared_ptr<StackBaseType>>();
            }
        }
    }
            
    template<typename StackBaseType>
    auto SerializeGenericTypeInfo<ConfigurableStack<StackBaseType>>::GetGenericInfo() -> ClassInfoType*
    {
        return GetCurrentSerializeContextModule().CreateGenericClassInfo<ConfigurableStackType>();
    }

    template<typename StackBaseType>
    AZ::TypeId SerializeGenericTypeInfo<ConfigurableStack<StackBaseType>>::GetClassTypeId()
    {
        return GetGenericInfo()->GetClassData()->m_typeId;
    }
} // namespace AZ

