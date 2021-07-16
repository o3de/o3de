/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    class SetVariableNodeDescriptorComponent
        : public VariableNodeDescriptorComponent
    {
    public:
        AZ_COMPONENT(SetVariableNodeDescriptorComponent, "{5C1183AC-09E9-4D43-A6F4-76B4F3EE18ED}", VariableNodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);

        SetVariableNodeDescriptorComponent();

    protected:
        void UpdateTitle(AZStd::string_view variableName) override;
    };
}
