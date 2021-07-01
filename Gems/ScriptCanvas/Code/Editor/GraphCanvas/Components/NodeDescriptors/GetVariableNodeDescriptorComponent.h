/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
