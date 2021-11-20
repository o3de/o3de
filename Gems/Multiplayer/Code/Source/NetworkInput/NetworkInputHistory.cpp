/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/NetworkInput/NetworkInputHistory.h>

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
