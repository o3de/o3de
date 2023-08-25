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
    class AzFrameworkNativeUIModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AzFrameworkNativeUIModule, "{49E437E9-36D8-4FBC-829E-CBFE760B71C2}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AzFrameworkNativeUIModule, AZ::OSAllocator);

        AzFrameworkNativeUIModule();
        ~AzFrameworkNativeUIModule() override = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
