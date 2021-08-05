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
    //! Entry point for Shader Management Console Window library.
    class ShaderManagementConsoleWindowModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(ShaderManagementConsoleWindowModule, "{57D6239C-AE03-4ED8-9125-35C5B1625503}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ShaderManagementConsoleWindowModule, AZ::SystemAllocator, 0);

        ShaderManagementConsoleWindowModule();

        //! Add required SystemComponents to the SystemEntity.
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
