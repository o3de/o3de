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

// Description : Macros and other definitions needed for TypeInfo declarations.


#ifndef CRYINCLUDE_CRYCOMMON_TYPEINFO_DECL_H
#define CRYINCLUDE_CRYCOMMON_TYPEINFO_DECL_H
#pragma once

#include <AzCore/Math/Uuid.h>

//////////////////////////////////////////////////////////////////////////
// Meta-type support.
//////////////////////////////////////////////////////////////////////////

// Currently enable type info for all platforms.
#if !defined(ENABLE_TYPE_INFO)
#define ENABLE_TYPE_INFO
#endif
#ifdef ENABLE_TYPE_INFO

struct CTypeInfo;

// If TypeInfo exists for T, it is accessed via TypeInfo(T*).
// Default TypeInfo() is implemented by a struct member function.
template<class T>
inline const CTypeInfo& TypeInfo(const T* t)
{
    return t->TypeInfo();
}

// Declare a class's TypeInfo member
#define STRUCT_INFO \
    const CTypeInfo&TypeInfo() const;

#define NULL_STRUCT_INFO \
    const CTypeInfo&TypeInfo() const   { return *(CTypeInfo*)0; }

// Declare an override for a type without TypeInfo() member (e.g. basic type)
#define DECLARE_TYPE_INFO(Type) \
    template<>                  \
    const CTypeInfo&TypeInfo(const Type*);

// Template version.
#define DECLARE_TYPE_INFO_T(Type) \
    template<class T>             \
    const CTypeInfo&TypeInfo(const Type<T>*);

// Type info declaration, with additional prototypes for string conversions.
#define BASIC_TYPE_INFO(Type)                   \
    string ToString(Type const & val);          \
    bool FromString(Type & val, const char* s); \
    DECLARE_TYPE_INFO(Type)

#define CUSTOM_STRUCT_INFO(Struct)   \
    const CTypeInfo&TypeInfo() const \
    { static Struct Info; return Info; }

#else // ENABLE_TYPE_INFO

#define STRUCT_INFO
#define NULL_STRUCT_INFO
#define DECLARE_TYPE_INFO(Type)
#define DECLARE_TYPE_INFO_T(Type)
#define BASIC_TYPE_INFO(T)

#endif // ENABLE_TYPE_INFO

// Specify automatic tool generation of TypeInfo bodies.
#define AUTO_STRUCT_INFO                            STRUCT_INFO
#define AUTO_TYPE_INFO                              DECLARE_TYPE_INFO
#define AUTO_TYPE_INFO_T                            DECLARE_TYPE_INFO_T

// Obsolete "LOCAL" versions (all infos now generated in local files).
#define AUTO_STRUCT_INFO_LOCAL              STRUCT_INFO
#define AUTO_TYPE_INFO_LOCAL                    DECLARE_TYPE_INFO
#define AUTO_TYPE_INFO_LOCAL_T              DECLARE_TYPE_INFO_T

// Overrides for basic types.
DECLARE_TYPE_INFO(void)

BASIC_TYPE_INFO(bool)
BASIC_TYPE_INFO(char)
BASIC_TYPE_INFO(wchar_t)
BASIC_TYPE_INFO(signed char)
BASIC_TYPE_INFO(unsigned char)
BASIC_TYPE_INFO(short)
BASIC_TYPE_INFO(unsigned short)
BASIC_TYPE_INFO(int)
BASIC_TYPE_INFO(unsigned int)
BASIC_TYPE_INFO(long)
BASIC_TYPE_INFO(unsigned long)
BASIC_TYPE_INFO(int64)
BASIC_TYPE_INFO(uint64)

BASIC_TYPE_INFO(float)
BASIC_TYPE_INFO(double)

BASIC_TYPE_INFO(AZ::Uuid)

DECLARE_TYPE_INFO(string)

// All pointers share same TypeInfo.
const CTypeInfo&PtrTypeInfo();
template<class T>
inline const CTypeInfo& TypeInfo([[maybe_unused]] T** t)
{
    return PtrTypeInfo();
}


#endif // CRYINCLUDE_CRYCOMMON_TYPEINFO_DECL_H
