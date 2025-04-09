/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Name/Internal/NameData.h>
#include <AzCore/std/parallel/thread.h>

namespace UnitTest
{
    class NameTest;
    class NameDictionaryTester;
} // namespace UnitTest

namespace AZ
{
    class Module;
    class Name;
    class NameDictionary;
    class ScriptDataContext;
    class ReflectContext;

    //! A reference to data stored in a Name dictionary.
    //! Smaller than Name but requires a memory indirection to look up
    //! the name text or hash.
    //! \see Name
    class NameRef final
    {
        friend Name;

    public:
        NameRef() = default;
        NameRef(const NameRef&) = default;
        NameRef(NameRef&&) = default;

        NameRef& operator=(NameRef&&) = default;
        NameRef& operator=(const NameRef&) = default;
        NameRef(Name name);
        NameRef& operator=(Name name);

        bool operator==(const NameRef& other) const;
        bool operator!=(const NameRef& other) const;
        bool operator==(const Name& other) const;
        bool operator!=(const Name& other) const;

        AZStd::string_view GetStringView() const;
        const char* GetCStr() const;
        Internal::NameData::Hash GetHash() const;

    private:
        NameRef(const AZStd::intrusive_ptr<Internal::NameData>& nameRef);
        NameRef(AZStd::intrusive_ptr<Internal::NameData>&& nameRef);

        AZStd::intrusive_ptr<Internal::NameData> m_data = nullptr;
    };

    //! The Name class provides very fast string equality comparison, so that names can be used as IDs without sacrificing performance.
    //! It is a smart pointer to a NameData held in a NameDictionary, where names are tracked, de-duplicated, and ref-counted.
    //!
    //! Creating a Name object with a value that doesn't exist in the dictionary is very slow.
    //! Creating a Name object with a value that already exists in the dictionary is similar to creating AZStd::string.
    //! Copy-constructing a Name object is very fast.
    //! Equality-comparison of two Name objects is very fast.
    //!
    //! Names require the dictionary to be initialized before they are created, unless they are created from a
    //! string literal via Name::FromStringLiteral, in which they'll store their string for deferred initialization.
    //! A Name instance may only be statically declared using Name::FromStringLiteral (or the AZ_NAME_LITERAL helper macro).
    class Name final
    {
        friend NameRef;
        friend NameDictionary;
        friend UnitTest::NameTest;
        friend UnitTest::NameDictionaryTester;

    public:
        using Hash = Internal::NameData::Hash;

        AZ_TYPE_INFO(Name, "{3D2B920C-9EFD-40D5-AAE0-DF131C3D4931}");
        AZ_CLASS_ALLOCATOR(Name, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        Name();
        Name(const Name& name);
        Name(Name&& name);

        //! Creates a Name from a string literal.
        //! FromStringLiteral is suitable for creating Names that live at a global or static
        //! scope.
        //!
        //! Names created from string literals may exist without a NameDictionary, and may
        //! continue to persist after the NameDictionary is destroyed (even if it is destroyed
        //! multiple times, as in some text fixtures).
        //!
        //! \warning FromStringLiteral is not thread-safe and should only be called from the
        //! main thread.
        static Name FromStringLiteral(AZStd::string_view name,  NameDictionary* nameDictionary);

        Name& operator=(const Name&);
        Name& operator=(Name&&);

        ~Name();

        //! Creates an instance of a name from a string.
        //! The name string is used as a key to lookup an entry in the dictionary, and is not
        //! internally held after the call.
        explicit Name(AZStd::string_view name);
        Name(AZStd::string_view name, NameDictionary& nameDictionary);

        //! Creates an instance of a name from a hash.
        //! The hash will be used to find an existing name in the dictionary. If there is no
        //! name with this hash, the resulting name will be empty.
        explicit Name(Hash hash);

        //! Lookup an instance of a name from a hash.
        //! This overload uses the supplied nameDictionary
        Name(Hash hash, NameDictionary& nameDictionary);

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
            return m_data == other.m_data;
        }

        bool operator!=(const NameRef& other) const
        {
            return m_data != other.m_data;
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

        //! For internal use:
        //! Gets a reference to the current head of the deferred Name linked list.
        //! The list is used to initialize Names created before the NameDictionary when
        //! their modules are loaded.
        static Name*& GetDeferredHead()
        {
            return s_staticNameBegin;
        }

    private:
        // Assigns a new name.
        // The name string is used as a key to lookup an entry in the dictionary, and is not
        // internally held after the call.
        // This is needed for reflection into behavior context.
        void SetName(AZStd::string_view name);
        void SetName(AZStd::string_view name, NameDictionary& nameDictionary);

        // Assigns a new name.
        // The name string is stored persistently and used as a key to look up an entry in the dictionary.
        // If this is called before the dictionary is available, the key will be used when the name dictionary
        // becomes available.
        void SetNameLiteral(AZStd::string_view name, NameDictionary* nameDictionary);

        // This constructor is used by NameDictionary to construct from a dictionary-held NameData instance.
        Name(Internal::NameData* nameData);

        static void ScriptConstructor(Name* thisPtr, ScriptDataContext& dc);

        void LinkStaticName(Name** name);
        void UnlinkStaticName();

        //! If set, this name can be stored at a static scope and defer loading its NameData
        //! until the NameDictionary is actually created. If set, this Name must hold a persistent pointer
        //! to a string literal in m_view, and the NameDictionary will create m_data when this name is registered
        //! and destroy m_data when it is destroyed. m_view will remain for the lifetime of this name.
        bool m_supportsDeferredLoad = false;
        //! If set, this name has been linked to the name list of the NameDictionary.
        //! This means this Name can have its data stripped and restored if the NameDictionary is ever
        //! recreated; currently, this should only occur in unit tests.
        bool m_linkedToDictionary = false;

        //! The internal hash used by this name.
        Hash m_hash = 0;

        // Points to the string that represents the value of this name.
        // Most of the time this same information is available in m_data, but keeping it here too...
        // - Removes an indirection when accessing the name value.
        // - Allows functions like data() to return an empty string instead of null for empty Name objects.
        AZStd::string_view m_view{ "" };

        //! Pointer to NameData in the NameDictionary. This holds both the hash and string pair.
        AZStd::intrusive_ptr<Internal::NameData> m_data;

        //! Describes the begin of the static list of Names that were initialized before the NameDictionary was available.
        //! On module initialization, these names are linked into the NameDictionary's static pool and created.
        static Name* s_staticNameBegin;
        //! Describes the next name entry in the static list of Names.
        Name* m_nextName = nullptr;
        //! Describes the previous name entry in the static list of Names.
        Name* m_previousName = nullptr;
    };
} // namespace AZ

//! Defines a cached name literal that describes an AZ::Name. Subsequent calls to this macro will retrieve the cached name from the
//! global dictionary.
#define AZ_NAME_LITERAL(str)                                                                                                               \
    (                                                                                                                                      \
        []() -> const AZ::Name&                                                                                                            \
        {                                                                                                                                  \
            static const AZ::Name nameLiteral(AZ::Name::FromStringLiteral(str, AZ::Interface<AZ::NameDictionary>::Get()));                 \
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

    template<>
    struct hash<AZ::NameRef>
    {
        AZ::Name::Hash operator()(const AZ::NameRef& value) const
        {
            return value.GetHash();
        }
    };
} // namespace AZStd
