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

#include <AzCore/Module/Module.h>

namespace AzNetworking
{
    class AzNetworkingModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AzNetworkingModule, "{4118D37D-233D-4CD5-ACE7-747FBAF2615D}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AzNetworkingModule, AZ::OSAllocator, 0);

        AzNetworkingModule();
        ~AzNetworkingModule() override = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
