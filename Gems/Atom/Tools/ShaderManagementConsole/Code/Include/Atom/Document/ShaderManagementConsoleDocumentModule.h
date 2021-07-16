/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Module.h>

namespace ShaderManagementConsole
{
    //! Entry point for Shader Management Console Document library.
    class ShaderManagementConsoleDocumentModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(ShaderManagementConsoleDocumentModule, "{81D7A170-9284-4DE9-8D92-B6B94E8A2BDF}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ShaderManagementConsoleDocumentModule, AZ::SystemAllocator, 0);

        ShaderManagementConsoleDocumentModule();

        //! Add required SystemComponents to the SystemEntity.
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
