/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace Multiplayer
{
    template <typename TYPE, uint32_t SIZE>
    constexpr RewindableFixedVector<TYPE, SIZE>::RewindableFixedVector(const TYPE& initialValue, uint32_t count)
    {
        m_container.fill(initialValue);
        m_rewindableSize = count;
    }

    template <typename TYPE, uint32_t SIZE>
    RewindableFixedVector<TYPE, SIZE>::~RewindableFixedVector()
    {
        ;
    }

    template <typename TYPE, uint32_t SIZE>
    bool RewindableFixedVector<TYPE, SIZE>::Serialize(AzNetworking::ISerializer& serializer)
    {
        if(!m_rewindableSize.Serialize(serializer))
        {
            return false;
        }

        for (uint32_t idx = 0; idx < size(); ++idx)
        {
            if(!m_container[idx].Serialize(serializer))
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
            if(!m_rewindableSize.Serialize(serializer))
            {
                return false;
            }

            if ((serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject) && origSize == m_rewindableSize)
            {
                deltaRecord.SetBit(SIZE, false);
            }
        }
        for (uint32_t idx = 0; idx < size(); ++idx)
        {
            if (deltaRecord.GetBit(idx))
            {
                serializer.ClearTrackedChangesFlag();
                if(!m_container[idx].Serialize(serializer))
                {
                    return false;
                }

                if ((serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject) && !serializer.GetTrackedChangesFlag())
                {
                    deltaRecord.SetBit(idx, false);
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

        for (uint32_t idx = 0; idx < bufferSize; ++idx)
        {
            m_container[idx] = buffer[idx];
        }
        
        return true;
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr RewindableFixedVector<TYPE, SIZE>& RewindableFixedVector<TYPE, SIZE>::operator=(const RewindableFixedVector<TYPE, SIZE>& rhs)
    {
        resize(rhs.size());
        for (uint32_t idx = 0; idx < size(); ++idx)
        {
            m_container[idx] = rhs.m_container[idx].Get();
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

        if (count > size())
        {
            for (uint32_t idx = size(); idx < count; ++idx)
            {
                m_container[idx] = TYPE();
            }
        }
        m_rewindableSize = count;

        return true;
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr bool RewindableFixedVector<TYPE, SIZE>::resize_no_construct(uint32_t count)
    {
        if (count > SIZE)
        {
            return false;
        }

        m_rewindableSize = count;

        return true;
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr void RewindableFixedVector<TYPE, SIZE>::clear()
    {
        for (uint32_t idx = 0; idx < SIZE; ++idx)
        {
            m_container[idx] = TYPE();
        }
        m_rewindableSize = 0;
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
            m_container[m_rewindableSize] = value;
            m_rewindableSize = m_rewindableSize + 1;
            return true;
        }

        return false;
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr bool RewindableFixedVector<TYPE, SIZE>::pop_back()
    {
        if (size() > 0)
        {
            m_rewindableSize = m_rewindableSize - 1;
            m_container[m_rewindableSize] = TYPE();
            return true;
        }

        return false;
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr bool RewindableFixedVector<TYPE, SIZE>::empty() const
    {
        return m_rewindableSize.Get() == 0;
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr const TYPE& RewindableFixedVector<TYPE, SIZE>::back() const
    {
        AZ_Assert(size() > 0, "Attempted to get back element of an empty RewindableFixedVector");
        return m_container[m_rewindableSize - 1].Get();
    }

    template <typename TYPE, uint32_t SIZE>
    constexpr uint32_t RewindableFixedVector<TYPE, SIZE>::size() const
    {
        return m_rewindableSize;
    }
}
