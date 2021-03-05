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

#include "GradientSignal_precompiled.h"
#include <AzCore/Module/Module.h>

namespace GradientSignal
{
    class GradientSignalModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(GradientSignalModule, "{B3CBEC4A-599F-4B60-94E1-112B61FE78C5}", AZ::Module);
        AZ_CLASS_ALLOCATOR(GradientSignalModule, AZ::SystemAllocator, 0);

        GradientSignalModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}