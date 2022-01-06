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
}

namespace AZ
{
    class NameDictionary;
    class ScriptDataContext;
    class ReflectContext;

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
    class Name
    {
        friend NameDictionary;
        friend UnitTest::NameTest;
    public:
        using Hash = Internal::NameData::Hash;

        AZ_TYPE_INFO(Name, "{3D2B920C-9EFD-40D5-AAE0-DF131C3D4931}");
        AZ_CLASS_ALLOCATOR(Name, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);

        Name();
        Name(const Name& name);
        Name(Name&& name);
        Name& operator=(const Name&);
        Name& operator=(Name&&);


        //! Creates an instance of a name from a string. 
        //! The name string is used as a key to lookup an entry in the dictionary, and is not 
        //! internally held after the call.
        explicit Name(AZStd::string_view name);

        //! Creates an instance of a name from a hash.
        //! The hash will be used to find an existing name in the dictionary. If there is no
        //! name with this hash, the resulting name will be empty.
        explicit Name(Hash hash);
        
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

    private:
        
        // Assigns a new name.  
        // The name string is used as a key to lookup an entry in the dictionary, and is not 
        // internally held after the call.
        // This is needed for reflection into behavior context.
        void SetName(AZStd::string_view name);
        
        void SetEmptyString();
        
        // This constructor is used by NameDictionary to construct from a dictionary-held NameData instance.
        Name(Internal::NameData* nameData);

        static void ScriptConstructor(Name* thisPtr, ScriptDataContext& dc);

        // Points to the string that represents the value of this name.
        // Most of the time this same information is available in m_data, but keeping it here too...
        // - Removes an indirection when accessing the name value.
        // - Allows functions like data() to return an empty string instead of null for empty Name objects.
        AZStd::string_view m_view;

        Hash m_hash;

        //! Pointer to NameData in the NameDictionary. This holds both the hash and string pair.
        AZStd::intrusive_ptr<Internal::NameData> m_data;
    };

} // namespace AZ

namespace AZStd
{
    template <typename T>
    struct hash;

    // hashing support for STL containers
    template <>
    struct hash<AZ::Name>
    {
        AZ::Name::Hash operator()(const AZ::Name& value) const
        {
            return value.GetHash();
        }
    };
}


