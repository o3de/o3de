/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Module.h>

namespace GradientSignal
{
    class GradientSignalModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(GradientSignalModule, "{B3CBEC4A-599F-4B60-94E1-112B61FE78C5}", AZ::Module);
        AZ_CLASS_ALLOCATOR(GradientSignalModule, AZ::SystemAllocator);

        GradientSignalModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
