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

#include "UidGenerator.h"

#include <ctime>

namespace News
{

    UidGenerator::UidGenerator()
    {
        srand(static_cast<int>(time(nullptr)));
    }

    int UidGenerator::GenerateUid()
    {
        int uid;
        do
        {
            uid = rand();
        } 
        while (std::find(m_uids.begin(), m_uids.end(), uid) != m_uids.end());
        m_uids.push_back(uid);
        return uid;
    }

    int UidGenerator::AddUid(int uid)
    {
        if (std::find(m_uids.begin(), m_uids.end(), uid) == m_uids.end())
        {
            m_uids.push_back(uid);
        }
        return uid;
    }

    void UidGenerator::RemoveUid(int uid)
    {
        auto it = std::find(m_uids.begin(), m_uids.end(), uid);
        if (it != m_uids.end())
        {
            m_uids.erase(it);
        }
    }

    void UidGenerator::Clear()
    {
        m_uids.clear();
    }

}
