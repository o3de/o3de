/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <StarsModuleInterface.h>
#include <StarsSystemComponent.h>

namespace AZ::Render
{
    class StarsModule
        : public StarsModuleInterface
    {
    public:
        AZ_RTTI(StarsModule, "{962F1F0F-AEEB-4E98-87A0-E7E0403D37AC}", StarsModuleInterface);
        AZ_CLASS_ALLOCATOR(StarsModule, AZ::SystemAllocator, 0);
    };
}// namespace AZ::Render

AZ_DECLARE_MODULE_CLASS(Gem_Stars, AZ::Render::StarsModule)
