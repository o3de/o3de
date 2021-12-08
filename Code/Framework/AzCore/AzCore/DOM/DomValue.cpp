/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomValue.h>

#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ::Dom
{
    template<class T>
    AZStd::shared_ptr<T>& CheckCopyOnWrite(AZStd::shared_ptr<T>& refCountedPointer)
    {
        if (refCountedPointer.use_count() > 1)
        {
            AZStd::shared_ptr<T> newPointer = AZStd::make_shared<T>();
            *newPointer = *refCountedPointer;
            refCountedPointer = AZStd::move(newPointer);
        }
        return refCountedPointer;
    }

    AZ::Name Node::GetName() const
    {
        return m_name;
    }

    void Node::SetName(AZ::Name name)
    {
        m_name = name;
    }

    ObjectPtr Node::GetMutableProperties()
    {
        CheckCopyOnWrite(m_object);
        return m_object;
    }

    ConstObjectPtr Node::GetProperties() const
    {
        return m_object;
    }

    ArrayPtr Node::GetMutableChildren()
    {
        CheckCopyOnWrite(m_array);
        return m_array;
    }

    ConstArrayPtr Node::GetChildren() const
    {
        return m_array;
    }

    bool Node::operator==(const Node& rhs) const
    {
        return m_name == rhs.m_name && m_array == rhs.m_array && m_object == rhs.m_object;
    }

    Value::Value()
    {
    }

    Value::Value(const Value& value)
        : m_value(value.m_value)
    {
    }

    Value::Value(Value&& value) noexcept
        : m_value(value.m_value)
    {
    }

    Value::Value(AZStd::string_view string, bool copy)
    {
        if (copy)
        {
            CopyFromString(string);
        }
        else
        {
            SetString(string);
        }
    }

    Value::Value(AZStd::any* value)
        : m_value(value)
    {
    }

    Value Value::FromOpaqueValue(AZStd::any& value)
    {
        return Value(&value);
    }

    Value::Value(int64_t value)
        : m_value(value)
    {
    }

    Value::Value(uint64_t value)
        : m_value(value)
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

    Value& Value::operator=(const Value& other)
    {
        m_value = other.m_value;
        return *this;
    }

    Value& Value::operator=(Value&& other) noexcept
    {
        m_value = other.m_value;
        return *this;
    }

    bool Value::operator==(const Value& rhs) const
    {
        if (IsString() && rhs.IsString())
        {
            return GetString() == rhs.GetString();
        }
        else if (IsNumber() && rhs.IsNumber())
        {
            if (IsInt())
            {
                return GetInt() == rhs.GetInt();
            }
            else if (IsUint())
            {
                return GetUint() == rhs.GetUint();
            }
            else
            {
                return GetDouble() == rhs.GetDouble();
            }
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
        AZStd::swap(m_value, other.m_value);
    }

    Type Dom::Value::GetType() const
    {
        switch (m_value.index())
        {
        case 0: // AZStd::monostate
            return Type::NullType;
        case 1: // int64_t
        case 2: // uint64_t
        case 3: // double
            return Type::NumberType;
        case 4: // bool
            return AZStd::get<bool>(m_value) ? Type::TrueType : Type::FalseType;
        case 5: // AZStd::string_view
        case 6: // AZStd::shared_ptr<AZStd::string>
            return Type::StringType;
        case 7: // ObjectPtr
            return Type::ObjectType;
        case 8: // ArrayPtr
            return Type::ArrayType;
        case 9: // Node
            return Type::NodeType;
        case 10: // AZStd::any*
            return Type::OpaqueType;
        }
        AZ_Assert(false, "AZ::Dom::Value::GetType: m_value has an unexpected type");
        return Type::NullType;
    }

    bool Value::IsNull() const
    {
        return GetType() == Type::NullType;
    }

    bool Value::IsFalse() const
    {
        return GetType() == Type::FalseType;
    }

    bool Value::IsTrue() const
    {
        return GetType() == Type::TrueType;
    }

    bool Value::IsBool() const
    {
        return AZStd::holds_alternative<bool>(m_value);
    }

    bool Value::IsNode() const
    {
        return GetType() == Type::NodeType;
    }

    bool Value::IsObject() const
    {
        return GetType() == Type::ObjectType;
    }

    bool Value::IsArray() const
    {
        return GetType() == Type::ArrayType;
    }

    bool Value::IsOpaqueValue() const
    {
        return GetType() == Type::OpaqueType;
    }

    bool Value::IsNumber() const
    {
        return GetType() == Type::NumberType;
    }

    bool Value::IsInt() const
    {
        return AZStd::holds_alternative<int64_t>(m_value);
    }

    bool Value::IsUint() const
    {
        return AZStd::holds_alternative<uint64_t>(m_value);
    }

    bool Value::IsDouble() const
    {
        return AZStd::holds_alternative<double>(m_value);
    }

    bool Value::IsString() const
    {
        return GetType() == Type::StringType;
    }

    Value& Value::SetObject()
    {
        m_value = AZStd::make_shared<Object>();
        return *this;
    }

    const Object::ContainerType& Value::GetObjectInternal() const
    {
        const Type type = GetType();
        AZ_Assert(
            type == Type::ObjectType || type == Type::NodeType, "AZ::Dom::Value: attempted to retrieve an object from a non-object value");
        if (type == Type::ObjectType)
        {
            return AZStd::get<ObjectPtr>(m_value)->m_values;
        }
        else
        {
            return AZStd::get<Node>(m_value).GetProperties()->m_values;
        }
    }

    Object::ContainerType& Value::GetObjectInternal()
    {
        const Type type = GetType();
        AZ_Assert(
            type == Type::ObjectType || type == Type::NodeType, "AZ::Dom::Value: attempted to retrieve an object from a non-object value");
        if (type == Type::ObjectType)
        {
            return CheckCopyOnWrite(AZStd::get<ObjectPtr>(m_value))->m_values;
        }
        else
        {
            return AZStd::get<Node>(m_value).GetMutableProperties()->m_values;
        }
    }

    const Array::ContainerType& Value::GetArrayInternal() const
    {
        const Type type = GetType();
        AZ_Assert(
            type == Type::ArrayType || type == Type::NodeType, "AZ::Dom::Value: attempted to retrieve an array from a non-array value");
        if (type == Type::ObjectType)
        {
            return AZStd::get<ArrayPtr>(m_value)->m_values;
        }
        else
        {
            return AZStd::get<Node>(m_value).GetChildren()->m_values;
        }
    }

    Array::ContainerType& Value::GetArrayInternal()
    {
        const Type type = GetType();
        AZ_Assert(
            type == Type::ArrayType || type == Type::NodeType, "AZ::Dom::Value: attempted to retrieve an array from a non-array value");
        if (type == Type::ObjectType)
        {
            return CheckCopyOnWrite(AZStd::get<ArrayPtr>(m_value))->m_values;
        }
        else
        {
            return AZStd::get<Node>(m_value).GetMutableChildren()->m_values;
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
        return FindMember(name)->second;
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

    Object::Iterator Value::MemberBegin()
    {
        return GetObjectInternal().begin();
    }

    Object::Iterator Value::MemberEnd()
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

    Object::Iterator Value::FindMember(KeyType name)
    {
        Object::ContainerType& object = GetObjectInternal();
        return AZStd::find_if(
            object.begin(), object.end(),
            [&name](const Object::EntryType& entry)
            {
                return entry.first == name;
            });
    }

    Object::Iterator Value::FindMember(AZStd::string_view name)
    {
        return FindMember(AZ::Name(name));
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

    Value& Value::AddMember(KeyType name, const Value& value)
    {
        Object::ContainerType& object = GetObjectInternal();
        if (auto memberIt = FindMember(name); memberIt != object.end())
        {
            memberIt->second = value;
        }
        else
        {
            object.emplace_back(name, value);
        }
        return *this;
    }

    Value& Value::AddMember(AZStd::string_view name, const Value& value)
    {
        return AddMember(AZ::Name(name), value);
    }

    Value& Value::AddMember(AZ::Name name, Value&& value)
    {
        Object::ContainerType& object = GetObjectInternal();
        if (auto memberIt = FindMember(name); memberIt != object.end())
        {
            memberIt->second = value;
        }
        else
        {
            object.emplace_back(name, value);
        }
        return *this;
    }

    Value& Value::AddMember(AZStd::string_view name, Value&& value)
    {
        return AddMember(AZ::Name(name), value);
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
        Object::Iterator nextIndex = object.end();
        auto lastEntry = object.end() - 1;
        if (pos != lastEntry)
        {
            AZStd::swap(*pos, *lastEntry);
            nextIndex = pos;
        }
        object.resize(object.size() - 1);
        return nextIndex;
    }

    Object::Iterator Value::EraseMember(Object::ConstIterator pos)
    {
        return GetObjectInternal().erase(pos);
    }

    Object::Iterator Value::EraseMember(Object::ConstIterator first, Object::ConstIterator last)
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

    Object::ContainerType& Value::GetObject()
    {
        return GetObjectInternal();
    }

    const Object::ContainerType& Value::GetObject() const
    {
        return GetObjectInternal();
    }

    Value& Value::SetArray()
    {
        m_value = AZStd::make_shared<Array>();
        return *this;
    }

    size_t Value::Size() const
    {
        return GetArrayInternal().size();
    }

    size_t Value::Capacity() const
    {
        return GetArrayInternal().capacity();
    }

    bool Value::Empty() const
    {
        return GetArrayInternal().empty();
    }

    void Value::Clear()
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

    Array::ConstIterator Value::Begin() const
    {
        return GetArrayInternal().begin();
    }

    Array::ConstIterator Value::End() const
    {
        return GetArrayInternal().end();
    }

    Array::Iterator Value::Begin()
    {
        return GetArrayInternal().begin();
    }

    Array::Iterator Value::End()
    {
        return GetArrayInternal().end();
    }

    Value& Value::Reserve(size_t newCapacity)
    {
        GetArrayInternal().reserve(newCapacity);
        return *this;
    }

    Value& Value::PushBack(Value value)
    {
        GetArrayInternal().push_back(AZStd::move(value));
        return *this;
    }

    Value& Value::PopBack()
    {
        GetArrayInternal().pop_back();
        return *this;
    }

    Array::Iterator Value::Erase(Array::ConstIterator pos)
    {
        return GetArrayInternal().erase(pos);
    }

    Array::Iterator Value::Erase(Array::ConstIterator first, Array::ConstIterator last)
    {
        return GetArrayInternal().erase(first, last);
    }

    Array::ContainerType& Value::GetArray()
    {
        return GetArrayInternal();
    }

    const Array::ContainerType& Value::GetArray() const
    {
        return GetArrayInternal();
    }

    int64_t Value::GetInt() const
    {
        switch (m_value.index())
        {
        case 1: // int64_t
            return AZStd::get<int64_t>(m_value);
        case 2: // uint64_t
            return aznumeric_cast<int64_t>(AZStd::get<uint64_t>(m_value));
        case 3: // double
            return aznumeric_cast<int64_t>(AZStd::get<double>(m_value));
        }
        AZ_Assert(false, "AZ::Dom::Value: Called GetInt on a non-numeric type");
        return {};
    }

    void Value::SetInt(int64_t value)
    {
        m_value = value;
    }

    uint64_t Value::GetUint() const
    {
        switch (m_value.index())
        {
        case 1: // int64_t
            return aznumeric_cast<uint64_t>(AZStd::get<int64_t>(m_value));
        case 2: // uint64_t
            return AZStd::get<uint64_t>(m_value);
        case 3: // double
            return aznumeric_cast<uint64_t>(AZStd::get<double>(m_value));
        }
        AZ_Assert(false, "AZ::Dom::Value: Called GetInt on a non-numeric type");
        return {};
    }

    void Value::SetUint(uint64_t value)
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
        case 1: // int64_t
            return aznumeric_cast<double>(AZStd::get<int64_t>(m_value));
        case 2: // uint64_t
            return aznumeric_cast<double>(AZStd::get<uint64_t>(m_value));
        case 3: // double
            return AZStd::get<double>(m_value);
        }
        AZ_Assert(false, "AZ::Dom::Value: Called GetInt on a non-numeric type");
        return {};
    }

    void Value::SetDouble(double value)
    {
        m_value = value;
    }

    AZStd::string_view Value::GetString() const
    {
        switch (m_value.index())
        {
        case 5: // AZStd::string_view
            return AZStd::get<AZStd::string_view>(m_value);
        case 6: // AZStd::shared_ptr<AZStd::string>
            return *AZStd::get<AZStd::shared_ptr<AZStd::string>>(m_value);
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
        m_value = AZStd::make_shared<AZStd::string>(value);
    }

    AZStd::any& Value::GetOpaqueValue() const
    {
        return *AZStd::get<AZStd::any*>(m_value);
    }

    void Value::SetOpaqueValue(AZStd::any& value)
    {
        m_value = &value;
    }

    void Value::SetNull()
    {
        m_value = AZStd::monostate();
    }
} // namespace AZ::Dom
