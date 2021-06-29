/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>

namespace EMStudio
{
    class PluginOptions
    {
    public:
        AZ_RTTI(PluginOptions, "{8305D7EE-9C52-456B-A1D1-4BE1500CD06A}");

        virtual ~PluginOptions() {}
    };
}
