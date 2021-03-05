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

#include "Vegetation_precompiled.h"
#include <AzCore/Module/Module.h>

namespace Vegetation
{
    class VegetationModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(VegetationModule, "{AEA5121D-425F-4460-8C0F-02AA69D6B480}", AZ::Module);
        AZ_CLASS_ALLOCATOR(VegetationModule, AZ::SystemAllocator, 0);

        VegetationModule();

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}