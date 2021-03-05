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
    //! Entry point for Material Editor Document library. This module is responsible for registering dependencies and logic needed
    //! for the Material Document API
    class MaterialDocumentModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(MaterialDocumentModule, "{81D7A170-9284-4DE9-8D92-B6B94E8A2BDF}", AZ::Module);
        AZ_CLASS_ALLOCATOR(MaterialDocumentModule, AZ::SystemAllocator, 0);

        MaterialDocumentModule();

        //! Add required SystemComponents to the SystemEntity.
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
