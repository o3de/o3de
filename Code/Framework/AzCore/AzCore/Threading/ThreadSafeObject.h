/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/scoped_lock.h>

namespace AZ
{
    //! @class ThreadSafeObject
    //! Wraps an object in a thread-safe interface.
    template <typename _TYPE>
    class ThreadSafeObject
    {
    public:

        ThreadSafeObject() = default;
        ThreadSafeObject(const _TYPE& rhs);
        ~ThreadSafeObject() = default;

        //! Assigns a new value to the object under lock.
        //! @param value the new value to set the object instance to
        void operator =(const _TYPE& value);

        //! Implicit conversion to underlying type.
        //! @return a copy of the current value of the object retrieved under lock
        operator _TYPE() const;

    private:

        AZ_DISABLE_COPY_MOVE(ThreadSafeObject);

        mutable AZStd::mutex m_mutex;
        _TYPE m_object;
    };
}

#include <AzCore/Threading/ThreadSafeObject.inl>
