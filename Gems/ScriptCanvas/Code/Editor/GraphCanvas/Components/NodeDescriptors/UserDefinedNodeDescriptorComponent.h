/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h"

namespace ScriptCanvasEditor
{
    class UserDefinedNodeDescriptorComponent
        : public NodeDescriptorComponent
    {
    public:
        AZ_COMPONENT(UserDefinedNodeDescriptorComponent, "{24109899-2026-4981-BFA1-38DB134246A2}", NodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);
        
        UserDefinedNodeDescriptorComponent();
        ~UserDefinedNodeDescriptorComponent() = default;
    };
}
