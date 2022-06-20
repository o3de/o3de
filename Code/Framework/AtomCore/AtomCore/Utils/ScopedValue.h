/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/functional.h>

namespace AZ
{
    //! Sets a variable upon construction and again when the object goes out of scope.
    template<typename T>
    class ScopedValue
    {
    private:
        T* m_ptr;
        T m_finalValue;

    public:
        ScopedValue(T* ptr, T initialValue, T finalValue) :
            m_ptr(ptr), m_finalValue(finalValue)
        {
            AZ_Assert(m_ptr, "ScopedValue::m_ptr is null");
            *m_ptr = initialValue;
        }

        ~ScopedValue()
        {
            *m_ptr = m_finalValue;
        }
    };

} // namespace AZ
