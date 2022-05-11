/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyEditorToolsSystem.h>

namespace AzToolsFramework
{
    PropertyEditorToolsSystem::PropertyEditorToolsSystem()
    {
        AZ::Interface<PropertyEditorToolsSystemInterface>::Register(this);
    }

    PropertyEditorToolsSystem::~PropertyEditorToolsSystem()
    {
        AZ::Interface<PropertyEditorToolsSystemInterface>::Unregister(this);
    }

    PropertyEditorToolsSystem::PropertyHandlerId PropertyEditorToolsSystem::GetPropertyHandlerForNode(const AZ::Dom::Value node)
    {
        using AZ::DocumentPropertyEditor::GetNodeName;
        using AZ::DocumentPropertyEditor::Nodes::PropertyEditor;

        if (!node.IsNode() || node.GetNodeName() != GetNodeName<PropertyEditor>())
        {
            AZ_Assert(false, "Attempted to look up a property handler for a value that is not a PropertyEditor node");
            return InvalidHandlerId;
        }

        AZStd::string_view typeName = PropertyEditor::Type.ExtractFromDomNode(node).value_or("");
        if (typeName.empty())
        {
            return InvalidHandlerId;
        }

        auto handlerBucketIt = m_registeredHandlers.find(AZ::Name(typeName));
        if (handlerBucketIt == m_registeredHandlers.end())
        {
            return InvalidHandlerId;
        }

        for (auto handlerIt = handlerBucketIt->second.begin(); handlerIt != handlerBucketIt->second.end(); ++handlerIt)
        {
            if (handlerIt->get()->m_shouldHandleNode(node))
            {
                return handlerIt->get();
            }
        }

        return InvalidHandlerId;
    }

    PropertyEditorToolsSystem::PropertyHandlerInstance PropertyEditorToolsSystem::CreateHandlerInstance(PropertyHandlerId handlerId)
    {
        if (handlerId == InvalidHandlerId)
        {
            AZ_Assert(false, "Attempted to instantiate an invalid property handler");
            return {};
        }

        return handlerId->m_factory();
    }

    void PropertyEditorToolsSystem::RegisterHandler(HandlerData handlerData)
    {
        // Get or create a bucket for handlers with this handler's name
        // Insert a heap allocated holder for the data (heap allocated so that its address can be used as a persistent HandlerId)
        m_registeredHandlers[AZ::Name(handlerData.m_name)].push_back(AZStd::make_unique<HandlerData>(AZStd::move(handlerData)));
    }
} // namespace AzToolsFramework
