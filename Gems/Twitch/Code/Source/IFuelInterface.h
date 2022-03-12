/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <HttpRequestor/HttpRequestorBus.h>
#include <Twitch/TwitchTypes.h>

namespace Twitch
{
    class AZ_DEPRECATED(,"The IFuelInterface has been deprecated. HTTPRequest functionality has been moved to TwitchREST. Auth has been mvoed to TwitchSystemComponent. All remaining has been deprecated.")
        IFuelInterface
    {

    };
}
