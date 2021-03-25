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

#include <AzCore/Serialization/SerializeContext.h>
#include <Editor/GraphCanvas/Components/NodeDescriptors/AzEventHandlerNodeDescriptorComponent.h>

namespace ScriptCanvasEditor
{
    void AzEventHandlerNodeDescriptorComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<AzEventHandlerNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(0)
                ->Field("EventName", &AzEventHandlerNodeDescriptorComponent::m_eventName)
                ;
        }
    }

    AzEventHandlerNodeDescriptorComponent::AzEventHandlerNodeDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::AzEventHandler)
    {
    }

    AzEventHandlerNodeDescriptorComponent::AzEventHandlerNodeDescriptorComponent(AZStd::string eventName)
        : NodeDescriptorComponent(NodeDescriptorType::AzEventHandler)
        , m_eventName{ eventName }
    {
    }

    const AZStd::string& AzEventHandlerNodeDescriptorComponent::GetEventName() const
    {
        return m_eventName;
    }
}
