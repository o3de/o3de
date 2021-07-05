/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Module.h>

namespace MaterialEditor
{
    //! Entry point for Material Editor Viewport library. This module is responsible for registering and reflecting dependencies
    class MaterialViewportModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(MaterialViewportModule, "{D4A53226-D31A-4FBD-A599-F17F740EC2BA}", AZ::Module);
        AZ_CLASS_ALLOCATOR(MaterialViewportModule, AZ::SystemAllocator, 0);

        MaterialViewportModule();

        //! Add required SystemComponents to the SystemEntity.
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
