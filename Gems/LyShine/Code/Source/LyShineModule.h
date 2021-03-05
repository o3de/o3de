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
