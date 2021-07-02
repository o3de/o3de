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
