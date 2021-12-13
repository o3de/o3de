/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomValue.h>
#include <AzCore/DOM/DomValueWriter.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ::Dom
{
    template<class T>
    AZStd::shared_ptr<T>& CheckCopyOnWrite(AZStd::shared_ptr<T>& refCountedPointer)
    {
        if (refCountedPointer.use_count() > 1)
        {
            AZStd::shared_ptr<T> newPointer = AZStd::allocate_shared<T>(AZStdAlloc<ValueAllocator>());
            *newPointer = *refCountedPointer;
            refCountedPointer = AZStd::move(newPointer);
        }
        return refCountedPointer;
    }

    Node::Node(AZ::Name name)
        : m_name(name)
    {
    }

    AZ::Name Node::GetName() const
    {
        return m_name;
    }

    void Node::SetName(AZ::Name name)
    {
        m_name = name;
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

    Value::Value(AZStd::shared_ptr<const AZStd::string> string)
        : m_value(string)
    {
    }

    Value::Value()
    {
    }

    Value::Value(const Value& value)
        : m_value(value.m_value)
    {
    }

    Value::Value(Value&& value) noexcept
    {
        operator=(value);
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

    Value::Value(int32_t value)
        : m_value(aznumeric_cast<int64_t>(value))
    {
    }

    Value::Value(uint32_t value)
        : m_value(aznumeric_cast<uint64_t>(value))
    {
    }

    Value::Value(int64_t value)
        : m_value(value)
    {
    }

    Value::Value(uint64_t value)
        : m_value(value)
    {
    }

    Value::Value(float value)
        : m_value(aznumeric_cast<double>(value))
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
        case Type::False:
            m_value = false;
            break;
        case Type::True:
            m_value = true;
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
        case Type::Number:
            m_value = 0.0;
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
        m_value.swap(other.m_value);
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
                return GetInt64() == rhs.GetInt64();
            }
            else if (IsUint())
            {
                return GetUint64() == rhs.GetUint64();
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
        m_value.swap(other.m_value);
    }

    Type Dom::Value::GetType() const
    {
        switch (m_value.index())
        {
        case 0: // AZStd::monostate
            return Type::Null;
        case 1: // int64_t
        case 2: // uint64_t
        case 3: // double
            return Type::Number;
        case 4: // bool
            return AZStd::get<bool>(m_value) ? Type::True : Type::False;
        case 5: // AZStd::string_view
        case 6: // AZStd::shared_ptr<const AZStd::string>
        case 7: // ShortStringType
            return Type::String;
        case 8: // ObjectPtr
            return Type::Object;
        case 9: // ArrayPtr
            return Type::Array;
        case 10: // NodePtr
            return Type::Node;
        case 11: // AZStd::any*
            return Type::Opaque;
        }
        AZ_Assert(false, "AZ::Dom::Value::GetType: m_value has an unexpected type");
        return Type::Null;
    }

    bool Value::IsNull() const
    {
        return GetType() == Type::Null;
    }

    bool Value::IsFalse() const
    {
        return GetType() == Type::False;
    }

    bool Value::IsTrue() const
    {
        return GetType() == Type::True;
    }

    bool Value::IsBool() const
    {
        return AZStd::holds_alternative<bool>(m_value);
    }

    bool Value::IsNode() const
    {
        return GetType() == Type::Node;
    }

    bool Value::IsObject() const
    {
        return GetType() == Type::Object;
    }

    bool Value::IsArray() const
    {
        return GetType() == Type::Array;
    }

    bool Value::IsOpaqueValue() const
    {
        return GetType() == Type::Opaque;
    }

    bool Value::IsNumber() const
    {
        return GetType() == Type::Number;
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
        return GetType() == Type::String;
    }

    Value& Value::SetObject()
    {
        m_value = AZStd::allocate_shared<Object>(AZStdAlloc<ValueAllocator>());
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
        return *CheckCopyOnWrite(AZStd::get<NodePtr>(m_value));
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
            return CheckCopyOnWrite(AZStd::get<ObjectPtr>(m_value))->m_values;
        }
        else
        {
            return CheckCopyOnWrite(AZStd::get<NodePtr>(m_value))->GetProperties();
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
            return CheckCopyOnWrite(AZStd::get<ArrayPtr>(m_value))->m_values;
        }
        else
        {
            return CheckCopyOnWrite(AZStd::get<NodePtr>(m_value))->GetChildren();
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

    Value& Value::AddMember(KeyType name, const Value& value)
    {
        Object::ContainerType& object = GetObjectInternal();
        object.reserve((object.size() / Object::ReserveIncrement + 1) * Object::ReserveIncrement);
        if (auto memberIt = FindMutableMember(name); memberIt != object.end())
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
        if (auto memberIt = FindMutableMember(name); memberIt != object.end())
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
        m_value = AZStd::allocate_shared<Array>(AZStdAlloc<ValueAllocator>());
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

    Value& Value::MutableAt(size_t index)
    {
        return operator[](index);
    }

    const Value& Value::At(size_t index) const
    {
        return operator[](index);
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
        Array::ContainerType& array = GetArrayInternal();
        array.reserve((array.size() / Array::ReserveIncrement + 1) * Array::ReserveIncrement);
        array.push_back(AZStd::move(value));
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
        m_value = AZStd::allocate_shared<Node>(AZStdAlloc<ValueAllocator>(), name);
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
        GetNodeInternal().SetName(name);
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

    int64_t Value::GetInt64() const
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

    void Value::SetInt64(int64_t value)
    {
        m_value = value;
    }

    int32_t Value::GetInt32() const
    {
        return aznumeric_cast<int32_t>(GetInt64());
    }

    void Value::SetInt32(int32_t value)
    {
        m_value = aznumeric_cast<int64_t>(value);
    }

    uint64_t Value::GetUint64() const
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

    void Value::SetUint64(uint64_t value)
    {
        m_value = value;
    }

    uint32_t Value::GetUint32() const
    {
        return aznumeric_cast<uint32_t>(GetUint64());
    }

    void Value::SetUint32(uint32_t value)
    {
        m_value = aznumeric_cast<uint64_t>(value);
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

    float Value::GetFloat() const
    {
        return aznumeric_cast<float>(GetDouble());
    }

    void Value::SetFloat(float value)
    {
        m_value = aznumeric_cast<double>(value);
    }

    void Value::SetString(AZStd::shared_ptr<const AZStd::string> string)
    {
        m_value = string;
    }

    AZStd::string_view Value::GetString() const
    {
        switch (m_value.index())
        {
        case 5: // AZStd::string_view
            return AZStd::get<AZStd::string_view>(m_value);
        case 6: // AZStd::shared_ptr<const AZStd::string>
            return *AZStd::get<AZStd::shared_ptr<const AZStd::string>>(m_value);
        case 7: // ShortStringType
            {
                const ShortStringType& ShortString = AZStd::get<ShortStringType>(m_value);
                return { ShortString.m_data.data(), ShortString.m_size };
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
        if (value.size() <= ShortStringSize)
        {
            ShortStringType buffer;
            buffer.m_size = value.size();
            memcpy(buffer.m_data.data(), value.data(), buffer.m_size);
            m_value = buffer;
        }
        m_value = value;
    }

    void Value::CopyFromString(AZStd::string_view value)
    {
        if (value.size() <= ShortStringSize)
        {
            SetString(value);
        }
        else
        {
            m_value = AZStd::allocate_shared<const AZStd::string>(AZStdAlloc<ValueAllocator>(), value);
        }
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
                else if constexpr (AZStd::is_same_v<Alternative, int64_t>)
                {
                    result = visitor.Int64(arg);
                }
                else if constexpr (AZStd::is_same_v<Alternative, uint64_t>)
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
                else if constexpr (AZStd::is_same_v<Alternative, AZStd::shared_ptr<const AZStd::string>>)
                {
                    result = visitor.RefCountedString(arg, copyStrings ? Lifetime::Temporary : Lifetime::Persistent);
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
                else if constexpr (AZStd::is_same_v<Alternative, AZStd::any>)
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

    bool Value::DeepCompareIsEqual(const Value& other) const
    {
        if (IsString() && other.IsString())
        {
            // If we both hold the same ref counted string we don't need to do a full comparison
            if (AZStd::holds_alternative<AZStd::shared_ptr<const AZStd::string>>(m_value) && m_value == other.m_value)
            {
                return true;
            }
            return GetString() == other.GetString();
        }

        if (m_value.index() != other.m_value.index())
        {
            return false;
        }

        return AZStd::visit(
            [&](auto&& ourValue) -> bool
            {
                using Alternative = AZStd::decay_t<decltype(ourValue)>;
                auto&& theirValue = AZStd::get<AZStd::remove_cvref_t<decltype(ourValue)>>(other.m_value);

                if constexpr (AZStd::is_same_v<Alternative, AZStd::monostate>)
                {
                    return true;
                }
                else if constexpr (AZStd::is_same_v<Alternative, ObjectPtr>)
                {
                    if (ourValue == theirValue)
                    {
                        return true;
                    }

                    if (ourValue->m_values.size() != theirValue->m_values.size())
                    {
                        return false;
                    }

                    for (size_t i = 0; i < ourValue->m_values.size(); ++i)
                    {
                        const Object::EntryType& lhs = ourValue->m_values[i];
                        const Object::EntryType& rhs = theirValue->m_values[i];
                        if (lhs.first != rhs.first || !lhs.second.DeepCompareIsEqual(rhs.second))
                        {
                            return false;
                        }
                    }

                    return true;
                }
                else if constexpr (AZStd::is_same_v<Alternative, ArrayPtr>)
                {
                    if (ourValue == theirValue)
                    {
                        return true;
                    }

                    if (ourValue->m_values.size() != theirValue->m_values.size())
                    {
                        return false;
                    }

                    for (size_t i = 0; i < ourValue->m_values.size(); ++i)
                    {
                        const Value& lhs = ourValue->m_values[i];
                        const Value& rhs = theirValue->m_values[i];
                        if (!lhs.DeepCompareIsEqual(rhs))
                        {
                            return false;
                        }
                    }

                    return true;
                }
                else if constexpr (AZStd::is_same_v<Alternative, NodePtr>)
                {
                    if (ourValue == theirValue)
                    {
                        return true;
                    }

                    const Node& ourNode = *ourValue;
                    const Node& theirNode = *theirValue;

                    const Object::ContainerType& ourProperties = ourNode.GetProperties();
                    const Object::ContainerType& theirProperties = theirNode.GetProperties();

                    if (ourProperties.size() != theirProperties.size())
                    {
                        return false;
                    }

                    for (size_t i = 0; i < ourProperties.size(); ++i)
                    {
                        const Object::EntryType& lhs = ourProperties[i];
                        const Object::EntryType& rhs = theirProperties[i];
                        if (lhs.first != rhs.first || !lhs.second.DeepCompareIsEqual(rhs.second))
                        {
                            return false;
                        }
                    }

                    const Array::ContainerType& ourChildren = ourNode.GetChildren();
                    const Array::ContainerType& theirChildren = theirNode.GetChildren();

                    for (size_t i = 0; i < ourChildren.size(); ++i)
                    {
                        const Value& lhs = ourChildren[i];
                        const Value& rhs = theirChildren[i];
                        if (!lhs.DeepCompareIsEqual(rhs))
                        {
                            return false;
                        }
                    }

                    return true;
                }
                else
                {
                    return ourValue == theirValue;
                }
            },
            m_value);
    }

    Value Value::DeepCopy(bool copyStrings) const
    {
        Value newValue;
        AZStd::unique_ptr<Visitor> writer = newValue.GetWriteHandler();
        Accept(*writer, copyStrings);
        return newValue;
    }
} // namespace AZ::Dom
