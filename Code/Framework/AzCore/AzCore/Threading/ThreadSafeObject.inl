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
