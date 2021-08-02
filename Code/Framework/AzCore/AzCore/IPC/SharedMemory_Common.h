/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AZ
{
    class SharedMemory_Common
    {
    public:
        enum AccessMode
        {
            ReadOnly,
            ReadWrite
        };

        enum CreateResult
        {
            CreateFailed,
            CreatedNew,
            CreatedExisting,
        };

    protected:
        SharedMemory_Common() :
            m_mappedBase(nullptr),
            m_dataSize(0)
        {
            m_name[0] = '\0';
        }

        ~SharedMemory_Common()
        {
        }

        char m_name[128];
        void* m_mappedBase;
        unsigned int m_dataSize;
    };
}
