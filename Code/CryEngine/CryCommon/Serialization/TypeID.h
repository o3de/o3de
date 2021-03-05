/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_TYPEID_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_TYPEID_H
#pragma once


#include "Serialization/Assert.h"
#include "Serialization/Strings.h"
#include <string.h>

namespace Serialization {
    class IArchive;
    struct TypeInfo;
    class TypeID
    {
    public:
        TypeID()
            : typeInfo_(0)
            , module_(0) {}

        TypeID(const TypeID& original)
            : typeInfo_(original.typeInfo_)
            , module_(original.module_)
        {
        }

        operator bool() const{
            return *this != TypeID();
        }

        template<class T>
        static TypeID get();
        std::size_t sizeOf() const;
        const char* name() const;

        bool operator==(const TypeID& rhs) const;
        bool operator!=(const TypeID& rhs) const;
        bool operator<(const TypeID& rhs) const;
    private:
        TypeInfo* typeInfo_;
        void* module_;
        friend struct TypeInfo;
        friend class TypeDescription;
    };

    struct TypeInfo
    {
        TypeID id;
        size_t size;
        char name[128];

        // We are trying to minimize type names here. Stripping namespaces,
        // whitespaces and S/C/E/I prefixes. Why namespaces? Type names are usually
        // used in two contexts: for unique name within factory context, where
        // collision is unlikely, or for filtering in PropertyTree where concise
        // name is much more useful.
        static void cleanTypeName(char*& d, const char* dend, const char*& s, const char* send)
        {
            if (strncmp(s, "class ", 6) == 0)
            {
                s += 6;
            }
            else if (strncmp(s, "struct ", 7) == 0)
            {
                s += 7;
            }

            while (*s == ' ' && s != send)
            {
                ++s;
            }

            // strip C/S/I/E prefixes
            if ((*s == 'C' || *s == 'S' || *s == 'I' || *s == 'E') && s[1] >= 'A' && s[1] <= 'Z')
            {
                ++s;
            }

            if (s >= send)
            {
                return;
            }

            char* startd = d;
            while (d != dend && s != send)
            {
                while (*s == ' ' && s != send)
                {
                    ++s;
                }
                if (s == send)
                {
                    break;
                }
                if (*s == ':' && s[1] == ':')
                {
                    // strip namespaces
                    s += 2;
                    d = startd;

                    if ((*s == 'C' || *s == 'S' || *s == 'I' || *s == 'E') && s[1] >= 'A' && s[1] <= 'Z')
                    {
                        ++s;
                    }
                }
                if (s >= send)
                {
                    break;
                }
                if (*s == '<')
                {
                    * d = '<';
                    ++d;
                    ++s;
                    cleanTypeName(d, dend, s, send);
                }
                else if (*s == '>')
                {
                    * d = '\0';
                    return;
                }
                * d = *s;
                ++s;
                ++d;
            }
        }

        template<size_t nameLen>
        static void extractTypeName(char (&name)[nameLen], const char* funcName)
        {
#ifdef __clang__
            // "static yasli::TypeID yasli::TypeID::get() [T = ActualTypeName]"
            const char* s = strstr(funcName, "[T = ");
            if (s)
            {
                s += 5;
            }
            const char* send = strrchr(funcName, ']');
#elif __GNUC__ >= 4 || (__GNUC__ == 4 &&  __GNUC_MINOR__ >= 4)
            // "static yasli::TypeID yasli::TypeID::get() [with T = ActualTypeName]"
            const char* s = strstr(funcName, "[with T = ");
            if (s)
            {
                s += 9;
            }
            const char* send = strrchr(funcName, ']');
#else
            // "static yasli::TypeID yasli::TypeID::get<ActualTypeName>()"
            const char* s = strchr(funcName, '<');
            const char* send = strrchr(funcName, '>');
            YASLI_ASSERT(s != 0  && send != 0);
            if (s != send)
            {
                ++s;
            }
#endif
            YASLI_ASSERT(s != 0  && send != 0);

            char* d = name;
            const char* dend = name + sizeof(name) - 1;
            cleanTypeName(d, dend, s, send);
            * d = '\0';

            // This assertion is not critical, but may result in collision as
            // stripped name wil be used, e.g. for lookup in factory.
            YASLI_ASSERT(s == send && "Type name does not fit into the buffer");
        }

        TypeInfo(size_t _size, const char* templatedFunctionName)
            : size(_size)
        {
            extractTypeName(name, templatedFunctionName);
            id.typeInfo_ = this;
            static int moduleSpecificSymbol;
            id.module_ = &moduleSpecificSymbol;
        }

        bool operator==(const TypeInfo& rhs) const
        {
            return size == rhs.size && strcmp(name, rhs.name) == 0;
        }

        bool operator<(const TypeInfo& rhs) const
        {
            if (size == rhs.size)
            {
                return strcmp(name, rhs.name) < 0;
            }
            else
            {
                return size < rhs.size;
            }
        }
    };

    template<class T>
    TypeID TypeID::get()
    {
#ifdef _MSC_VER
        static TypeInfo typeInfo(sizeof(T), __FUNCSIG__);
#else
        static TypeInfo typeInfo(sizeof(T), __PRETTY_FUNCTION__);
#endif
        return typeInfo.id;
    }

    inline const char* TypeID::name() const
    {
        if (typeInfo_)
        {
            return typeInfo_->name;
        }
        else
        {
            return "";
        }
    }

    inline size_t TypeID::sizeOf() const
    {
        if (typeInfo_)
        {
            return typeInfo_->size;
        }
        else
        {
            return 0;
        }
    }

    inline bool TypeID::operator==(const TypeID& rhs) const
    {
        if (typeInfo_ == rhs.typeInfo_)
        {
            return true;
        }
        else if (!typeInfo_ || !rhs.typeInfo_)
        {
            return false;
        }
        else if (module_ == rhs.module_)
        {
            return false;
        }
        else
        {
            return *typeInfo_ == *rhs.typeInfo_;
        }
    }

    inline bool TypeID::operator!=(const TypeID& rhs) const
    {
        return !operator==(rhs);
    }

    inline bool TypeID::operator<(const TypeID& rhs) const
    {
        if (!typeInfo_)
        {
            return rhs.typeInfo_ != 0;
        }
        else if (!rhs.typeInfo_)
        {
            return false;
        }
        else
        {
            return *typeInfo_ < *rhs.typeInfo_;
        }
    }

    template<class T>
    T* createDerivedClass(TypeID typeID);
}

//bool Serialize(Serialization::IArchive& ar, Serialization::TypeID& typeID, const char* name, const char* label);

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_TYPEID_H
