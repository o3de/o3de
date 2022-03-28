/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
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

