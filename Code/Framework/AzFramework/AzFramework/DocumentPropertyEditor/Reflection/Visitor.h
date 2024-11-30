/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string_view.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/DocumentPropertyEditor/Reflection/Attribute.h>

namespace AZ::Reflection
{
    class IRead;
    class IReadWrite;

    // Note on the I*Access structs: Functions may return default values such as a null type id, empty strings or nullptr.
    // This will be dependent on the source providing the data. If for instance the information comes from a file a
    // type id might not exist or direct access to an element can be provided.
    // The I*Access classes will be implemented by the data provider, users of this API are not expected to provide their
    // own implementation. Users are only expected to implement the Visit* functions they're interested in from the IRead
    // or IReadWrite interface.

    struct IStringAccess
    {
        virtual const AZ::TypeId& GetStringType() const = 0;

        virtual bool Set(AZStd::string_view string) = 0;
    };

    struct IArrayAccess
    {
        virtual const AZ::TypeId& GetArrayType() const = 0;
        virtual const AZ::TypeId& GetElementType() const = 0;
        virtual const AZ::TypeId& GetCombinedType() const = 0;
        virtual AZStd::string_view GetElementTypeName() const = 0;

        virtual size_t GetElementCount() const = 0;

        virtual bool AppendElement() = 0;
        virtual bool InsertElement(size_t index) = 0;
        virtual bool RemoveElement(size_t index) = 0;

        virtual void* GetElement(size_t index) = 0;
        virtual const void* GetElement(size_t index) const = 0;
        virtual void VisitElement(size_t index, IRead& visitor) const = 0;
        virtual void VisitElement(size_t index, IReadWrite& visitor) = 0;

        virtual void VisitElements(IRead& visitor) const = 0;
        virtual void VisitElements(IReadWrite& visitor) = 0;
    };

    struct IMapAccess
    {
        using Handle = void*;
        static constexpr Handle InvalidHandle = nullptr;

        virtual const AZ::TypeId& GetMapType() const = 0;
        virtual const AZ::TypeId& GetKeyType() const = 0;
        virtual const AZ::TypeId& GetValueType() const = 0;
        virtual const AZ::TypeId& GetCombinedType() const = 0;
        virtual AZStd::string_view GetKeyTypeName() const = 0;
        virtual AZStd::string_view GetValueTypeName() const = 0;

        virtual size_t GetElementCount() const = 0;

        // AddElement works by creating a temporary key and value. The visitor is then used to visit both to allow data to be set
        // that's different from the defaults. After that the element is added to the map.
        virtual bool AddElement(IReadWrite& visitor) = 0;

        // Find element works by creating a temporary key and calling the visitor to update the values of the key. The resulting
        // composited key is then used to locate the key/value pair which is returned an anonymous handle.
        virtual Handle FindElement(IReadWrite& visitor) const = 0;
        // This function works by iterating over the value in the provided element using the provided visitor.
        virtual bool UpdateElement(Handle element, IReadWrite& visitor) = 0;
        virtual void* GetKey(Handle handle) = 0;
        virtual const void* GetKey(Handle handle) const = 0;
        virtual void* GetValue(Handle handle) = 0;
        virtual const void* GetValue(Handle handle) const = 0;

        // The first call to the visitor will always be the key. If the type has a begin/end pair, the end will be called next.
        // Next the value will be visited, which again is follwed by an end call if the type has a begin/end pair. This repeats
        // until all elements have been visited.
        virtual void VisitElements(IRead& visitor) const = 0;
        virtual void VisitElements(IReadWrite& visitor) = 0;
    };

    struct IDictionaryAccess
    {
        virtual const AZ::TypeId& GetDictionaryType() const = 0;
        // This will always return a type that implements a string in some form.
        virtual const AZ::TypeId& GetKeyType() const = 0;
        virtual const AZ::TypeId& GetValueType() const = 0;
        virtual const AZ::TypeId& GetCombinedType() const = 0;
        virtual AZStd::string_view GetValueTypeName() const = 0;

        virtual size_t GetElementCount() const = 0;

        virtual bool AddElement(AZStd::string_view key) = 0;
        virtual bool RemoveElement(AZStd::string_view key) = 0;

        virtual void* GetElement(AZStd::string_view key) = 0;
        virtual const void* GetElement(AZStd::string_view key) const = 0;
        virtual void VisitElement(AZStd::string_view key, IRead& visitor) const = 0;
        virtual void VisitElement(AZStd::string_view key, IReadWrite& visitor) = 0;

        // The first call to the visitor will always be a string followed by the value, similar to the map.
        virtual void VisitElements(IRead& visitor) const = 0;
        virtual void VisitElements(IReadWrite& visitor) = 0;
    };

