/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "LuaBuilderWorker.h"

namespace LuaBuilder
{
    class BuilderPluginComponent
        : public AZ::Component
    {
    public:
        ~BuilderPluginComponent() override = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        AZ_COMPONENT(BuilderPluginComponent, "{F85990CF-BF5F-4C02-9188-4C8698F20843}");

        static void Reflect(AZ::ReflectContext* context);

        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

    private:
        LuaBuilderWorker m_luaBuilder;
    };
}
