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