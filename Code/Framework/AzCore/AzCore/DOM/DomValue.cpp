/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomPath.h>
#include <AzCore/DOM/DomValue.h>
#include <AzCore/DOM/DomValueWriter.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ::Dom
{
    namespace Internal
    {
        template<class T>
        AZStd::shared_ptr<T>& CheckCopyOnWrite(AZStd::shared_ptr<T>& refCountedPointer)
        {
            if (refCountedPointer.use_count() == 1)
            {
                return refCountedPointer;
            }
            else
            {
                refCountedPointer = AZStd::allocate_shared<T>(ValueAllocator_for_std_t(), *refCountedPointer);
                return refCountedPointer;
            }
        }

        template<class TestType>
        constexpr size_t GetTypeIndexInternal(size_t index = 0);

        template<class TestType, class FirstType, class... Rest>
        constexpr size_t GetTypeIndexInternal(size_t index = 0)
        {
            if constexpr (AZStd::is_same_v<TestType, FirstType>)
            {
                return index;
            }
            else
            {
                return GetTypeIndexInternal<TestType, Rest...>(index + 1);
            }
        }

        template<typename T>
        struct ExtractTypeArgs
        {
        };

        template<template<typename...> class TypeToExtract, typename... Args>
        struct ExtractTypeArgs<TypeToExtract<Args...>>
        {
            template<class T>
            static constexpr size_t GetTypeIndex()
            {
                return GetTypeIndexInternal<T, Args...>();
            }
        };
    } // namespace Internal

    // Helper function, looks up the index of a type within Value::m_value's storage
    template<class T>
    constexpr size_t GetTypeIndex()
    {
        return Internal::ExtractTypeArgs<Value::ValueType>::GetTypeIndex<T>();
    }

    const Array::ContainerType& Array::GetValues() const
    {
        return m_values;
    }

    const Object::ContainerType& Object::GetValues() const
    {
        return m_values;
    }

    Node::Node(AZ::Name name)
        : m_name(AZStd::move(name))
    {
    }

    AZ::Name Node::GetName() const
    {
        return m_name;
    }

    void Node::SetName(AZ::Name name)
    {
        m_name = AZStd::move(name);
    }

    Object::ContainerType& Node::GetProperties()
    {
        return m_properties;
    }

    const Object::ContainerType& Node::GetProperties() const
    {
        return m_properties;
    }

    Array::ContainerType& Node::GetChildren()
    {
        return m_children;
    }

    const Array::ContainerType& Node::GetChildren() const
    {
        return m_children;
    }

    Value::Value(SharedStringType sharedString)
        : m_value(AZStd::move(sharedString))
    {
    }

    Value::Value(const Value& value)
        : m_value(value.m_value)
    {
    }

    Value::Value(Value&& value) noexcept
    {
        memcpy(this, &value, sizeof(Value));
        memset(&value, 0, sizeof(Value));
    }

    Value::Value(AZStd::string_view stringView, bool copy)
    {
        if (copy)
        {
            CopyFromString(stringView);
        }
        else
        {
            SetString(stringView);
        }
    }

    Value::Value(AZStd::any opaqueValue)
        : m_value(AZStd::allocate_shared<AZStd::any>(ValueAllocator_for_std_t(), AZStd::move(opaqueValue)))
    {
    }

    Value Value::FromOpaqueValue(const AZStd::any& value)
    {
        return Value(value);
    }

    Value Value::CreateNode(AZ::Name nodeName)
    {
        Value result(Type::Node);
        result.SetNodeName(AZStd::move(nodeName));
        return result;
    }

    Value Value::CreateNode(AZStd::string_view nodeName)
    {
        return CreateNode(AZ::Name(nodeName));
    }

    Value::Value(int8_t value)
        : m_value(static_cast<AZ::s64>(value))
    {
    }

    Value::Value(uint8_t value)
        : m_value(static_cast<AZ::u64>(value))
    {
    }

    Value::Value(int16_t value)
        : m_value(static_cast<AZ::s64>(value))
    {
    }

    Value::Value(uint16_t value)
        : m_value(static_cast<AZ::u64>(value))
    {
    }

    Value::Value(int32_t value)
        : m_value(static_cast<AZ::s64>(value))
    {
    }

    Value::Value(uint32_t value)
        : m_value(static_cast<AZ::u64>(value))
    {
    }

    Value::Value(long value)
        : m_value(static_cast<AZ::s64>(value))
    {
    }

    Value::Value(unsigned long value)
        : m_value(static_cast<AZ::u64>(value))
    {
    }

    Value::Value(long long value)
        : m_value(static_cast<AZ::s64>(value))
    {
    }

    Value::Value(unsigned long long value)
        : m_value(static_cast<AZ::u64>(value))
    {
    }

    Value::Value(float value)
        : m_value(static_cast<double>(value))
    {
    }

    Value::Value(double value)
        : m_value(value)
    {
    }

    Value::Value(bool value)
        : m_value(value)
    {
    }

    Value::Value(Type type)
    {
        switch (type)
        {
        case Type::Null:
            // Null is the default initialized value
            break;
        case Type::Bool:
            m_value = false;
            break;
        case Type::Object:
            SetObject();
            break;
        case Type::Array:
            SetArray();
            break;
        case Type::String:
            SetString("");
            break;
        case Type::Int64:
            m_value = AZ::s64{};
            break;
        case Type::Uint64:
            m_value = AZ::u64{};
            break;
        case Type::Double:
            m_value = double{};
            break;
        case Type::Node:
            SetNode("");
            break;
        case Type::Opaque:
            AZ_Assert(false, "AZ::Dom::Value may not be constructed with an empty opaque type");
            break;
        }
    }

    Value& Value::operator=(const Value& other)
    {
        m_value = other.m_value;
        return *this;
    }

    Value& Value::operator=(Value&& other) noexcept
    {
        SetNull();
        memcpy(this, &other, sizeof(Value));
        memset(&other, 0, sizeof(Value));
        return *this;
    }

    bool Value::operator==(const Value& rhs) const
    {
        if (IsString() && rhs.IsString())
        {
            return GetString() == rhs.GetString();
        }
        else
        {
            return m_value == rhs.m_value;
        }
    }

    bool Value::operator!=(const Value& rhs) const
    {
        return !operator==(rhs);
    }

    void Value::Swap(Value& other) noexcept
    {
        AZStd::aligned_storage_for_t<Value> temp;
        memcpy(&temp, this, sizeof(Value));
        memcpy(this, &other, sizeof(Value));
        memcpy(&other, &temp, sizeof(Value));
    }

    Type Dom::Value::GetType() const
    {
        switch (m_value.index())
        {
        case GetTypeIndex<AZStd::monostate>():
            return Type::Null;
        case GetTypeIndex<AZ::s64>():
            return Type::Int64;
        case GetTypeIndex<AZ::u64>():
            return Type::Uint64;
        case GetTypeIndex<double>():
            return Type::Double;
        case GetTypeIndex<bool>():
            return Type::Bool;
        case GetTypeIndex<AZStd::string_view>():
        case GetTypeIndex<SharedStringType>():
        case GetTypeIndex<ShortStringType>():
            return Type::String;
        case GetTypeIndex<ObjectPtr>():
            return Type::Object;
        case GetTypeIndex<ArrayPtr>():
            return Type::Array;
        case GetTypeIndex<NodePtr>():
            return Type::Node;
        case GetTypeIndex<AZStd::shared_ptr<AZStd::any>>():
            return Type::Opaque;
        }
        AZ_Assert(false, "AZ::Dom::Value::GetType: m_value has an unexpected type");
        return Type::Null;
    }

    bool Value::IsNull() const
    {
        return AZStd::holds_alternative<AZStd::monostate>(m_value);
    }

    bool Value::IsFalse() const
    {
        const bool* value = AZStd::get_if<bool>(&m_value);
        return value != nullptr ? !(*value) : false;
    }

    bool Value::IsTrue() const
    {
        const bool* value = AZStd::get_if<bool>(&m_value);
        return value != nullptr ? *value : false;
    }

    bool Value::IsBool() const
    {
        return AZStd::holds_alternative<bool>(m_value);
    }

    bool Value::IsNode() const
    {
        return AZStd::holds_alternative<NodePtr>(m_value);
    }

    bool Value::IsObject() const
    {
        return AZStd::holds_alternative<ObjectPtr>(m_value);
    }

    bool Value::IsArray() const
    {
        return AZStd::holds_alternative<ArrayPtr>(m_value);
    }

    bool Value::IsOpaqueValue() const
    {
        return AZStd::holds_alternative<OpaqueStorageType>(m_value);
    }

    bool Value::IsNumber() const
    {
        return AZStd::visit(
            [](auto&& value) -> bool
            {
                using CurrentType = AZStd::decay_t<decltype(value)>;
                if constexpr (AZStd::is_same_v<CurrentType, AZ::s64>)
                {
                    return true;
                }
                else if constexpr (AZStd::is_same_v<CurrentType, AZ::u64>)
                {
                    return true;
                }
                else if constexpr (AZStd::is_same_v<CurrentType, double>)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            },
            m_value);
    }

    bool Value::IsInt() const
    {
        return AZStd::holds_alternative<AZ::s64>(m_value);
    }

    bool Value::IsUint() const
    {
        return AZStd::holds_alternative<AZ::u64>(m_value);
    }

    bool Value::IsDouble() const
    {
        return AZStd::holds_alternative<double>(m_value);
    }

    bool Value::IsString() const
    {
        return AZStd::visit(
            [](auto&& value) -> bool
            {
                using CurrentType = AZStd::decay_t<decltype(value)>;
                if constexpr (AZStd::is_same_v<CurrentType, AZStd::string_view>)
                {
                    return true;
                }
                else if constexpr (AZStd::is_same_v<CurrentType, SharedStringType>)
                {
                    return true;
                }
                else if constexpr (AZStd::is_same_v<CurrentType, ShortStringType>)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            },
            m_value);
    }

    Value& Value::SetObject()
    {
        m_value = AZStd::allocate_shared<Object>(ValueAllocator_for_std_t());
        return *this;
    }

    const Node& Value::GetNodeInternal() const
    {
        AZ_Assert(GetType() == Type::Node, "AZ::Dom::Value: attempted to retrieve a node from a non-node value");
        return *AZStd::get<NodePtr>(m_value);
    }

    Node& Value::GetNodeInternal()
    {
        AZ_Assert(GetType() == Type::Node, "AZ::Dom::Value: attempted to retrieve a node from a non-node value");
        return *Internal::CheckCopyOnWrite(AZStd::get<NodePtr>(m_value));
    }

    const Object::ContainerType& Value::GetObjectInternal() const
    {
        const Type type = GetType();
        AZ_Assert(
            type == Type::Object || type == Type::Node,
            "AZ::Dom::Value: attempted to retrieve an object from a value that isn't an object or a node");
        if (type == Type::Object)
        {
            return AZStd::get<ObjectPtr>(m_value)->m_values;
        }
        else
        {
            return AZStd::get<NodePtr>(m_value)->GetProperties();
        }
    }

    Object::ContainerType& Value::GetObjectInternal()
    {
        const Type type = GetType();
        AZ_Assert(
            type == Type::Object || type == Type::Node,
            "AZ::Dom::Value: attempted to retrieve an object from a value that isn't an object or a node");
        if (type == Type::Object)
        {
            return Internal::CheckCopyOnWrite(AZStd::get<ObjectPtr>(m_value))->m_values;
        }
        else
        {
            return Internal::CheckCopyOnWrite(AZStd::get<NodePtr>(m_value))->GetProperties();
        }
    }

    const Array::ContainerType& Value::GetArrayInternal() const
    {
        const Type type = GetType();
        AZ_Assert(
            type == Type::Array || type == Type::Node,
            "AZ::Dom::Value: attempted to retrieve an array from a value that isn't an array or a node");
        if (type == Type::Array)
        {
            return AZStd::get<ArrayPtr>(m_value)->m_values;
        }
        else
        {
            return AZStd::get<NodePtr>(m_value)->GetChildren();
        }
    }

    Array::ContainerType& Value::GetArrayInternal()
    {
        const Type type = GetType();
        AZ_Assert(
            type == Type::Array || type == Type::Node,
            "AZ::Dom::Value: attempted to retrieve an array from a value that isn't an array or node");
        if (type == Type::Array)
        {
            return Internal::CheckCopyOnWrite(AZStd::get<ArrayPtr>(m_value))->m_values;
        }
        else
        {
            return Internal::CheckCopyOnWrite(AZStd::get<NodePtr>(m_value))->GetChildren();
        }
    }

    size_t Value::MemberCount() const
    {
        return GetObjectInternal().size();
    }

    size_t Value::MemberCapacity() const
    {
        return GetObjectInternal().size();
    }

    bool Value::ObjectEmpty() const
    {
        return GetObjectInternal().empty();
    }

    Value& Value::operator[](KeyType name)
    {
        Object::ContainerType& object = GetObjectInternal();
        auto existingEntry = AZStd::find_if(
            object.begin(), object.end(),
            [&name](const Object::EntryType& entry)
            {
                return entry.first == name;
            });
        if (existingEntry != object.end())
        {
            return existingEntry->second;
        }
        else
        {
            object.emplace_back(name, Value());
            return object[object.size() - 1].second;
        }
    }

    const Value& Value::operator[](KeyType name) const
    {
        return FindMember(name)->second;
    }

    Value& Value::operator[](AZStd::string_view name)
    {
        return operator[](AZ::Name(name));
    }

    const Value& Value::operator[](AZStd::string_view name) const
    {
        return operator[](AZ::Name(name));
    }

    Object::ConstIterator Value::MemberBegin() const
    {
        return GetObjectInternal().begin();
    }

    Object::ConstIterator Value::MemberEnd() const
    {
        return GetObjectInternal().end();
    }

    Object::Iterator Value::MutableMemberBegin()
    {
        return GetObjectInternal().begin();
    }

    Object::Iterator Value::MutableMemberEnd()
    {
        return GetObjectInternal().end();
    }

    Object::ConstIterator Value::FindMember(KeyType name) const
    {
        const Object::ContainerType& object = GetObjectInternal();
        return AZStd::find_if(
            object.begin(), object.end(),
            [&name](const Object::EntryType& entry)
            {
                return entry.first == name;
            });
    }

    Object::ConstIterator Value::FindMember(AZStd::string_view name) const
    {
        return FindMember(AZ::Name(name));
    }

    Object::Iterator Value::FindMutableMember(KeyType name)
    {
        Object::ContainerType& object = GetObjectInternal();
        return AZStd::find_if(
            object.begin(), object.end(),
            [&name](const Object::EntryType& entry)
            {
                return entry.first == name;
            });
    }

    Object::Iterator Value::FindMutableMember(AZStd::string_view name)
    {
        return FindMutableMember(AZ::Name(name));
    }

    Value& Value::MemberReserve(size_t newCapacity)
    {
        GetObjectInternal().reserve(newCapacity);
        return *this;
    }

    bool Value::HasMember(KeyType name) const
    {
        return FindMember(name) != GetObjectInternal().end();
    }

    bool Value::HasMember(AZStd::string_view name) const
    {
        return HasMember(AZ::Name(name));
    }

    Value& Value::AddMember(KeyType name, Value value)
    {
        Object::ContainerType& object = GetObjectInternal();
        // Reserve in ReserveIncrement chunks instead of the default vector doubling strategy
        // Profiling has found that this is an aggregate performance gain for typical workflows
        object.reserve(AZ_SIZE_ALIGN_UP(object.size() + 1, Object::ReserveIncrement));
        if (auto memberIt = FindMutableMember(name); memberIt != object.end())
        {
            memberIt->second = AZStd::move(value);
        }
        else
        {
            object.emplace_back(AZStd::move(name), AZStd::move(value));
        }
        return *this;
    }

    Value& Value::AddMember(AZStd::string_view name, Value value)
    {
        return AddMember(AZ::Name(name), AZStd::move(value));
    }

    void Value::RemoveAllMembers()
    {
        GetObjectInternal().clear();
    }

    void Value::RemoveMember(KeyType name)
    {
        Object::ContainerType& object = GetObjectInternal();
        object.erase(AZStd::remove_if(
            object.begin(), object.end(),
            [&name](const Object::EntryType& entry)
            {
                return entry.first == name;
            }));
    }

    void Value::RemoveMember(AZStd::string_view name)
    {
        RemoveMember(AZ::Name(name));
    }

    Object::Iterator Value::RemoveMember(Object::Iterator pos)
    {
        Object::ContainerType& object = GetObjectInternal();
        if (!object.empty())
        {
            *pos = AZStd::move(object.back());
            object.pop_back();
        }
        return object.end();
    }

    Object::Iterator Value::EraseMember(Object::Iterator pos)
    {
        return GetObjectInternal().erase(pos);
    }

    Object::Iterator Value::EraseMember(Object::Iterator first, Object::Iterator last)
    {
        return GetObjectInternal().erase(first, last);
    }

    Object::Iterator Value::EraseMember(KeyType name)
    {
        return GetObjectInternal().erase(FindMember(name));
    }

    Object::Iterator Value::EraseMember(AZStd::string_view name)
    {
        return EraseMember(AZ::Name(name));
    }

    Object::ContainerType& Value::GetMutableObject()
    {
        return GetObjectInternal();
    }

    const Object::ContainerType& Value::GetObject() const
    {
        return GetObjectInternal();
    }

    Value& Value::SetArray()
    {
        m_value = AZStd::allocate_shared<Array>(ValueAllocator_for_std_t());
        return *this;
    }

    size_t Value::ArraySize() const
    {
        return GetArrayInternal().size();
    }

    size_t Value::ArrayCapacity() const
    {
        return GetArrayInternal().capacity();
    }

    bool Value::IsArrayEmpty() const
    {
        return GetArrayInternal().empty();
    }

    void Value::ClearArray()
    {
        GetArrayInternal().clear();
    }

    Value& Value::operator[](size_t index)
    {
        return GetArrayInternal()[index];
    }

    const Value& Value::operator[](size_t index) const
    {
        return GetArrayInternal()[index];
    }

    Value& Value::MutableArrayAt(size_t index)
    {
        return operator[](index);
    }

    const Value& Value::ArrayAt(size_t index) const
    {
        return operator[](index);
    }

    Array::ConstIterator Value::ArrayBegin() const
    {
        return GetArrayInternal().begin();
    }

    Array::ConstIterator Value::ArrayEnd() const
    {
        return GetArrayInternal().end();
    }

    Array::Iterator Value::MutableArrayBegin()
    {
        return GetArrayInternal().begin();
    }

    Array::Iterator Value::MutableArrayEnd()
    {
        return GetArrayInternal().end();
    }

    Value& Value::ArrayReserve(size_t newCapacity)
    {
        GetArrayInternal().reserve(newCapacity);
        return *this;
    }

    Value& Value::ArrayPushBack(Value value)
    {
        Array::ContainerType& array = GetArrayInternal();
        // Reserve in ReserveIncremenet chunks instead of the default vector doubling strategy
        // Profiling has found that this is an aggregate performance gain for typical workflows
        array.reserve(AZ_SIZE_ALIGN_UP(array.size() + 1, Array::ReserveIncrement));
        array.push_back(AZStd::move(value));
        return *this;
    }

    Value& Value::ArrayPopBack()
    {
        GetArrayInternal().pop_back();
        return *this;
    }

    Array::Iterator Value::ArrayInsertRange(Array::ConstIterator insertPos, AZStd::span<Value> values)
    {
        return GetArrayInternal().insert(insertPos, values.begin(), values.end());
    }

    Array::Iterator Value::ArrayInsert(Array::ConstIterator insertPos, Array::ConstIterator first, Array::ConstIterator last)
    {
        return GetArrayInternal().insert(insertPos, first, last);
    }

    Array::Iterator Value::ArrayInsert(Array::ConstIterator insertPos, AZStd::initializer_list<Value> initList)
    {
        return GetArrayInternal().insert(insertPos, initList);
    }

    Array::Iterator Value::ArrayInsert(Array::ConstIterator insertPos, Value value)
    {
        return GetArrayInternal().insert(insertPos, value);
    }

    Array::Iterator Value::ArrayErase(Array::Iterator pos)
    {
        return GetArrayInternal().erase(pos);
    }

    Array::Iterator Value::ArrayErase(Array::Iterator first, Array::Iterator last)
    {
        return GetArrayInternal().erase(first, last);
    }

    Array::ContainerType& Value::GetMutableArray()
    {
        return GetArrayInternal();
    }

    const Array::ContainerType& Value::GetArray() const
    {
        return GetArrayInternal();
    }

    void Value::SetNode(AZ::Name name)
    {
        m_value = AZStd::allocate_shared<Node>(ValueAllocator_for_std_t(), AZStd::move(name));
    }

    void Value::SetNode(AZStd::string_view name)
    {
        SetNode(AZ::Name(name));
    }

    AZ::Name Value::GetNodeName() const
    {
        return GetNodeInternal().GetName();
    }

    void Value::SetNodeName(AZ::Name name)
    {
        GetNodeInternal().SetName(AZStd::move(name));
    }

    void Value::SetNodeName(AZStd::string_view name)
    {
        SetNodeName(AZ::Name(name));
    }

    void Value::SetNodeValue(Value value)
    {
        AZ_Assert(GetType() == Type::Node, "AZ::Dom::Value: Attempted to set value for non-node type");
        Array::ContainerType& nodeChildren = GetArrayInternal();

        // Set the first non-node child, if one is found
        for (Value& entry : nodeChildren)
        {
            if (entry.GetType() != Type::Node)
            {
                entry = AZStd::move(value);
                return;
            }
        }

        // Otherwise, append the value entry
        nodeChildren.push_back(AZStd::move(value));
    }

    Value Value::GetNodeValue() const
    {
        AZ_Assert(GetType() == Type::Node, "AZ::Dom::Value: Attempted to get value for non-node type");
        const Array::ContainerType& nodeChildren = GetArrayInternal();

        // Get the first non-node child, if one is found
        for (const Value& entry : nodeChildren)
        {
            if (entry.GetType() != Type::Node)
            {
                return entry;
            }
        }

        return Value();
    }

    Node& Value::GetMutableNode()
    {
        return GetNodeInternal();
    }

    const Node& Value::GetNode() const
    {
        return GetNodeInternal();
    }

    AZ::s64 Value::GetInt64() const
    {
        switch (m_value.index())
        {
        case GetTypeIndex<AZ::s64>():
            return AZStd::get<AZ::s64>(m_value);
        case GetTypeIndex<AZ::u64>():
            return static_cast<AZ::s64>(AZStd::get<AZ::u64>(m_value));
        case GetTypeIndex<double>():
            return static_cast<AZ::s64>(AZStd::get<double>(m_value));
        }
        AZ_Assert(false, "AZ::Dom::Value: Called GetInt on a non-numeric type");
        return {};
    }

    void Value::SetInt64(AZ::s64 value)
    {
        m_value = value;
    }

    AZ::u64 Value::GetUint64() const
    {
        switch (m_value.index())
        {
        case GetTypeIndex<AZ::s64>():
            return static_cast<AZ::u64>(AZStd::get<AZ::s64>(m_value));
        case GetTypeIndex<AZ::u64>():
            return AZStd::get<AZ::u64>(m_value);
        case GetTypeIndex<double>():
            return static_cast<AZ::u64>(AZStd::get<double>(m_value));
        }
        AZ_Assert(false, "AZ::Dom::Value: Called GetInt on a non-numeric type");
        return {};
    }

    void Value::SetUint64(AZ::u64 value)
    {
        m_value = value;
    }

    bool Value::GetBool() const
    {
        if (IsBool())
        {
            return AZStd::get<bool>(m_value);
        }
        AZ_Assert(false, "AZ::Dom::Value: Called GetBool on a non-bool type");
        return {};
    }

    void Value::SetBool(bool value)
    {
        m_value = value;
    }

    double Value::GetDouble() const
    {
        switch (m_value.index())
        {
        case GetTypeIndex<AZ::s64>():
            return static_cast<double>(AZStd::get<AZ::s64>(m_value));
        case GetTypeIndex<AZ::u64>():
            return static_cast<double>(AZStd::get<AZ::u64>(m_value));
        case GetTypeIndex<double>():
            return AZStd::get<double>(m_value);
        }
        AZ_Assert(false, "AZ::Dom::Value: Called GetInt on a non-numeric type");
        return {};
    }

    void Value::SetDouble(double value)
    {
        m_value = value;
    }

    void Value::SetString(SharedStringType sharedString)
    {
        m_value = sharedString;
    }

    AZStd::string_view Value::GetString() const
    {
        switch (m_value.index())
        {
        case GetTypeIndex<AZStd::string_view>():
            return AZStd::get<AZStd::string_view>(m_value);
        case GetTypeIndex<SharedStringType>():
            {
                auto& buffer = *AZStd::get<SharedStringType>(m_value);
                return { buffer.data(), buffer.size() };
            }
        case GetTypeIndex<ShortStringType>():
            {
                const ShortStringType& shortString = AZStd::get<ShortStringType>(m_value);
                return { shortString.data(), shortString.size() };
            }
        }
        AZ_Assert(false, "AZ::Dom::Value: Called GetString on a non-string type");
        return {};
    }

    size_t Value::GetStringLength() const
    {
        return GetString().size();
    }

    void Value::SetString(AZStd::string_view value)
    {
        m_value = value;
    }

    void Value::CopyFromString(AZStd::string_view value)
    {
        if (value.size() <= ShortStringSize)
        {
            ShortStringType buffer;
            buffer.resize_no_construct(value.size());
            memcpy(buffer.data(), value.data(), value.size());
            m_value = buffer;
        }
        else
        {
            SharedStringType sharedString = AZStd::allocate_shared<SharedStringContainer>(ValueAllocator_for_std_t(), value.begin(), value.end());
            m_value = AZStd::move(sharedString);
        }
    }

    const AZStd::any& Value::GetOpaqueValue() const
    {
        return *AZStd::get<OpaqueStorageType>(m_value);
    }

    void Value::SetOpaqueValue(AZStd::any value)
    {
        m_value = AZStd::allocate_shared<AZStd::any>(ValueAllocator_for_std_t(), AZStd::move(value));
    }

    void Value::SetNull()
    {
        m_value = AZStd::monostate();
    }

    Visitor::Result Value::Accept(Visitor& visitor, bool copyStrings) const
    {
        Visitor::Result result = AZ::Success();

        AZStd::visit(
            [&](auto&& arg)
            {
                using Alternative = AZStd::decay_t<decltype(arg)>;

                if constexpr (AZStd::is_same_v<Alternative, AZStd::monostate>)
                {
                    result = visitor.Null();
                }
                else if constexpr (AZStd::is_same_v<Alternative, AZ::s64>)
                {
                    result = visitor.Int64(arg);
                }
                else if constexpr (AZStd::is_same_v<Alternative, AZ::u64>)
                {
                    result = visitor.Uint64(arg);
                }
                else if constexpr (AZStd::is_same_v<Alternative, double>)
                {
                    result = visitor.Double(arg);
                }
                else if constexpr (AZStd::is_same_v<Alternative, bool>)
                {
                    result = visitor.Bool(arg);
                }
                else if constexpr (AZStd::is_same_v<Alternative, AZStd::string_view>)
                {
                    result = visitor.String(arg, copyStrings ? Lifetime::Temporary : Lifetime::Persistent);
                }
                else if constexpr (AZStd::is_same_v<Alternative, SharedStringType>)
                {
                    result = visitor.RefCountedString(arg, copyStrings ? Lifetime::Temporary : Lifetime::Persistent);
                }
                else if constexpr (AZStd::is_same_v<Alternative, ShortStringType>)
                {
                    result = visitor.String(arg, copyStrings ? Lifetime::Temporary : Lifetime::Persistent);
                }
                else if constexpr (AZStd::is_same_v<Alternative, ObjectPtr>)
                {
                    result = visitor.StartObject();
                    if (result.IsSuccess())
                    {
                        const Object::ContainerType& object = GetObjectInternal();
                        for (const Object::EntryType& entry : object)
                        {
                            result = visitor.Key(entry.first);
                            if (!result.IsSuccess())
                            {
                                return;
                            }
                            result = entry.second.Accept(visitor, copyStrings);
                            if (!result.IsSuccess())
                            {
                                return;
                            }
                        }
                        result = visitor.EndObject(object.size());
                    }
                }
                else if constexpr (AZStd::is_same_v<Alternative, ArrayPtr>)
                {
                    result = visitor.StartArray();
                    if (result.IsSuccess())
                    {
                        const Array::ContainerType& arrayContainer = GetArrayInternal();
                        for (const Value& entry : arrayContainer)
                        {
                            result = entry.Accept(visitor, copyStrings);
                            if (!result.IsSuccess())
                            {
                                return;
                            }
                        }
                        result = visitor.EndArray(arrayContainer.size());
                    }
                }
                else if constexpr (AZStd::is_same_v<Alternative, NodePtr>)
                {
                    const Node& node = *AZStd::get<NodePtr>(m_value);
                    result = visitor.StartNode(node.GetName());
                    if (result.IsSuccess())
                    {
                        const Object::ContainerType& object = GetObjectInternal();
                        for (const Object::EntryType& entry : object)
                        {
                            result = visitor.Key(entry.first);
                            if (!result.IsSuccess())
                            {
                                return;
                            }
                            result = entry.second.Accept(visitor, copyStrings);
                            if (!result.IsSuccess())
                            {
                                return;
                            }
                        }

                        const Array::ContainerType& arrayContainer = GetArrayInternal();
                        for (const Value& entry : arrayContainer)
                        {
                            result = entry.Accept(visitor, copyStrings);
                            if (!result.IsSuccess())
                            {
                                return;
                            }
                        }

                        result = visitor.EndNode(object.size(), arrayContainer.size());
                    }
                }
                else if constexpr (AZStd::is_same_v<Alternative, OpaqueStorageType>)
                {
                    result = visitor.OpaqueValue(*arg);
                }
            },
            m_value);
        return result;
    }

    AZStd::unique_ptr<Visitor> Value::GetWriteHandler()
    {
        return AZStd::make_unique<ValueWriter>(*this);
    }

    const Value::ValueType& Value::GetInternalValue() const
    {
        return m_value;
    }

    Value& Value::operator[](const PathEntry& entry)
    {
        if (entry.IsEndOfArray())
        {
            Array::ContainerType& array = GetArrayInternal();
            return array.emplace_back();
        }
        return entry.IsIndex() ? operator[](entry.GetIndex()) : operator[](entry.GetKey());
    }

    const Value& Value::operator[](const PathEntry& entry) const
    {
        return entry.IsIndex() ? operator[](entry.GetIndex()) : operator[](entry.GetKey());
    }

    Value& Value::operator[](const Path& path)
    {
        Value* value = this;
        for (const PathEntry& entry : path)
        {
            value = &value->operator[](entry);
        }
        return *value;
    }

    const Value& Value::operator[](const Path& path) const
    {
        const Value* value = this;
        for (const PathEntry& entry : path)
        {
            value = &value->operator[](entry);
        }
        return *value;
    }

    const Value* Value::FindChild(const PathEntry& entry) const
    {
        if (entry.IsEndOfArray())
        {
            return nullptr;
        }
        else if (entry.IsIndex())
        {
            const Array::ContainerType& array = GetArrayInternal();
            const size_t index = entry.GetIndex();
            if (index < array.size())
            {
                return &array[index];
            }
        }
        else
        {
            const Object::ContainerType& obj = GetObjectInternal();
            auto memberIt = FindMember(entry.GetKey());
            if (memberIt != obj.end())
            {
                return &memberIt->second;
            }
        }
        return nullptr;
    }

    Value* Value::FindMutableChild(const PathEntry& entry)
    {
        if (entry.IsEndOfArray())
        {
            Array::ContainerType& array = GetArrayInternal();
            return &array.emplace_back();
        }
        else if (entry.IsIndex())
        {
            Array::ContainerType& array = GetArrayInternal();
            const size_t index = entry.GetIndex();
            if (index < array.size())
            {
                return &array[index];
            }
        }
        else
        {
            Object::ContainerType& obj = GetObjectInternal();
            auto memberIt = FindMutableMember(entry.GetKey());
            if (memberIt != obj.end())
            {
                return &memberIt->second;
            }
        }
        return nullptr;
    }

    const Value* Value::FindChild(const Path& path) const
    {
        const Value* value = this;
        for (const PathEntry& entry : path)
        {
            value = value->FindChild(entry);
            if (value == nullptr)
            {
                return nullptr;
            }
        }
        return value;
    }

    Value* Value::FindMutableChild(const Path& path)
    {
        Value* value = this;
        for (const PathEntry& entry : path)
        {
            value = value->FindMutableChild(entry);
            if (value == nullptr)
            {
                return nullptr;
            }
        }
        return value;
    }
} // namespace AZ::Dom
