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

namespace Multiplayer
{
    template <typename TYPE, uint32_t SIZE>
    constexpr RewindableFixedVector<TYPE, SIZE>::RewindableFixedVector(const TYPE& initialValue, uint32_t count)
    {
        m_container.resize(count, initialValue);
        m_rewindableSize = m_container.size();
    }

    template <typename TYPE, uint32_t SIZE>
    RewindableFixedVector<TYPE, SIZE>::~RewindableFixedVector()
    {
        ;
    }

    template <typename TYPE, uint32_t SIZE>
    bool RewindableFixedVector<TYPE, SIZE>::Serialize(AzNetworking::ISerializer& serializer)
    {
        m_rewindableSize = m_container.size();
        if(!m_rewindableSize.Serialize(serializer) && !resize(m_rewindableSize))
        {
            return false;
        }

        for (uint32_t i = 0; i < size(); ++i)
        {
            if(!m_container[i].Serialize(serializer))
            {
                return false;
            }
        }

        return serializer.IsValid();
    }

    template <typename TYPE, uint32_t SIZE>
    bool RewindableFixedVector<TYPE, SIZE>::Serialize(AzNetworking::ISerializer& serializer, AzNetworking::IBitset& deltaRecord)
    {
        if (deltaRecord.GetBit(SIZE))
        {
            const uint32_t origSize = m_rewindableSize;
            m_rewindableSize = m_container.size();
            if(!m_rewindableSize.Serialize(serializer) && !resize(m_rewindableSize))
            {
                return false;
            }

            if ((serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject) && origSize == m_rewindableSize)
            {
                deltaRecord.SetBit(SIZE, false);
            }
        }
        for (uint32_t i = 0; i < size(); ++i)
        {
            if (deltaRecord.GetBit(i))
            {
                serializer.ClearTrackedChangesFlag();
                if(!m_container[i].Serialize(serializer))
                {
                    return false;
                }

                if ((serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject) && !serializer.GetTrackedChangesFlag())
                {
                    deltaRecord.SetBit(i, false);
                }
            }
        }

        return serializer.IsValid();
    }
    
    template <typename TYPE, uint32_t SIZE>
    constexpr bool RewindableFixedVector<TYPE, SIZE>::copy_values(const TYPE* buffer, uint32_t bufferSize)
    {
        if (!resize(bufferSize))
        {
            return false;
        }

        for (uint32_t idx = 0; idx < bufferSize; ++i)
        {
            m_container[idx] = buffer[idx];
        }
        
        return true;
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr RewindableFixedVector<TYPE, SIZE>& RewindableFixedVector<TYPE, SIZE>::operator=(const RewindableFixedVector<TYPE, SIZE>& rhs)
    {
        resize(rhs.size());
        for (uint32_t idx = 0; idx < size(); ++i)
        {
            m_container[idx] = rhs.m_container[idx];
        }
        return *this;
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr bool RewindableFixedVector<TYPE, SIZE>::operator ==(const RewindableFixedVector<TYPE, SIZE>& rhs) const
    {
        return m_container == rhs.m_container && m_rewindableSize == rhs.m_rewindableSize;
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr bool RewindableFixedVector<TYPE, SIZE>::operator !=(const RewindableFixedVector<TYPE, SIZE>& rhs) const
    {
        return !(*this == rhs);
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr bool RewindableFixedVector<TYPE, SIZE>::resize(uint32_t count)
    {
        if (count > SIZE)
        {
            return false;
        }

        if (count == size())
        {
            return true;
        }

        m_container.resize(count, TYPE());
        m_rewindableSize = m_container.size();

        return true;
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr bool RewindableFixedVector<TYPE, SIZE>::resize_no_construct(uint32_t count)
    {
        if (count > SIZE)
        {
            return false;
        }

        m_container.resize_no_construct(count);
        m_rewindableSize = m_container.size();

        return true;
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr void RewindableFixedVector<TYPE, SIZE>::clear()
    {
        m_container.clear();
        m_rewindableSize = m_container.size();
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr const TYPE& RewindableFixedVector<TYPE, SIZE>::operator[](uint32_t index) const
    {
        AZ_Assert(index < size(), "Out of bounds access (requested %u, reserved %u)", index, size());
        return m_container[index].Get();
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr TYPE& RewindableFixedVector<TYPE, SIZE>::operator[](uint32_t index)
    {
        AZ_Assert(index < size(), "Out of bounds access (requested %u, reserved %u)", index, size());
        return m_container[index].Modify();
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr bool RewindableFixedVector<TYPE, SIZE>::push_back(const TYPE& value)
    {
        if (size() < SIZE)
        {
            m_container.push_back(value);
            m_rewindableSize = m_container.size();
            return true;
        }

        return false;
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr bool RewindableFixedVector<TYPE, SIZE>::pop_back()
    {
        if (size() > 0)
        {
            m_container.pop_back();
            m_rewindableSize = m_container.size();
            return true;
        }

        return false;
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr bool RewindableFixedVector<TYPE, SIZE>::empty() const
    {
        return m_container.empty();
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr const TYPE& RewindableFixedVector<TYPE, SIZE>::back() const
    {
        AZ_Assert(size() > 0, "Attempted to get back element of an empty RewindableFixedVector");
        return m_container.back().Get();
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr uint32_t RewindableFixedVector<TYPE, SIZE>::size() const
    {
        return m_rewindableSize;
    }
}
