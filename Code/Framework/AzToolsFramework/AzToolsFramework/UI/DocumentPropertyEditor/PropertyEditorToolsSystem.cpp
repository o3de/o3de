/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyEditorToolsSystem.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/ContainerActionButtonHandler.h>

namespace AzToolsFramework
{
    PropertyEditorToolsSystem::PropertyEditorToolsSystem()
    {
        AZ::Interface<PropertyEditorToolsSystemInterface>::Register(this);

        RegisterDefaultHandlers();
    }

    PropertyEditorToolsSystem::~PropertyEditorToolsSystem()
    {
        AZ::Interface<PropertyEditorToolsSystemInterface>::Unregister(this);
    }

    void PropertyEditorToolsSystem::RegisterDefaultHandlers()
    {
        PropertyEditorToolsSystemInterface::RegisterHandler<ContainerActionButtonHandler>();
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

        // If the Type is empty or unspecified, check the default handler list
        AZStd::string_view typeName = PropertyEditor::Type.ExtractFromDomNode(node).value_or("");
        if (typeName.empty())
        {
            for (PropertyHandlerId handler : m_defaultHandlers)
            {
                if (handler->m_shouldHandleNode(node))
                {
                    return handler;
                }
            }
            return InvalidHandlerId;
        }

        auto handlerBucketIt = m_registeredHandlers.find(AZ::Name(typeName));
        if (handlerBucketIt == m_registeredHandlers.end())
        {
            return InvalidHandlerId;
        }

        for (auto handlerIt = handlerBucketIt->second.begin(); handlerIt != handlerBucketIt->second.end(); ++handlerIt)
        {
            if (handlerIt->m_shouldHandleNode(node))
            {
                return &(*handlerIt);
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

    PropertyEditorToolsSystem::PropertyHandlerId PropertyEditorToolsSystem::RegisterHandler(HandlerData handlerData)
    {
        // Get or create a bucket for this handler's name
        auto& handlerBucket = m_registeredHandlers[AZ::Name(handlerData.m_name)];
        // Add this handler to the bucket
        m_registeredHandlers[AZ::Name(handlerData.m_name)].emplace_back(AZStd::move(handlerData));
        // Retrieve a reference to the inserted entry to serve as our persistent handler ID.
        PropertyHandlerId id = &handlerBucket.back();
        if (id->m_isDefaultHandler)
        {
            m_defaultHandlers.push_back(id);
        }
        return id;
    }

    void PropertyEditorToolsSystem::UnregisterHandler(PropertyHandlerId handlerId)
    {
        if (handlerId == InvalidHandlerId)
        {
            AZ_Assert(false, "Attempted to unregister an invalid handler ID");
            return;
        }

        // Clear default handlers from the default handler list
        if (handlerId->m_isDefaultHandler)
        {
            m_defaultHandlers.erase(AZStd::remove(m_defaultHandlers.begin(), m_defaultHandlers.end(), handlerId));
        }

        auto handlerBucketIt = m_registeredHandlers.find(AZ::Name(handlerId->m_name));
        if (handlerBucketIt == m_registeredHandlers.end())
        {
            AZ_Warning("DPE", false, "UnregisterHandler: the specified handler was not found");
        }

        auto& handlerBucket = handlerBucketIt->second;
        for (auto handlerIt = handlerBucket.begin(); handlerIt != handlerBucket.end(); ++handlerIt)
        {
            if (&(*handlerIt) == handlerId)
            {
                handlerBucket.erase(handlerIt);
                return;
            }
        }

        AZ_Warning("DPE", false, "UnregisterHandler: the specified handler was not found");
    }
} // namespace AzToolsFramework
