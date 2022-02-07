/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Printers.h"
#include <ostream>
#include <AzFramework/Input/Channels/InputChannel.h>

namespace AzFramework
{
    void PrintTo(const InputChannel::State& state, std::ostream* os)
    {
        switch(state)
        {
            case InputChannel::State::Began:
                *os << "Began";
                break;
            case InputChannel::State::Ended:
                *os << "Ended";
                break;
            case InputChannel::State::Idle:
                *os << "Idle";
                break;
            case InputChannel::State::Updated:
                *os << "Updated";
                break;
        }
    }
} // namespace AzFramework
