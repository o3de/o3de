/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/NetworkInput/NetworkInput.h>
#include <AzCore/std/containers/deque.h>

namespace Multiplayer
{
    //! @class NetworkInputHistory
    //! @brief A list of input commands, used for bookkeeping on the client.
    class NetworkInputHistory final
    {
    public:
        AZStd::size_t Size() const;

        const NetworkInput& operator[](AZStd::size_t index) const;
        NetworkInput& operator[](AZStd::size_t index);

        void PushBack(NetworkInput& networkInput);
        void PopFront();
        const NetworkInput& Front() const;

    private:

        struct Wrapper // Strictly a workaround to deal with the private constructor of NetworkInput
        {
            Wrapper() : m_networkInput() {}
            Wrapper(const NetworkInput& networkInput) : m_networkInput(networkInput) {}
            NetworkInput m_networkInput;
        };

        AZStd::deque<Wrapper> m_history;
    };
}
