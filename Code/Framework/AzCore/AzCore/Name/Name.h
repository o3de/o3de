/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Name/Internal/NameData.h>

namespace UnitTest
{
    class NameTest;
    class NameDictionaryTester;
} // namespace UnitTest

namespace AZ
{
    class NameDictionary;
    class ScriptDataContext;
    class ReflectContext;

    //! A reference to data stored in a Name dictionary.
    //! Smaller than Name but requires a memory indirection to look up
    //! the name text or hash.
    //! \see Name
    using NameRef = AZStd::intrusive_ptr<Internal::NameData>;

    //! The Name class provides very fast string equality comparison, so that names can be used as IDs without sacrificing performance.
    //! It is a smart pointer to a NameData held in a NameDictionary, where names are tracked, de-duplicated, and ref-counted.
    //!
    //! Creating a Name object with a value that doesn't exist in the dictionary is very slow.
    //! Creating a Name object with a value that already exists in the dictionary is similar to creating AZStd::string.
    //! Copy-constructing a Name object is very fast.
    //! Equality-comparison of two Name objects is very fast.
    //!
    //! The dictionary must be initialized before Name objects are created.
    //! A Name instance must not be statically declared.
    class Name final
    {
        friend NameDictionary;
        friend UnitTest::NameTest;
        friend UnitTest::NameDictionaryTester;

    public:
        using Hash = Internal::NameData::Hash;

        AZ_TYPE_INFO(Name, "{3D2B920C-9EFD-40D5-AAE0-DF131C3D4931}");
        AZ_CLASS_ALLOCATOR(Name, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);

        Name();
        Name(const Name& name);
        Name(Name&& name);
        template<size_t N>
        explicit Name(const char (&literalString)[N])
        {
            SetName(literalString, true);
        }

        static Name FromStringLiteral(AZStd::string_view name);

        Name& operator=(const Name&);
        Name& operator=(Name&&);

        ~Name();

        //! Creates a NameRef from this Name, exposing its internal NameData pointer with no cached hash/string.
        inline operator const NameRef&() const&
        {
            return m_data;
        }

        operator NameRef&() &
        {
            return m_data;
        }

        //! Creates an instance of a name from a string.
        //! The name string is used as a key to lookup an entry in the dictionary, and is not
        //! internally held after the call.
        explicit Name(AZStd::string_view name);

        //! Creates an instance of a name from a hash.
        //! The hash will be used to find an existing name in the dictionary. If there is no
        //! name with this hash, the resulting name will be empty.
        explicit Name(Hash hash);

        //! Creates a name from a NameRef, an already existent name within the name dictionary.
        Name(NameRef name);

        //! Assigns a new name.
        //! The name string is used as a key to lookup an entry in the dictionary, and is not
        //! internally held after the call.
        Name& operator=(AZStd::string_view name);

        //! Returns the name's string value.
        //! This is always null-terminated.
        //! This will always point to a string in memory (i.e. it will return "" instead of null).
        AZStd::string_view GetStringView() const;
        const char* GetCStr() const;

        bool IsEmpty() const;

        bool operator==(const Name& other) const
        {
            return GetHash() == other.GetHash();
        }

        bool operator!=(const Name& other) const
        {
            return GetHash() != other.GetHash();
        }

        bool operator==(const NameRef& other) const
        {
            return m_data == other;
        }

        bool operator!=(const NameRef& other) const
        {
            return m_data != other;
        }

        // We delete these operators because using Name in ordered containers is not supported.
        // The point of Name is for fast equality comparison and lookup. Unordered containers should be used instead.
        friend bool operator<(const Name& lhs, const Name& rhs) = delete;
        friend bool operator<=(const Name& lhs, const Name& rhs) = delete;
        friend bool operator>(const Name& lhs, const Name& rhs) = delete;
        friend bool operator>=(const Name& lhs, const Name& rhs) = delete;

        //! Returns the string's hash that is used as the key in the NameDictionary.
        Hash GetHash() const
        {
            return m_hash;
        }

        AZ_FORCE_INLINE static Name* GetDeferredHead()
        {
            return s_staticNameBegin;
        }

    private:
        // Assigns a new name.
        // The name string is used as a key to lookup an entry in the dictionary, and is not
        // internally held after the call.
        // This is needed for reflection into behavior context.
        void SetName(AZStd::string_view name, bool isLiteral = false);

        void SetNameLiteral(AZStd::string_view name);

        // This constructor is used by NameDictionary to construct from a dictionary-held NameData instance.
        Name(Internal::NameData* nameData);

        static void ScriptConstructor(Name* thisPtr, ScriptDataContext& dc);

        //! If set, this name has been linked to the global name dictionary's static list
        bool m_linkedToDictionary = false;
        //! If set, this name can be used for deferred loading
        bool m_supportsDeferredLoad = false;

        //! The internal hash used by this name.
        Hash m_hash = 0;

        // Points to the string that represents the value of this name.
        // Most of the time this same information is available in m_data, but keeping it here too...
        // - Removes an indirection when accessing the name value.
        // - Allows functions like data() to return an empty string instead of null for empty Name objects.
        AZStd::string_view m_view;

        //! Pointer to NameData in the NameDictionary. This holds both the hash and string pair.
        NameRef m_data;

        void LinkStaticName(Name** name);
        void UnlinkStaticName();

        //! Describes the begin of the static list of Names that were initialized before the NameDictionary was available.
        //! On module initialization, these names are linked into the NameDictionary's static pool and created.
        static Name* s_staticNameBegin;
        //! Describes the next name entry in the static list of Names, if needed.
        Name* m_nextName = nullptr;
        //! Describes the previous name entry in the static list of Names, if needed.
        Name* m_previousName = nullptr;
    };

} // namespace AZ

//! Defines a cached name literal that describes an AZ::Name. Subsequent calls to this macro will retrieve the cached name from the
//! dictionary.
#define AZ_NAME_LITERAL(str)                                                                                                               \
    (                                                                                                                                      \
        []() -> AZ::Name                                                                                                                   \
        {                                                                                                                                  \
            static AZ::Name nameLiteral(AZ::Name::FromStringLiteral(str));                                                                 \
            return nameLiteral;                                                                                                            \
        })()

namespace AZStd
{
    template<typename T>
    struct hash;

    // hashing support for STL containers
    template<>
    struct hash<AZ::Name>
    {
        AZ::Name::Hash operator()(const AZ::Name& value) const
        {
            return value.GetHash();
        }
    };
} // namespace AZStd
