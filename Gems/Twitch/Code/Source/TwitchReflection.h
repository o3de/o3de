/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace Twitch
{
    namespace Internal
    {
        /*
        ** Reflect the different aspects of math (depending on context)
        */

        void Reflect(AZ::BehaviorContext & context);
    }
}