    struct IEnumAccess
    {
        virtual const AZ::TypeId& GetType() const = 0;
        virtual const AZ::TypeId& GetUnderlyingType() const = 0;

        virtual bool SetValue(AZ::s64 value) = 0;
        // Set the value by providing the name of the enum. Flags can be combined into a single string using
        // a '|' as a separator, for instance "flag1|flag2".
        virtual bool SetValue(AZStd::string_view value) = 0;
    };

    struct IPointerAccess
    {
        virtual const AZ::TypeId& GetPointerType() const = 0;
        virtual const AZ::TypeId& GetBaseValueType() const = 0;
        virtual const AZ::TypeId& GetActualValueType() const = 0;
        virtual const AZ::TypeId& GetCombinedType() const = 0;
        virtual AZStd::string_view GetBaseTypeName() const = 0;
        virtual AZStd::string_view GetActualTypeName() const = 0;

        virtual bool IsNull() const = 0;

        virtual bool SetValue(const AZ::TypeId& typeId) = 0;
        virtual bool SetValue(AZStd::string_view typeName) = 0;
        virtual void Reset() = 0;

        virtual void* Get() = 0;
        virtual const void* Get() const = 0;

        virtual void VisitValue(IRead& visitor) const = 0;
        virtual void VisitValue(IReadWrite& visitor) const = 0;
    };

    struct IBufferAccess
    {
        // These two functions can return a nullptr if the file isn't embedded or not loaded.
        virtual const AZStd::byte* begin() const = 0;
        virtual const AZStd::byte* end() const = 0;
        // This function may return an empty path if the data is embedded.
        virtual AZ::IO::PathView GetSourceFile() const = 0;

        // The functions below may fail depending on whether the implementation stores the path and/or the data.
        virtual bool SetData(AZStd::vector<AZStd::byte> data) = 0;
        virtual bool SetSourceFile(AZ::IO::PathView sourceFilePath) = 0;
    };

    struct IAssetAccess
    {
        virtual const AZ::TypeId& GetDataType() const = 0;
        virtual AZStd::string_view GetDataTypeName() const = 0;

        virtual bool IsReady() const = 0;

        virtual bool Set(const AZ::Data::AssetId& assetId) = 0;
        virtual void Reset() = 0;
    };

    struct IObjectAccess
    {
        virtual AZ::TypeId GetType() const = 0;
        virtual AZStd::string_view GetTypeName() const = 0;

        virtual void* Get() = 0;
        virtual const void* Get() const = 0;
    };

    // The IRead and/or IReadWrite interfaces are implemented by the user.

    class IRead
    {
    public:
        virtual ~IRead() = default;

        virtual void Visit(bool value, const IAttributes& attributes);

        virtual void Visit(char value, const IAttributes& attributes);

        virtual void Visit(AZ::s8 value, const IAttributes& attributes);
        virtual void Visit(AZ::s16 value, const IAttributes& attributes);
        virtual void Visit(AZ::s32 value, const IAttributes& attributes);
        virtual void Visit(AZ::s64 value, const IAttributes& attributes);

        virtual void Visit(AZ::u8 value, const IAttributes& attributes);
        virtual void Visit(AZ::u16 value, const IAttributes& attributes);
        virtual void Visit(AZ::u32 value, const IAttributes& attributes);
        virtual void Visit(AZ::u64 value, const IAttributes& attributes);

        virtual void Visit(float value, const IAttributes& attributes);
        virtual void Visit(double value, const IAttributes& attributes);

        // Starting from this point there can be multiple interpretations of the data, for instance there can be different types of strings
        // that are supported.

        virtual void VisitObjectBegin(const IObjectAccess& access, const IAttributes& attributes);
        virtual void VisitObjectEnd(const IObjectAccess& access, const IAttributes& attributes);

        virtual void Visit(const AZStd::string_view value, const IStringAccess& access, const IAttributes& attributes);
        virtual void Visit(const IArrayAccess& access, const IAttributes& attributes);
        virtual void Visit(const IMapAccess& access, const IAttributes& attributes);
        virtual void Visit(const IDictionaryAccess& access, const IAttributes& attributes);
        virtual void Visit(AZ::s64 value, const IEnumAccess& access, const IAttributes& attributes) = 0;
        virtual void Visit(const IPointerAccess& access, const IAttributes& attributes);
        virtual void Visit(const IBufferAccess& access, const IAttributes& attributes);
        virtual void Visit(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const IAssetAccess& access, const IAttributes& attributes);
    };

    class IReadWrite
    {
    public:
        virtual ~IReadWrite() = default;

        virtual void Visit(bool& value, const IAttributes& attributes);

        virtual void Visit(char& value, const IAttributes& attributes);

