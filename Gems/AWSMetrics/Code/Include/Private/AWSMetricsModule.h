/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include <AzCore/Module/Module.h>

namespace AWSMetrics
{
    class AWSMetricsModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AWSMetricsModule, "{A36566F3-E144-4188-A7E0-BAB45BCEA55F}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AWSMetricsModule, AZ::SystemAllocator, 0);

        AWSMetricsModule();
        ~AWSMetricsModule() override = default;
        /**
         * Add required SystemComponents to the SystemEntity.
         */
        virtual AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}

