/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
