/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SurfaceDataModule.h>

namespace SurfaceData
{
    class SurfaceDataEditorModule
        : public SurfaceDataModule
    {
    public:
        AZ_RTTI(SurfaceDataEditorModule, "{B80F2321-B79A-4161-B586-3E508655DFAF}", SurfaceDataModule);
        AZ_CLASS_ALLOCATOR(SurfaceDataEditorModule, AZ::SystemAllocator);

        SurfaceDataEditorModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
