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
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Memory/Memory.h>

namespace ScriptCanvas
{
    class ScriptCanvasModuleCommon : public AZ::Module
    {
    public:
        AZ_RTTI(ScriptCanvasModuleCommon, "{FDEA6784-C26A-435A-8A07-D8BCE87B13B0}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ScriptCanvasModuleCommon, AZ::SystemAllocator);

        ScriptCanvasModuleCommon();
        ~ScriptCanvasModuleCommon();

    protected:
        AZ::ComponentTypeList GetCommonSystemComponents() const;
    };

    
    //! The ScriptCanvas::Module class coordinates with the application to reflect classes and create system components.
    class ScriptCanvasModule : public ScriptCanvasModuleCommon
    {
    public:
        
        AZ_RTTI(ScriptCanvasModule, "{098376F1-D0C2-4B92-9FCF-AFC8C7DFA172}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ScriptCanvasModule, AZ::SystemAllocator);

        ScriptCanvasModule();
        
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;

    };

} 
