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

namespace AZ
{
    class AzCoreModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AzCoreModule, "{898CE9C5-B4CC-4331-811E-3B44B967A1C1}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AzCoreModule, AZ::OSAllocator, 0);

        AzCoreModule();
        ~AzCoreModule() override = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
