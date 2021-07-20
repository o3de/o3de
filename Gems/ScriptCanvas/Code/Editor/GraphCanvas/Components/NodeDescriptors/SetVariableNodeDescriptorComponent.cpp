/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <AzCore/Component/ComponentApplicationBus.h>

#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/SetVariableNodeDescriptorComponent.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/View/Widgets/PropertyGridBus.h>

#include <ScriptCanvas/Libraries/Core/SetVariable.h>
#include <ScriptCanvas/Variable/VariableData.h>

namespace ScriptCanvasEditor
{
    ///////////////////////////////////////
    // SetVariableNodeDescriptorComponent
    ///////////////////////////////////////

    void SetVariableNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<SetVariableNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(1)
                ;
        }
    }

    SetVariableNodeDescriptorComponent::SetVariableNodeDescriptorComponent()
        : VariableNodeDescriptorComponent(NodeDescriptorType::SetVariable)
    {

    }

    void SetVariableNodeDescriptorComponent::UpdateTitle(AZStd::string_view variableName)
    {
        AZStd::string titleName = AZStd::string::format("Set %s", variableName.data());

        GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetTitle, titleName);
    }
}
