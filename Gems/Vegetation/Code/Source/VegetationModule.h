/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Module.h>

namespace Vegetation
{
    class VegetationModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(VegetationModule, "{AEA5121D-425F-4460-8C0F-02AA69D6B480}", AZ::Module);
        AZ_CLASS_ALLOCATOR(VegetationModule, AZ::SystemAllocator);

        VegetationModule();

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
