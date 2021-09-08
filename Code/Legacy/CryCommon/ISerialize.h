/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Description : main header file
#pragma once

#include <AzCore/Math/Vector3.h>
#include <Cry_Math.h>
#include <IXml.h>
#include <StlUtils.h>

// Forward declarations
class CTimeValue;

// this enumeration details what "kind" of serialization we are
// performing, so that classes that want to, for instance, tailor
// the data they present depending on where data is being written
// to can do so
enum class ESerializationTarget
{
    eST_SaveGame,
};

//////////////////////////////////////////////////////////////////////////
// Temporary class for string serialization.
//////////////////////////////////////////////////////////////////////////
struct SSerializeString
{
    SSerializeString(){};
    SSerializeString(const SSerializeString& src)
    {
        m_str.assign(src.c_str());
    };
    explicit SSerializeString(const char* sbegin, const char* send)
        : m_str(sbegin, send){};
    ~SSerializeString()
    {
    }

    // Casting to const char*
    SSerializeString(const char* s)
        : m_str(s){};

    SSerializeString& operator=(const SSerializeString& src)
    {
        m_str.assign(src.c_str());
        return *this;
    }
    SSerializeString& operator=(const char* src)
    {
        m_str.assign(src);
        return *this;
    }

    bool operator!=(const SSerializeString& src)
    {
        return m_str != src.m_str;
    }

    size_t size() const
    {
        return m_str.size();
    }
    size_t length() const
    {
        return m_str.length();
    }
    const char* c_str() const
    {
        return m_str.c_str();
    };
    bool empty() const
    {
        return m_str.empty();
    }
    void resize(int sz)
    {
        m_str.resize(sz);
    }
    void reserve(int sz)
    {
        m_str.reserve(sz);
    }

    void set_string(const AZStd::string& s)
    {
        m_str.assign(s.begin(), s.size());
    }
    operator const AZStd::string() const
    {
        return m_str;
    }

private:
    AZStd::string m_str;
};

// the ISerialize is intended to be implemented by objects that need
// to read and write from various data sources, in such a way that
// different tradeoffs can be balanced by the object that is being
// serialized, and so that objects being serialized need only write
// a single function in order to be read from and written to
struct ISerialize
{
    static const int ENUM_POLICY_TAG = 0xe0000000;

    ISerialize()
    {
    }

    virtual ~ISerialize()
    {
    }

    // this is for string values -- they need special support
    virtual void ReadStringValue(const char* name, SSerializeString& curValue) = 0;
    virtual void WriteStringValue(const char* name, SSerializeString& buffer) = 0;

    //////////////////////////////////////////////////////////////////////////
    // these functions should be implemented to deal with groups
    //////////////////////////////////////////////////////////////////////////

    // Begins a serialization group - must be matched by an EndGroup
    // szName is preferably as short as possible for performance reasons
    // Spaces in szName cause undefined behaviour, use alpha characters,underscore and numbers only for a name.
    virtual void BeginGroup(const char* szName) = 0;
    virtual bool BeginOptionalGroup(const char* szName, bool condition) = 0;
    virtual void EndGroup() = 0;
    //////////////////////////////////////////////////////////////////////////

    virtual bool IsReading() const = 0;

    // declare all primitive Value() implementations
#define SERIALIZATION_TYPE(T) \
    virtual void Value(const char* name, T& x) = 0;
#include "SerializationTypes.h"
#undef SERIALIZATION_TYPE
};

// this class provides a wrapper so that ISerialize can be used much more
// easily; it is a template so that if we need to wrap a more specific
// ISerialize implementation we can do so easily
template<class TISerialize>
class CSerializeWrapper
{
public:
    CSerializeWrapper(TISerialize* pSerialize)
        : m_pSerialize(pSerialize)
    {
    }

    // we provide a wrapper around the abstract implementation
    // ISerialize to allow easy changing of our
    // interface, and easy implementation of our details.
    // some of the wrappers are trivial, however for consistency, they
    // have been made to follow the trend.

    // the value function allows us to declare that a value needs
    // to be serialized/deserialized;

    template<typename T_Value>
    ILINE void Value(const char* szName, T_Value& value)
    {
        m_pSerialize->Value(szName, value);
    }

    void Value(const char* szName, AZStd::string& value)
    {
        if (IsWriting())
        {
            SSerializeString& serializeString = SetSharedSerializeString(value);
            m_pSerialize->WriteStringValue(szName, serializeString);
        }
        else
        {
            value = "";
            SSerializeString& serializeString = SetSharedSerializeString(value);
            m_pSerialize->ReadStringValue(szName, serializeString);
            value = serializeString.c_str();
        }
    }
    void Value(const char* szName, const AZStd::string& value)
    {
        if (IsWriting())
        {
            SSerializeString& serializeString = SetSharedSerializeString(value);
            m_pSerialize->WriteStringValue(szName, serializeString);
        }
        else
        {
            assert(0 && "This function can only be used for Writing");
        }
    }

    // groups help us find common data
    ILINE void BeginGroup(const char* szName)
    {
        m_pSerialize->BeginGroup(szName);
    }
    ILINE bool BeginOptionalGroup(const char* szName, bool condition)
    {
        return m_pSerialize->BeginOptionalGroup(szName, condition);
    }
    ILINE void EndGroup()
    {
        m_pSerialize->EndGroup();
    }

    ILINE bool IsWriting() const
    {
        return !m_pSerialize->IsReading();
    }
    ILINE bool IsReading() const
    {
        return m_pSerialize->IsReading();
    }

    friend ILINE TISerialize* GetImpl(CSerializeWrapper<TISerialize> ser)
    {
        return ser.m_pSerialize;
    }

    operator CSerializeWrapper<ISerialize>()
    {
        return CSerializeWrapper<ISerialize>(m_pSerialize);
    }

    SSerializeString& SetSharedSerializeString(const AZStd::string& str)
    {
        static SSerializeString serializeString;
        serializeString.set_string(str);
        return serializeString;
    }

private:
    TISerialize* m_pSerialize;
};

// default serialize class to use!!
typedef CSerializeWrapper<ISerialize> TSerialize;
