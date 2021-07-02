/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
