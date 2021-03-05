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

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/VariableNodeDescriptorComponent.h>

#include <ScriptCanvas/Variable/VariableBus.h>

namespace ScriptCanvasEditor
{
    class GetVariableNodeDescriptorComponent
        : public VariableNodeDescriptorComponent
    {
    public:
        AZ_COMPONENT(GetVariableNodeDescriptorComponent, "{78D946A9-4CC6-4BA7-A46A-A4C87191678D}", VariableNodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);

        GetVariableNodeDescriptorComponent();

    protected:
        void UpdateTitle(AZStd::string_view variableName) override;
    };
}