/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Module.h>

namespace FastNoiseGem
{
    class FastNoiseModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(FastNoiseModule, "{D2E0B087-0033-4D23-8985-C2FD46BDE080}", AZ::Module);
        AZ_CLASS_ALLOCATOR(FastNoiseModule, AZ::SystemAllocator);

        FastNoiseModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
