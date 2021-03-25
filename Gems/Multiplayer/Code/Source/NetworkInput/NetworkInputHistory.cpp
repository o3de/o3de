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

#include <Source/NetworkInput/NetworkInputHistory.h>

namespace Multiplayer
{
    AZStd::size_t NetworkInputHistory::Size() const
    {
        return m_history.size();
    }

    const NetworkInput& NetworkInputHistory::operator[](AZStd::size_t index) const
    {
        return m_history[index].m_networkInput;
    }

    NetworkInput& NetworkInputHistory::operator[](AZStd::size_t index)
    {
        return m_history[index].m_networkInput;
    }

    void NetworkInputHistory::PushBack(NetworkInput& networkInput)
    {
        m_history.push_back(Wrapper(networkInput));
    }

    void NetworkInputHistory::PopFront()
    {
        m_history.pop_front();
    }

    const NetworkInput& NetworkInputHistory::Front() const
    {
        return m_history.front().m_networkInput;
    }
}
