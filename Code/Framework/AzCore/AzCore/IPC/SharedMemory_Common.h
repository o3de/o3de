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
