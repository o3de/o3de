/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "TimeValue.h"
#include <ISerialize.h> // <> required for Interfuscator

template<bool READING, ESerializationTarget TARGET>
class CSimpleSerializeImpl
{
public:
    CSimpleSerializeImpl()
        : m_failed(false)
    {
    }
    ILINE bool IsReading() const
    {
        return READING;
    }
    ILINE void BeginGroup(const char* szName)
    {
    }
    ILINE void EndGroup()
    {
    }

    ILINE bool Ok() const
    {
        return !m_failed;
    }

protected:
    ILINE void Failed()
    {
        m_failed = true;
    }

private:
    bool m_failed;
};

template<class Impl>
class CSimpleSerialize : public ISerialize
{
public:
    ILINE CSimpleSerialize(Impl& impl)
        : m_impl(impl)
    {
    }

    void BeginGroup(const char* szName) override
    {
        m_impl.BeginGroup(szName);
    }

    bool BeginOptionalGroup(const char* szName, bool condition) override
    {
        return m_impl.BeginOptionalGroup(szName, condition);
    }

    void EndGroup() override
    {
        m_impl.EndGroup();
    }

    bool IsReading() const override
    {
        return m_impl.IsReading();
    }

    void WriteStringValue(const char* name, SSerializeString& value) override
    {
        m_impl.Value(name, value);
    }
    void ReadStringValue(const char* name, SSerializeString& curValue) override
    {
        m_impl.Value(name, curValue);
    }

#define SERIALIZATION_TYPE(T)                   \
    void Value(const char* name, T& x) override \
    {                                           \
        m_impl.Value(name, x);                  \
    }
#include "SerializationTypes.h"
#undef SERIALIZATION_TYPE

    Impl* GetInnerImpl()
    {
        return &m_impl;
    }

protected:
    Impl& m_impl;
};

//////////////////////////////////////////////////////////////////////////
// Support serialization with default values,
// Require Implementation serialization stub to have Value() method returning boolean.
//////////////////////////////////////////////////////////////////////////
template<class Impl>
using CSimpleSerializeWithDefaults = CSimpleSerialize<Impl>;
