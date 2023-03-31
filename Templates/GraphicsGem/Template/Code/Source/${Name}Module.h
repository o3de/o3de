/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

namespace AZ::Render
{
    class ${Name}Module
        : public AZ::Module
    {
    public:
        AZ_RTTI(${Name}Module, "{${Random_Uuid}}", AZ::Module);
        AZ_CLASS_ALLOCATOR(${Name}Module, AZ::SystemAllocator);

        ${Name}Module();

        //! Add required SystemComponents to the SystemEntity.
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}

AZ_DECLARE_MODULE_CLASS(Gem_${Name}, AZ::Render::${Name}Module)
