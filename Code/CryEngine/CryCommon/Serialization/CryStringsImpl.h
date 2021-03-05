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

#pragma once

#include "Serialization/IArchive.h"
#include "Serialization/CryStrings.h"

namespace Serialization
{
    template< class TFixedStringClass >
    class CFixedStringSerializer
        : public IString
    {
    public:
        CFixedStringSerializer(TFixedStringClass& str)
            : str_(str) { }

        void set(const char* value) { str_ = value; }
        const char* get() const { return str_.c_str(); }
        const void* handle() const { return &str_; }
        TypeID type() const { return TypeID::get<TFixedStringClass>(); }
    private:
        TFixedStringClass& str_;
    };

    template< class TFixedStringClass >
    class CFixedWStringSerializer
        : public IWString
    {
    public:
        CFixedWStringSerializer(TFixedStringClass& str)
            : str_(str) { }

        void set(const wchar_t* value) { str_ = value; }
        const wchar_t* get() const { return str_.c_str(); }
        const void* handle() const { return &str_; }
        TypeID type() const { return TypeID::get<TFixedStringClass>(); }
    private:
        TFixedStringClass& str_;
    };
}

template< size_t N >
inline bool Serialize(Serialization::IArchive& ar, CryFixedStringT< N >& value, const char* name, const char* label)
{
    Serialization::CFixedStringSerializer< CryFixedStringT< N > > str(value);
    return ar(static_cast<Serialization::IString&>(str), name, label);
}

template< size_t N >
inline bool Serialize(Serialization::IArchive& ar, CryStackStringT< char, N >& value, const char* name, const char* label)
{
    Serialization::CFixedStringSerializer< CryStackStringT< char, N > > str(value);
    return ar(static_cast<Serialization::IString&>(str), name, label);
}

template< size_t N >
inline bool Serialize(Serialization::IArchive& ar, CryStackStringT< wchar_t, N >& value, const char* name, const char* label)
{
    Serialization::CFixedWStringSerializer< CryStackStringT< wchar_t, N > > str(value);
    return ar(static_cast<Serialization::IWString&>(str), name, label);
}

