/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <iosfwd>
#include <AzFramework/Input/Channels/InputChannel.h>

namespace AzFramework
{
    void PrintTo(const InputChannel::State& state, std::ostream* os);
} // namespace AzFramework
