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

#include "FastNoise_precompiled.h"
#include <AzCore/Module/Module.h>

namespace FastNoiseGem
{
    class FastNoiseModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(FastNoiseModule, "{D2E0B087-0033-4D23-8985-C2FD46BDE080}", AZ::Module);
        AZ_CLASS_ALLOCATOR(FastNoiseModule, AZ::SystemAllocator, 0);

        FastNoiseModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}