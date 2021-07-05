/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
