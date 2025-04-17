/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Memory/Memory.h>

namespace ScriptCanvas::Developer
{
    class ScriptCanvasDeveloperModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(ScriptCanvasDeveloperModule, "{31E2A550-3940-4BDD-9764-EEDB9CC5B876}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ScriptCanvasDeveloperModule, AZ::SystemAllocator);

        ScriptCanvasDeveloperModule();
        ~ScriptCanvasDeveloperModule() override;
        
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;

    };
}
