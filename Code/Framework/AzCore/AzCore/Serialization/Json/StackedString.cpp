/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/StackedString.h>

namespace AZ
{
    StackedString::StackedString(Format format)
        : m_format(format)
    {
    }

    void StackedString::Push(AZStd::string_view value)
    {
        m_offsetStack.push(m_string.length());

        if (!value.empty())
        {
            switch (m_format)
            {
            case Format::ContextPath:
                if (!m_string.empty())
                {
                    m_string += '.';
                }
                break;
            case Format::JsonPointer:
                m_string += '/';
                break;
            default:
                if (!m_string.empty())
                {
                    m_string += ' ';
                }
                break;
            }

            m_string.append(value.data(), value.length());
        }
    }
    
    void StackedString::Push(size_t value)
    {
        char buffer[16];
        azsnprintf(buffer, AZ_ARRAY_SIZE(buffer), "%zu", value);
        Push(buffer);
    }

    void StackedString::Pop()
    {
        if (!m_offsetStack.empty())
        {
            size_t value = m_offsetStack.top();
            m_string.erase(m_string.begin() + value, m_string.end());
            m_offsetStack.pop();
        }
    }

    void StackedString::Reset()
    {
        m_offsetStack = {};
        m_string = OSString{};
    }


    AZStd::string_view StackedString::Get() const
    {
        if (m_string.empty())
        {
            switch (m_format)
            {
            case Format::ContextPath:
                return AZStd::string_view("<root>");
            case Format::JsonPointer:
                return AZStd::string_view("");
            default:
                return AZStd::string_view("<>");
                break;
            }
        }
        else
        {
            return m_string;
        }
    }

    StackedString::operator AZStd::string_view() const
    {
        return Get();
    }


    ScopedStackedString::ScopedStackedString(StackedString& string, AZStd::string_view value)
        : m_string(string)
    {
        string.Push(value);
    }

    ScopedStackedString::ScopedStackedString(StackedString& string, size_t value)
        : m_string(string)
    {
        string.Push(value);
    }

    ScopedStackedString::~ScopedStackedString()
    {
        m_string.Pop();
    }
} // namespace AZ
