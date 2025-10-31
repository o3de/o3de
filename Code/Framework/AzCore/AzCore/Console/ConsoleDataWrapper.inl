/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <sstream>
#include <AzCore/Console/ConsoleTypeHelpers.h>
#include <AzCore/RTTI/TypeInfo.h>

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
        const BASE_TYPE currentValue = this->m_value;
        // Do the value assignment outside new value check.
        // Client code can supply a type for m_value that overrides the operator= function and trigger side effects
        // in the operator= function body. Doing the assignment outside the value change check avoids those side
        // effects not being triggered because AzCore believes the value wouldn't change.
        this->m_value = rhs;
        if (currentValue != rhs)
        {
            InvokeCallback();
        }
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

        if (ConsoleTypeHelpers::ToValue(newValue, arguments))
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
        outString = ConsoleTypeHelpers::ToString(currentValue);
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
