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
#include <FastNoiseModule.h>

namespace FastNoiseGem
{
    class FastNoiseEditorModule
        : public FastNoiseModule
    {
    public:
        AZ_RTTI(FastNoiseEditorModule, "{119E8542-E0D2-4D58-9C87-B8E7CF032274}", FastNoiseModule);
        AZ_CLASS_ALLOCATOR(FastNoiseEditorModule, AZ::SystemAllocator, 0);

        FastNoiseEditorModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}