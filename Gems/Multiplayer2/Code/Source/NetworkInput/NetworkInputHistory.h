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

#include <Source/NetworkInput/NetworkInput.h>
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
