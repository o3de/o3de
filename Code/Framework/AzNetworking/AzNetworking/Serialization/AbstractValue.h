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

#pragma once

#include <AzCore/std/string/string.h>

namespace AbstractValue
{
    class BaseValue {};

    template <typename T>
    class ValueT
        : public BaseValue
    {
    public:
        ValueT() : m_value() {}
        ValueT(const T& value) : m_value(value) {}
        const T& GetValue() const { return m_value; }
    private:
        T m_value;
    };

    template <>
    class ValueT<char*>
        : public BaseValue
    {
    public:
        ValueT() : m_value() {}
        ValueT(const char* value) : m_value(value) {}
        const char* GetValue() const { return m_value.c_str(); }
    private:
        AZStd::string m_value;
    };

    using Bool   = ValueT<bool>;
    using Char   = ValueT<char>;
    using Float  = ValueT<float>;
    using Double = ValueT<double>;
    using String = ValueT<char*>;
    using Int8   = ValueT<int8_t>;
    using Int16  = ValueT<int16_t>;
    using Int32  = ValueT<int32_t>;
    using Int64  = ValueT<int64_t>;
    using UInt8  = ValueT<uint8_t>;
    using UInt16 = ValueT<uint16_t>;
    using UInt32 = ValueT<uint32_t>;
    using UInt64 = ValueT<uint64_t>;
}
