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
