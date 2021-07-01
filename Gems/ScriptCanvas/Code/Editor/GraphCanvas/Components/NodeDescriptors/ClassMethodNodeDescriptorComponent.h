/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h"

namespace ScriptCanvasEditor
{
    class ClassMethodNodeDescriptorComponent
        : public NodeDescriptorComponent
    {
    public:
        AZ_COMPONENT(ClassMethodNodeDescriptorComponent, "{853F137F-C2E8-4D7D-8A1C-D99D167BD569}", NodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);

        ClassMethodNodeDescriptorComponent();
        ~ClassMethodNodeDescriptorComponent() = default;
    };
}
