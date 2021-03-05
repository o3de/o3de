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

#include "SurfaceData_precompiled.h"
#include <AzCore/Module/Module.h>

namespace SurfaceData
{
    class SurfaceDataModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(SurfaceDataModule, "{B58B7CA8-98C9-4DC8-8607-E094989BBBE2}", AZ::Module);
        AZ_CLASS_ALLOCATOR(SurfaceDataModule, AZ::SystemAllocator, 0);

        SurfaceDataModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}