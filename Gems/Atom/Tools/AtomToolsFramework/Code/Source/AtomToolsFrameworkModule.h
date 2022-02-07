/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Module.h>
#include <AzToolsFramework/API/PythonLoader.h>

namespace AtomToolsFramework
{
    class AtomToolsFrameworkModule
        : public AZ::Module
        , public AzToolsFramework::EmbeddedPython::PythonLoader
    {
    public:
        AZ_RTTI(AtomToolsFrameworkModule, "{B58B7CA8-98C9-4DC8-8607-E094989BBBE2}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AtomToolsFrameworkModule, AZ::SystemAllocator, 0);

        AtomToolsFrameworkModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
