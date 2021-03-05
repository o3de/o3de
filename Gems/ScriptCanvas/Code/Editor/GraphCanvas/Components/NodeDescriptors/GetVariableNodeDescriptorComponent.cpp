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
#include "precompiled.h"

#include <AzCore/Component/ComponentApplicationBus.h>

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>

#include <Editor/GraphCanvas/Components/NodeDescriptors/GetVariableNodeDescriptorComponent.h>
#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/View/Widgets/PropertyGridBus.h>


#include <ScriptCanvas/Libraries/Core/GetVariable.h>

namespace ScriptCanvasEditor
{
    ///////////////////////////////////////
    // GetVariableNodeDescriptorComponent
    ///////////////////////////////////////

    void GetVariableNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<GetVariableNodeDescriptorComponent, VariableNodeDescriptorComponent>()
                ->Version(1)
                ;
        }
    }

    GetVariableNodeDescriptorComponent::GetVariableNodeDescriptorComponent()
        : VariableNodeDescriptorComponent(NodeDescriptorType::GetVariable)
    {

    }

    void GetVariableNodeDescriptorComponent::UpdateTitle(AZStd::string_view variableName)
    {
        AZStd::string titleName = AZStd::string::format("Get %s", variableName.data());

        GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetTitle, titleName);
    }    
}