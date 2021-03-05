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
