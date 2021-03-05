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
#include <SurfaceDataModule.h>

namespace SurfaceData
{
    class SurfaceDataEditorModule
        : public SurfaceDataModule
    {
    public:
        AZ_RTTI(SurfaceDataEditorModule, "{B80F2321-B79A-4161-B586-3E508655DFAF}", SurfaceDataModule);
        AZ_CLASS_ALLOCATOR(SurfaceDataEditorModule, AZ::SystemAllocator, 0);

        SurfaceDataEditorModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}