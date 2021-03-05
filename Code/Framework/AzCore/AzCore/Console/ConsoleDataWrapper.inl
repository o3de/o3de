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

#include <sstream>
#include <AzCore/Console/ConsoleTypeHelpers.h>


namespace AZ
{
    template <typename BASE_TYPE, ThreadSafety THREAD_SAFETY>
    inline ConsoleDataWrapper<BASE_TYPE, THREAD_SAFETY>::ConsoleDataWrapper
    (
        const BASE_TYPE& value,
        CallbackFunc callback,
        const char* name,
        const char* desc,
        ConsoleFunctorFlags flags
    )
        : m_callback(callback)
        , m_functor(name, desc, flags, AzTypeInfo<BASE_TYPE>::Uuid(), *this, &ConsoleDataWrapper<BASE_TYPE, THREAD_SAFETY>::CvarFunctor)
    {
        this->m_value = value;
    }

    template <typename BASE_TYPE, ThreadSafety THREAD_SAFETY>
    inline void ConsoleDataWrapper<BASE_TYPE, THREAD_SAFETY>::operator =(const BASE_TYPE& rhs)
    {
        this->m_value = rhs;
    }

    template <typename BASE_TYPE, ThreadSafety THREAD_SAFETY>
    inline ConsoleDataWrapper<BASE_TYPE, THREAD_SAFETY>::operator BASE_TYPE() const
    {
        const BASE_TYPE currentValue = this->m_value;
        return currentValue;
    }

    template <typename BASE_TYPE, ThreadSafety THREAD_SAFETY>
    inline bool ConsoleDataWrapper<BASE_TYPE, THREAD_SAFETY>::operator ==(const BASE_TYPE& rhs) const
    {
        const BASE_TYPE currentValue = this->m_value;
        return currentValue == rhs;
    }

    template <typename BASE_TYPE, ThreadSafety THREAD_SAFETY>
    inline bool ConsoleDataWrapper<BASE_TYPE, THREAD_SAFETY>::operator !=(const BASE_TYPE& rhs) const
    {
        const BASE_TYPE currentValue = this->m_value;
        return currentValue != rhs;
    }

    template <typename BASE_TYPE, ThreadSafety THREAD_SAFETY>
    inline bool ConsoleDataWrapper<BASE_TYPE, THREAD_SAFETY>::operator <(const BASE_TYPE& rhs) const
    {
        const BASE_TYPE currentValue = this->m_value;
        return currentValue < rhs;
    }

    template <typename BASE_TYPE, ThreadSafety THREAD_SAFETY>
    inline bool ConsoleDataWrapper<BASE_TYPE, THREAD_SAFETY>::operator <=(const BASE_TYPE& rhs) const
    {
        const BASE_TYPE currentValue = this->m_value;
        return currentValue <= rhs;
    }

    template <typename BASE_TYPE, ThreadSafety THREAD_SAFETY>
    inline bool ConsoleDataWrapper<BASE_TYPE, THREAD_SAFETY>::operator >(const BASE_TYPE& rhs) const
    {
        const BASE_TYPE currentValue = this->m_value;
        return currentValue > rhs;
    }

    template <typename BASE_TYPE, ThreadSafety THREAD_SAFETY>
    inline bool ConsoleDataWrapper<BASE_TYPE, THREAD_SAFETY>::operator >=(const BASE_TYPE& rhs) const
    {
        const BASE_TYPE currentValue = this->m_value;
        return currentValue >= rhs;
    }

    template <typename BASE_TYPE, ThreadSafety THREAD_SAFETY>
    bool ConsoleDataWrapper<BASE_TYPE, THREAD_SAFETY>::StringToValue(const ConsoleCommandContainer& arguments)
    {
        const BASE_TYPE currentValue = this->m_value;
        BASE_TYPE newValue = currentValue;

        if (ConsoleTypeHelpers::StringSetToValue(newValue, arguments))
        {
            if (newValue != currentValue)
            {
                this->m_value = newValue;
                InvokeCallback();
            }

            return true;
        }

        return false;
    }

    template <typename BASE_TYPE, ThreadSafety THREAD_SAFETY>
    void ConsoleDataWrapper<BASE_TYPE, THREAD_SAFETY>::ValueToString(CVarFixedString& outString) const
    {
        const BASE_TYPE currentValue = this->m_value;
        outString = ConsoleTypeHelpers::ValueToString(currentValue);
    }

    template <typename BASE_TYPE, ThreadSafety THREAD_SAFETY>
    inline void ConsoleDataWrapper<BASE_TYPE, THREAD_SAFETY>::InvokeCallback() const
    {
        if (m_callback)
        {
            const BASE_TYPE currentValue = this->m_value;
            m_callback(currentValue);
        }
    }

    template <typename BASE_TYPE, ThreadSafety THREAD_SAFETY>
    inline void ConsoleDataWrapper<BASE_TYPE, THREAD_SAFETY>::CvarFunctor(const ConsoleCommandContainer& arguments)
    {
        StringToValue(arguments);
    }
}
