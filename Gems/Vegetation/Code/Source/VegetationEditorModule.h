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
#include <VegetationModule.h>

namespace Vegetation
{
    class VegetationEditorModule
        : public VegetationModule
    {
    public:
        AZ_RTTI(VegetationEditorModule, "{8BA356E4-A07D-46A4-ADE1-B17F3BA032BF}", VegetationModule);
        AZ_CLASS_ALLOCATOR(VegetationEditorModule, AZ::SystemAllocator, 0);

        VegetationEditorModule();

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}