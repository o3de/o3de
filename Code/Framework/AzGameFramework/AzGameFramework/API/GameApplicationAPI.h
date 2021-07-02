/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZGAMEFRAMEWORK_GAMEAPPLICATIONAPI_H
#define AZGAMEFRAMEWORK_GAMEAPPLICATIONAPI_H

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AzGameFramework
{
    class GameApplicationEvents
        : public AZ::EBusTraits
    {
    public:

        using Bus = AZ::EBus<GameApplicationEvents>;
    };
} // namespace AzGameFramework

#endif // AZGAMEFRAMEWORK_GAMEAPPLICATIONAPI_H
