/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Module.h>

namespace AzNetworking
{
    class AzNetworkingModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AzNetworkingModule, "{4118D37D-233D-4CD5-ACE7-747FBAF2615D}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AzNetworkingModule, AZ::OSAllocator);

        AzNetworkingModule();
        ~AzNetworkingModule() override = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
