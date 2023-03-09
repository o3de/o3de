/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <FastNoiseModule.h>

namespace FastNoiseGem
{
    class FastNoiseEditorModule
        : public FastNoiseModule
    {
    public:
        AZ_RTTI(FastNoiseEditorModule, "{119E8542-E0D2-4D58-9C87-B8E7CF032274}", FastNoiseModule);
        AZ_CLASS_ALLOCATOR(FastNoiseEditorModule, AZ::SystemAllocator);

        FastNoiseEditorModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
