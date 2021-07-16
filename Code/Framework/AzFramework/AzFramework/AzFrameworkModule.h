/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Module/Module.h>

namespace AzFramework
{
    class AzFrameworkModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AzFrameworkModule, "{FC9FEAC4-ADF5-426B-B26D-96A3413F3AF2}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AzFrameworkModule, AZ::OSAllocator, 0);

        AzFrameworkModule();
        ~AzFrameworkModule() override = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
