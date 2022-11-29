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
#include <AzCore/Memory/AllocatorWrappers.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ::Dom
{
    class PathEntry;
    class Path;
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
    AZ_ALLOCATOR_DEFAULT_GLOBAL_WRAPPER(ValueAllocator, AZ::SystemAllocator, "{5BC8B389-72C7-459E-B502-12E74D61869F}")

    using StdValueAllocator = ValueAllocator;

    class Value;

    //! Internal storage for a Value array: an ordered list of Values.
    class Array
    {
    public:
        using ContainerType = AZStd::vector<Value, StdValueAllocator>;
        using Iterator = ContainerType::iterator;
        using ConstIterator = ContainerType::const_iterator;
        static constexpr const size_t ReserveIncrement = 4;
        static_assert((ReserveIncrement & (ReserveIncrement - 1)) == 0, "ReserveIncremenet must be a power of 2");

        const ContainerType& GetValues() const;

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
        using ContainerType = AZStd::vector<EntryType, StdValueAllocator>;
        using Iterator = ContainerType::iterator;
        using ConstIterator = ContainerType::const_iterator;
        static constexpr const size_t ReserveIncrement = 8;
        static_assert((ReserveIncrement & (ReserveIncrement - 1)) == 0, "ReserveIncremenet must be a power of 2");

        const ContainerType& GetValues() const;

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
        Node(const Node&) = default;
        Node(Node&&) = default;
        explicit Node(AZ::Name name);

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
        AZ_TYPE_INFO(Value, "{3E20677F-3B8E-4F89-B665-ED41D74F4799}");
        AZ_CLASS_ALLOCATOR(Value, ValueAllocator, 0);

        // Determine the short string buffer size based on the size of our largest internal type (string_view)
        // minus the size of the short string size field.
        static constexpr const size_t ShortStringSize = sizeof(AZStd::string_view) - 2;
        using ShortStringType = AZStd::fixed_string<ShortStringSize>;
        using SharedStringContainer = AZStd::vector<char>;
        using SharedStringType = AZStd::shared_ptr<const SharedStringContainer>;
        using OpaqueStorageType = AZStd::shared_ptr<AZStd::any>;

        //! The internal storage type for Value.
        //! These types do not correspond one-to-one with the Value's external Type as there may be multiple storage classes
        //! for the same type in some instances, such as string storage.
        using ValueType = AZStd::variant<
            // Null
            AZStd::monostate,
            // Int64
            int64_t,
            // Uint64
            uint64_t,
            // Double
            double,
            // Bool
            bool,
            // String
            AZStd::string_view,
            SharedStringType,
            ShortStringType,
            // Object
            ObjectPtr,
            // Array
            ArrayPtr,
            // Node
            NodePtr,
            // Opaque
            OpaqueStorageType>;

        // Constructors...
        Value() = default;
        Value(const Value&);
        Value(Value&&) noexcept;
        Value(AZStd::string_view stringView, bool copy);
        explicit Value(SharedStringType sharedString);

        explicit Value(int8_t value);
        explicit Value(uint8_t value);
        explicit Value(int16_t value);
        explicit Value(uint16_t value);
        explicit Value(int32_t value);
        explicit Value(uint32_t value);
        explicit Value(int64_t value);
        explicit Value(uint64_t value);
        explicit Value(float value);
        explicit Value(double value);
        explicit Value(bool value);

        explicit Value(Type type);

        // Disable accidental calls to Value(bool) with pointer types
        template<class T>
        explicit Value(T*) = delete;

        static Value FromOpaqueValue(const AZStd::any& value);
        static Value CreateNode(AZ::Name nodeName);
        static Value CreateNode(AZStd::string_view nodeName);

        // Equality / comparison / swap...
        Value& operator=(const Value&);
        Value& operator=(Value&&) noexcept;

        //! Assignment operator to allow forwarding types constructible via Value(T) to be assigned
        template<class T>
        auto operator=(T&& arg)
            -> AZStd::enable_if_t<!AZStd::is_same_v<AZStd::remove_cvref_t<T>, Value> && AZStd::is_constructible_v<Value, T>, Value&>
        {
            return operator=(Value(AZStd::forward<T>(arg)));
        }

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
        Object::Iterator MutableMemberBegin();
        Object::Iterator MutableMemberEnd();

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
        Object::Iterator EraseMember(Object::Iterator pos);
        Object::Iterator EraseMember(Object::Iterator first, Object::Iterator last);
        Object::Iterator EraseMember(KeyType name);
        Object::Iterator EraseMember(AZStd::string_view name);

        Object::ContainerType& GetMutableObject();
        const Object::ContainerType& GetObject() const;

        // Array API (also used by Node)...
        Value& SetArray();

        size_t ArraySize() const;
        size_t ArrayCapacity() const;
        bool IsArrayEmpty() const;
        void ClearArray();

        Value& operator[](size_t index);
        const Value& operator[](size_t index) const;

        Value& MutableArrayAt(size_t index);
        const Value& ArrayAt(size_t index) const;

        Array::ConstIterator ArrayBegin() const;
        Array::ConstIterator ArrayEnd() const;
        Array::Iterator MutableArrayBegin();
        Array::Iterator MutableArrayEnd();

        Value& ArrayReserve(size_t newCapacity);
        Value& ArrayPushBack(Value value);
        Value& ArrayPopBack();

        Array::Iterator ArrayErase(Array::Iterator pos);
        Array::Iterator ArrayErase(Array::Iterator first, Array::Iterator last);

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

        // uint API...
        uint64_t GetUint64() const;
        void SetUint64(uint64_t);

        // bool API...
        bool GetBool() const;
        void SetBool(bool);

        // double API...
        double GetDouble() const;
        void SetDouble(double);

        // String API...
        AZStd::string_view GetString() const;
        size_t GetStringLength() const;
        void SetString(AZStd::string_view);
        void SetString(SharedStringType sharedString);
        void CopyFromString(AZStd::string_view);

        // Opaque type API...
        const AZStd::any& GetOpaqueValue() const;
        //! This sets this Value to represent a value of an type that the DOM has
        //! no formal knowledge of. Where possible, it should be preferred to
        //! serialize an opaque type into a DOM value instead, as serializers
        //! and other systems will have no means of dealing with fully arbitrary
        //! values.
        void SetOpaqueValue(AZStd::any);

        // Null API...
        void SetNull();

        // Visitor API...
        Visitor::Result Accept(Visitor& visitor, bool copyStrings) const;
        AZStd::unique_ptr<Visitor> GetWriteHandler();

        // Path API...
        Value& operator[](const PathEntry& entry);
        const Value& operator[](const PathEntry& entry) const;
        Value& operator[](const Path& path);
        const Value& operator[](const Path& path) const;

        const Value* FindChild(const PathEntry& entry) const;
        Value* FindMutableChild(const PathEntry& entry);
        const Value* FindChild(const Path& path) const;
        Value* FindMutableChild(const Path& path);

        //! Gets the internal value of this Value. Note that this value's types may not correspond one-to-one with the Type enumeration,
        //! as internally the same type might have different storage mechanisms. Where possible, prefer using the typed API.
        const ValueType& GetInternalValue() const;

    private:
        const Node& GetNodeInternal() const;
        Node& GetNodeInternal();
        const Object::ContainerType& GetObjectInternal() const;
        Object::ContainerType& GetObjectInternal();
        const Array::ContainerType& GetArrayInternal() const;
        Array::ContainerType& GetArrayInternal();

        explicit Value(AZStd::any opaqueValue);

        static_assert(
            sizeof(ValueType) == sizeof(ShortStringType) + sizeof(size_t), "ValueType should have no members larger than ShortStringType");

        ValueType m_value;
    };
} // namespace AZ::Dom
