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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_ENUM_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_ENUM_H
#pragma once


#include <vector>
#include <map>

#include "StringList.h"
#include "Serialization/TypeID.h"

namespace Serialization {
    class IArchive;

    struct LessStrCmp
    {
        bool operator()(const char* l, const char* r) const
        {
            return strcmp(l, r) < 0;
        }
    };

    class CEnumDescription
    {
    public:
        struct NameValue
        {
            NameValue*  m_next;
            const char* m_name;
            const int   m_value;
            const char* m_label;

            NameValue(CEnumDescription& desc, const char* name, int value, const char* label="")
                : m_next(desc.m_regListHead)
                , m_name(name)
                , m_value(value)
                , m_label(label)
            {
                desc.m_regListHead = this;
            }
        };

        NameValue* m_regListHead = nullptr;

        CEnumDescription(const Serialization::TypeID& type)
            : type_(type) {}
        inline int value(const char* name) const;
        inline int valueByIndex(int index) const;
        inline int valueByLabel(const char* label) const;
        inline const char* name(int value) const;
        inline const char* nameByIndex(int index) const;
        inline const char* labelByIndex(int index) const;
        inline const char* label(int value) const;
        inline const char* indexByName(const char* name) const;
        inline int indexByValue(int value) const;

        inline bool Serialize(IArchive& ar, int& value, const char* name, const char* label) const;
        inline bool serializeBitVector(IArchive& ar, int& value, const char* name, const char* label) const;

        void add(int value, const char* name, const char* label = "");
        int count() const{ return int(values_.size()); }
        const StringListStatic& names() const{ return names_; }
        const StringListStatic& labels() const{ return labels_; }
        inline StringListStatic nameCombination(int bitVector) const;
        inline StringListStatic labelCombination(int bitVector) const;
        bool registered() const { return !names_.empty(); }
        TypeID type() const{ return type_; }
    private:
        void lazyRegister() const;

        StringListStatic names_;
        StringListStatic labels_;

        typedef AZStd::unordered_map<AZStd::string_view, int, AZStd::hash<AZStd::string_view>, AZStd::equal_to<AZStd::string_view>, AZ::StdLegacyAllocator> NameToValue;
        NameToValue nameToValue_;
        typedef AZStd::unordered_map<AZStd::string_view, int, AZStd::hash<AZStd::string_view>, AZStd::equal_to<AZStd::string_view>, AZ::StdLegacyAllocator> LabelToValue;
        LabelToValue labelToValue_;
        typedef AZStd::unordered_map<int, int, AZStd::hash<int>, AZStd::equal_to<int>, AZ::StdLegacyAllocator> ValueToIndex;
        ValueToIndex valueToIndex_;
        typedef AZStd::unordered_map<int, const char*, AZStd::hash<int>, AZStd::equal_to<int>, AZ::StdLegacyAllocator> ValueToName;
        ValueToName valueToName_;
        typedef AZStd::unordered_map<int, const char*, AZStd::hash<int>, AZStd::equal_to<int>, AZ::StdLegacyAllocator> ValueToLabel;
        ValueToName valueToLabel_;
        AZStd::vector<int, AZ::StdLegacyAllocator> values_;
        TypeID type_;        
    };

    template<class Enum>
    class EnumDescriptionImpl
        : public CEnumDescription
    {
        EnumDescriptionImpl()
            : CEnumDescription(Serialization::TypeID::get<Enum>()) {}
    public:
        static CEnumDescription& the()
        {
            static EnumDescriptionImpl description;
            return description;
        }
    };

    template<class Enum>
    CEnumDescription& getEnumDescription()
    {
        return EnumDescriptionImpl<Enum>::the();
    }

    inline bool serializeEnum(const CEnumDescription& desc, IArchive& ar, int& value, const char* name, const char* label)
    {
        return desc.Serialize(ar, value, name, label);
    }
}

#define SERIALIZATION_ENUM_BEGIN(Type, label)                \
    namespace {                                              \
        bool registerEnum_##Type();                          \
        bool Type##_enum_registered = registerEnum_##Type(); \
        bool registerEnum_##Type(){                          \
            Serialization::CEnumDescription& description = Serialization::EnumDescriptionImpl<Type>::the();

#define SERIALIZATION_ENUM_BEGIN_NESTED(Class, Enum, label)                      \
    namespace {                                                                  \
        bool registerEnum_##Class##_##Enum();                                    \
        bool Class##_##Enum##_enum_registered = registerEnum_##Class##_##Enum(); \
        bool registerEnum_##Class##_##Enum(){                                    \
            Serialization::CEnumDescription& description = Serialization::EnumDescriptionImpl<Class::Enum>::the();

#define SERIALIZATION_ENUM_BEGIN_NESTED2(Class, Class1, Enum, label)                             \
    namespace {                                                                                  \
        bool registerEnum_##Class##Class1##_##Enum();                                            \
        bool Class##Class1##_##Enum##_enum_registered = registerEnum_##Class##Class1##_##Enum(); \
        bool registerEnum_##Class##Class1##_##Enum(){                                            \
            Serialization::CEnumDescription& description = Serialization::EnumDescriptionImpl<Class::Class1::Enum>::the();


#define SERIALIZATION_ENUM_VALUE(value, label) \
    static Serialization::CEnumDescription::NameValue AZ_JOIN(enumValue, __LINE__)(description, label, (int)value);

#define SERIALIZATION_ENUM(value, name, label) \
    static Serialization::CEnumDescription::NameValue AZ_JOIN(enumValue, __LINE__)(description, name, (int)value, label);

#define SERIALIZATION_ENUM_VALUE_NESTED(Class, value, label) \
    static Serialization::CEnumDescription::NameValue AZ_JOIN(enumValue, __LINE__)(description, #value, (int)Class::value, label);

#define SERIALIZATION_ENUM_VALUE_NESTED2(Class, Class1, value, label) \
    static Serialization::CEnumDescription::NameValue AZ_JOIN(enumValue, __LINE__)(description, #value, (int)Class::Class1::value, label);


#define SERIALIZATION_ENUM_END() \
    return true;                 \
    };                           \
    };

#include "EnumImpl.h"
// vim:ts=4 sw=4:

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_ENUM_H
