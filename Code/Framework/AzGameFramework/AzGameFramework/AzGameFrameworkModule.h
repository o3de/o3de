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

namespace AzGameFramework
{
    class AzGameFrameworkModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AzGameFrameworkModule, "{FC9FEAC4-ADF5-426B-B26D-96A3413F3AF2}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AzGameFrameworkModule, AZ::OSAllocator, 0);

        AzGameFrameworkModule();
        ~AzGameFrameworkModule() override = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}