/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <IGem.h>
#include <AzCore/Component/Component.h>

namespace LyShine
{
    class LyShineModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(LyShineModule, "{5B98FB11-A597-47DB-8BE8-74F44D957C67}", CryHooksModule);

        LyShineModule();

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
