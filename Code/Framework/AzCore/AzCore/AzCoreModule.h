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
    class AzCoreModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AzCoreModule, "{898CE9C5-B4CC-4331-811E-3B44B967A1C1}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AzCoreModule, AZ::OSAllocator);

        AzCoreModule();
        ~AzCoreModule() override = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
