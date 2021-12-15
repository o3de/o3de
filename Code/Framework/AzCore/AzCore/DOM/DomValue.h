/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomBackend.h>
#include <AzCore/DOM/DomVisitor.h>
#include <AzCore/Memory/HphaSchema.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ::Dom
{
    using KeyType = AZ::Name;

    //! The type of underlying value stored in a value. \see Value
    enum class Type
    {
        Null,
        Bool,
        Object,
        Array,
        String,
        Int64,
        Uint64,
        Double,
        Node,
        Opaque,
    };

    //! The allocator used by Value.
    //! Value heap allocates shared_ptrs for its container storage (Array / Object / Node) alongside
    class ValueAllocator final : public SimpleSchemaAllocator<AZ::HphaSchema, AZ::HphaSchema::Descriptor, false, false>
    {
    public:
        AZ_TYPE_INFO(ValueAllocator, "{5BC8B389-72C7-459E-B502-12E74D61869F}");

        using Base = SimpleSchemaAllocator<AZ::HphaSchema, AZ::HphaSchema::Descriptor, false, false>;

        ValueAllocator()
            : Base("DomValueAllocator", "Allocator for AZ::Dom::Value")
        {
            DisableOverriding();
        }
    };

    class Value;

    //! Internal storage for a Value array: an ordered list of Values.
    class Array
    {
    public:
        using ContainerType = AZStd::vector<Value, AZStdAlloc<ValueAllocator>>;
        using Iterator = ContainerType::iterator;
        using ConstIterator = ContainerType::const_iterator;
        static constexpr const size_t ReserveIncrement = 4;

    private:
        ContainerType m_values;

        friend class Value;
    };

    using ArrayPtr = AZStd::shared_ptr<Array>;
    using ConstArrayPtr = AZStd::shared_ptr<const Array>;

    //! Internal storage for a Value object: an ordered list of Name / Value pairs.
    class Object
    {
    public:
        using EntryType = AZStd::pair<KeyType, Value>;
        using ContainerType = AZStd::vector<EntryType, AZStdAlloc<ValueAllocator>>;
        using Iterator = ContainerType::iterator;
        using ConstIterator = ContainerType::const_iterator;
        static constexpr const size_t ReserveIncrement = 8;

    private:
        ContainerType m_values;

        friend class Value;
    };

    using ObjectPtr = AZStd::shared_ptr<Object>;
    using ConstObjectPtr = AZStd::shared_ptr<const Object>;

    //! Storage for a Value node: a named Value with both properties and children.
    //! Properties are stored as an ordered list of Name / Value pairs.
    //! Children are stored as an oredered list of Values.
    class Node
    {
    public:
        Node() = default;
        Node(AZ::Name name);
        Node(const Node&) = default;
        Node(Node&&) = default;

        Node& operator=(const Node&) = default;
        Node& operator=(Node&&) = default;

        AZ::Name GetName() const;
        void SetName(AZ::Name name);

        Object::ContainerType& GetProperties();
        const Object::ContainerType& GetProperties() const;

        Array::ContainerType& GetChildren();
        const Array::ContainerType& GetChildren() const;

    private:
        AZ::Name m_name;
        Object::ContainerType m_properties;
        Array::ContainerType m_children;

        friend class Value;
    };

    using NodePtr = AZStd::shared_ptr<Node>;
    using ConstNodePtr = AZStd::shared_ptr<Node>;

    //! Value is a typed union of Dom types that can represent the types provdied by AZ::Dom::Visitor.
    //! Value can be one of the following types:
    //! - Null: a type with no value, this is the default type for Value
    //! - Bool: a true or false boolean value
    //! - Object: a container with an ordered list of Name/Value pairs, analagous to a JSON object
    //! - Array: a container with an ordered list of Values, analagous to a JSON array
    //! - String: a UTF-8 string
    //! - Int64: a signed, 64-bit integer
    //! - Uint64: an unsigned, 64-bit integer
    //! - Double: a double precision floating point value
    //! - Node: a container with a Name, an ordered list of Name/Values pairs (attributes), and an ordered list of Values (children),
    //! analagous to an XML node
    //! - Opaque: an arbitrary value stored in an AZStd::any. This is a non-serializable representation of an entry used only for in-memory
    //! options. This is intended to be used as an intermediate value over the course of DOM transformation and as a proxy to pass through
    //! types of which the DOM has no knowledge to other systems.
    //! \note Value is a copy-on-write data structure and may be cheaply returned by value. Heap allocated data larger than the size of the
    //! value itself (objects, arrays, and nodes) are copied by new Values only when their contents change, so care should be taken in
    //! performance critical code to avoid mutation operations such as operator[] to avoid copies. It is recommended that an immutable Value
    //! be explicitly be stored as a `const Value` to avoid accidental detach and copy operations.
    class Value final
    {
    public:
        // Constructors...
        Value();
        Value(const Value&);
        Value(Value&&) noexcept;
        Value(AZStd::string_view string, bool copy);
        Value(AZStd::shared_ptr<const AZStd::string> string);

        Value(int32_t value);
        Value(uint32_t value);
        Value(int64_t value);
        Value(uint64_t value);
        Value(float value);
        Value(double value);
        Value(bool value);

        explicit Value(Type type);

        static Value FromOpaqueValue(AZStd::any& value);

        // Equality / comparison / swap...
        Value& operator=(const Value&);
        Value& operator=(Value&&) noexcept;

        bool operator==(const Value& rhs) const;
        bool operator!=(const Value& rhs) const;

        void Swap(Value& other) noexcept;

        // Type info...
        Type GetType() const;
        bool IsNull() const;
        bool IsFalse() const;
        bool IsTrue() const;
        bool IsBool() const;
        bool IsNode() const;
        bool IsObject() const;
        bool IsArray() const;
        bool IsOpaqueValue() const;
        bool IsNumber() const;
        bool IsInt() const;
        bool IsUint() const;
        bool IsDouble() const;
        bool IsString() const;

        // Object API (also used by Node)...
        Value& SetObject();
        size_t MemberCount() const;
        size_t MemberCapacity() const;
        bool ObjectEmpty() const;

        Value& operator[](KeyType name);
        const Value& operator[](KeyType name) const;
        Value& operator[](AZStd::string_view name);
        const Value& operator[](AZStd::string_view name) const;

        Object::ConstIterator MemberBegin() const;
        Object::ConstIterator MemberEnd() const;
        Object::Iterator MemberBegin();
        Object::Iterator MemberEnd();

        Object::Iterator FindMutableMember(KeyType name);
        Object::Iterator FindMutableMember(AZStd::string_view name);
        Object::ConstIterator FindMember(KeyType name) const;
        Object::ConstIterator FindMember(AZStd::string_view name) const;

        Value& MemberReserve(size_t newCapacity);
        bool HasMember(KeyType name) const;
        bool HasMember(AZStd::string_view name) const;

        Value& AddMember(KeyType name, const Value& value);
        Value& AddMember(AZStd::string_view name, const Value& value);
        Value& AddMember(KeyType name, Value&& value);
        Value& AddMember(AZStd::string_view name, Value&& value);

        void RemoveAllMembers();
        void RemoveMember(KeyType name);
        void RemoveMember(AZStd::string_view name);
        Object::Iterator RemoveMember(Object::Iterator pos);
        Object::Iterator EraseMember(Object::ConstIterator pos);
        Object::Iterator EraseMember(Object::ConstIterator first, Object::ConstIterator last);
        Object::Iterator EraseMember(KeyType name);
        Object::Iterator EraseMember(AZStd::string_view name);

        Object::ContainerType& GetMutableObject();
        const Object::ContainerType& GetObject() const;

        // Array API (also used by Node)...
        Value& SetArray();

        size_t Size() const;
        size_t Capacity() const;
        bool Empty() const;
        void Clear();

        Value& operator[](size_t index);
        const Value& operator[](size_t index) const;

        Value& MutableAt(size_t index);
        const Value& At(size_t index) const;

        Array::ConstIterator Begin() const;
        Array::ConstIterator End() const;
        Array::Iterator Begin();
        Array::Iterator End();

        Value& Reserve(size_t newCapacity);
        Value& PushBack(Value value);
        Value& PopBack();

        Array::Iterator Erase(Array::ConstIterator pos);
        Array::Iterator Erase(Array::ConstIterator first, Array::ConstIterator last);

        Array::ContainerType& GetMutableArray();
        const Array::ContainerType& GetArray() const;

        // Node API (supports both object + array API, plus a dedicated NodeName)...
        void SetNode(AZ::Name name);
        void SetNode(AZStd::string_view name);

        AZ::Name GetNodeName() const;
        void SetNodeName(AZ::Name name);
        void SetNodeName(AZStd::string_view name);
        //! Convenience method, sets the first non-node element of a Node.
        void SetNodeValue(Value value);
        //! Convenience method, gets the first non-node element of a Node.
        Value GetNodeValue() const;

        Node& GetMutableNode();
        const Node& GetNode() const;

        // int API...
        int64_t GetInt64() const;
        void SetInt64(int64_t);
        int32_t GetInt32() const;
        void SetInt32(int32_t);

        // uint API...
        uint64_t GetUint64() const;
        void SetUint64(uint64_t);
        uint32_t GetUint32() const;
        void SetUint32(uint32_t);

        // bool API...
        bool GetBool() const;
        void SetBool(bool);

        // double API...
        double GetDouble() const;
        void SetDouble(double);
        float GetFloat() const;
        void SetFloat(float);

        // String API...
        AZStd::string_view GetString() const;
        size_t GetStringLength() const;
        void SetString(AZStd::string_view);
        void SetString(AZStd::shared_ptr<const AZStd::string>);
        void CopyFromString(AZStd::string_view);

        // Opaque type API...
        AZStd::any& GetOpaqueValue() const;
        //! This sets this Value to represent a value of an type that the DOM has
        //! no formal knowledge of. Where possible, it should be preferred to
        //! serialize an opaque type into a DOM value instead, as serializers
        //! and other systems will have no means of dealing with fully arbitrary
        //! values.
        void SetOpaqueValue(AZStd::any&);

        // Null API...
        void SetNull();

        // Visitor API...
        Visitor::Result Accept(Visitor& visitor, bool copyStrings) const;
        AZStd::unique_ptr<Visitor> GetWriteHandler();

        bool DeepCompareIsEqual(const Value& other) const;
        Value DeepCopy(bool copyStrings = true) const;

    private:
        const Node& GetNodeInternal() const;
        Node& GetNodeInternal();
        const Object::ContainerType& GetObjectInternal() const;
        Object::ContainerType& GetObjectInternal();
        const Array::ContainerType& GetArrayInternal() const;
        Array::ContainerType& GetArrayInternal();

        explicit Value(AZStd::any* opaqueValue);

        // Determine the short string buffer size based on the size of our largest internal type (string_view)
        // minus the size of the short string size field.
        static constexpr const size_t ShortStringSize = sizeof(AZStd::string_view) - sizeof(size_t);
        struct ShortStringType
        {
            AZStd::array<char, ShortStringSize> m_data;
            size_t m_size;

            bool operator==(const ShortStringType& other) const
            {
                return m_size == other.m_size ? memcmp(m_data.data(), other.m_data.data(), m_size) == 0 : false;
            }
        };

        //! The internal storage type for Value.
        //! These types do not correspond one-to-one with the Value's external Type as there may be multiple storage classes
        //! for the same type in some instances, such as string storage.
        using ValueType = AZStd::variant<
            // NullType
            AZStd::monostate,
            // NumberType
            int64_t,
            uint64_t,
            double,
            // FalseType & TrueType
            bool,
            // StringType
            AZStd::string_view,
            AZStd::shared_ptr<const AZStd::string>,
            ShortStringType,
            // ObjectType
            ObjectPtr,
            // ArrayType
            ArrayPtr,
            // NodeType
            NodePtr,
            // OpaqueType
            AZStd::any*>;

        static_assert(
            sizeof(ValueType) == sizeof(AZStd::variant<ShortStringType>), "ValueType should have no members larger than ShortStringType");

        ValueType m_value;
    };
} // namespace AZ::Dom
