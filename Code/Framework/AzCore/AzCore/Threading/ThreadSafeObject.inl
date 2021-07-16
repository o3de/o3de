/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AZ
{
    template <typename _TYPE>
    inline ThreadSafeObject<_TYPE>::ThreadSafeObject(const _TYPE& rhs)
        : m_object(rhs)
    {
        ;
    }

    template <typename _TYPE>
    inline void ThreadSafeObject<_TYPE>::operator =(const _TYPE& value)
    {
        AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
        m_object = value;
    }

    template <typename _TYPE>
    inline ThreadSafeObject<_TYPE>::operator _TYPE() const
    {
        AZStd::scoped_lock<AZStd::mutex> lock(m_mutex);
        return m_object;
    }
}
