/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
    inline const uint8_t* NetworkOutputSerializer::GetUnreadData() const
    {
        return (const uint8_t*)(m_buffer + m_bufferPosition);
    }

    inline uint32_t NetworkOutputSerializer::GetUnreadSize() const
    {
        return (m_bufferCapacity - m_bufferPosition);
    }

    inline uint32_t NetworkOutputSerializer::GetReadSize() const
    {
        return m_bufferPosition;
    }
}
