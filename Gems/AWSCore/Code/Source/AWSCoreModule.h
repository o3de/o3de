/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Module.h>

namespace AWSCore
{
    class AWSCoreModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AWSCoreModule, "{291657AE-6725-4D41-9EEF-5E6096B79600}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AWSCoreModule, AZ::SystemAllocator, 0);

        AWSCoreModule();
        ~AWSCoreModule() override = default;
        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}

