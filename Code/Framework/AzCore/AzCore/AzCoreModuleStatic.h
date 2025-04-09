/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Module/Module.h>
namespace AZ
{
    class AzCoreModuleStatic
        : public AZ::Module
    {
    public:
        AZ_RTTI(AzCoreModuleStatic, "{95A21E30-2806-40B0-AF2F-36C0E3EE8631}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AzCoreModuleStatic, AZ::OSAllocator);

        AzCoreModuleStatic();
        ~AzCoreModuleStatic() override = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