        virtual void Visit(AZ::s8& value, const IAttributes& attributes);
        virtual void Visit(AZ::s16& value, const IAttributes& attributes);
        virtual void Visit(AZ::s32& value, const IAttributes& attributes);
        virtual void Visit(AZ::s64& value, const IAttributes& attributes);

        virtual void Visit(AZ::u8& value, const IAttributes& attributes);
        virtual void Visit(AZ::u16& value, const IAttributes& attributes);
        virtual void Visit(AZ::u32& value, const IAttributes& attributes);
        virtual void Visit(AZ::u64& value, const IAttributes& attributes);

        virtual void Visit(float& value, const IAttributes& attributes);
        virtual void Visit(double& value, const IAttributes& attributes);

        // Starting from this point there can be multiple interpretations of the data, for instance there can be different types of strings
        // that are supported.

        virtual void VisitObjectBegin(IObjectAccess& access, const IAttributes& attributes);
        virtual void VisitObjectEnd(IObjectAccess& access, const IAttributes& attributes);

        virtual void Visit(const AZStd::string_view value, IStringAccess& access, const IAttributes& attributes);
        virtual void Visit(IArrayAccess& access, const IAttributes& attributes);
        virtual void Visit(IMapAccess& access, const IAttributes& attributes);
        virtual void Visit(IDictionaryAccess& access, const IAttributes& attributes);
        virtual void Visit(AZ::s64 value, const IEnumAccess& access, const IAttributes& attributes);
        virtual void Visit(IPointerAccess& access, const IAttributes& attributes);
        virtual void Visit(IBufferAccess& access, const IAttributes& attributes);
        virtual void Visit(const AZ::Data::Asset<AZ::Data::AssetData>& asset, IAssetAccess& access, const IAttributes& attributes);
    };

    class IReadWriteToRead final : public IReadWrite
    {
    public:
        IReadWriteToRead(IRead* reader) : m_reader(reader) {}

        void Visit(bool& value, const IAttributes& attributes) override { m_reader->Visit(value, attributes); }

        void Visit(char& value, const IAttributes& attributes) override { m_reader->Visit(value, attributes); }

        void Visit(AZ::s8& value, const IAttributes& attributes) override { m_reader->Visit(value, attributes); }
        void Visit(AZ::s16& value, const IAttributes& attributes) override { m_reader->Visit(value, attributes); }
        void Visit(AZ::s32& value, const IAttributes& attributes) override { m_reader->Visit(value, attributes); }
        void Visit(AZ::s64& value, const IAttributes& attributes) override { m_reader->Visit(value, attributes); }

        void Visit(AZ::u8& value, const IAttributes& attributes) override { m_reader->Visit(value, attributes); }
        void Visit(AZ::u16& value, const IAttributes& attributes) override { m_reader->Visit(value, attributes); }
        void Visit(AZ::u32& value, const IAttributes& attributes) override { m_reader->Visit(value, attributes); }
        void Visit(AZ::u64& value, const IAttributes& attributes) override { m_reader->Visit(value, attributes); }

        void Visit(float& value, const IAttributes& attributes) override { m_reader->Visit(value, attributes); }
        void Visit(double& value, const IAttributes& attributes) override { m_reader->Visit(value, attributes); }

        void VisitObjectBegin(IObjectAccess& access, const IAttributes& attributes) override { m_reader->VisitObjectBegin(access, attributes); }
        void VisitObjectEnd(IObjectAccess& access, const IAttributes& attributes) override { m_reader->VisitObjectEnd(access, attributes); }

        void Visit(const AZStd::string_view value, IStringAccess& access, const IAttributes& attributes) override { m_reader->Visit(value, access, attributes); }
        void Visit(IArrayAccess& access, const IAttributes& attributes) override { m_reader->Visit(access, attributes); }
        void Visit(IMapAccess& access, const IAttributes& attributes) override { m_reader->Visit(access, attributes); }
        void Visit(IDictionaryAccess& access, const IAttributes& attributes) override { m_reader->Visit(access, attributes); }
        void Visit(AZ::s64 value, const IEnumAccess& access, const IAttributes& attributes) override { m_reader->Visit(value, access, attributes); }
        void Visit(IPointerAccess& access, const IAttributes& attributes) override { m_reader->Visit(access, attributes); }
        void Visit(IBufferAccess& access, const IAttributes& attributes) override { m_reader->Visit(access, attributes); }
        void Visit(const AZ::Data::Asset<AZ::Data::AssetData>& asset, IAssetAccess& access, const IAttributes& attributes) override { m_reader->Visit(asset, access, attributes); }

    private:
        IRead* m_reader;
    };
} // namespace AZ::Reflection
