/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
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
        PropertyEditorToolsSystemInterface::RegisterHandler<GenericButtonHandler>();
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

        auto typeIdAttribute = node.FindMember(PropertyEditor::ValueType.GetName());
        AZ::TypeId typeId = AZ::TypeId::CreateNull();
        if (typeIdAttribute != node.MemberEnd())
        {
            typeId = AZ::Dom::Utils::DomValueToTypeId(typeIdAttribute->second);
        }
        else
        {
            AZ::Dom::Value value = PropertyEditor::Value.ExtractFromDomNode(node).value_or(AZ::Dom::Value());
            // If the object is a pointer object extract the TypeId from it
            if (auto pointerObject = AZ::Dom::Utils::ValueToType<AZ::PointerObject>(value);
                pointerObject && pointerObject->IsValid())
            {
                typeId = pointerObject->m_typeId;
            }
            else
            {
                typeId = AZ::Dom::Utils::GetValueTypeId(value);
            }
        }

        AZStd::string_view typeName = PropertyEditor::Type.ExtractFromDomNode(node).value_or("");
        return GetPropertyHandlerForType(typeName, typeId);
    }

    PropertyEditorToolsSystem::PropertyHandlerId PropertyEditorToolsSystem::GetPropertyHandlerForType(AZStd::string_view handlerName, const AZ::TypeId& typeId)
    {
        // If the Type is empty or unspecified, check the default handler list
        if (handlerName.empty())
        {
            for (PropertyHandlerId handler : m_defaultHandlers)
            {
                if (handler->m_shouldHandleType(typeId))
                {
                    return handler;
                }
            }
        }
        // Otherwise search all registered handlers by name
        else if (auto handlerBucketIt = m_registeredHandlers.find(AZ::Name(handlerName)); handlerBucketIt != m_registeredHandlers.end())
        {
            for (auto handlerIt = handlerBucketIt->second.begin(); handlerIt != handlerBucketIt->second.end(); ++handlerIt)
            {
                if (handlerIt->m_shouldHandleType(typeId))
                {
                    return &(*handlerIt);
                }
            }
        }

        // If we still couldn't find the handler, check if there is any generic class info for this type.
        // This might happen if there is a single generic handler for multiple derived types, such is the
        // case for asset property handlers that have a generic handler for AZ::Data::Asset<AZ::Data::AssetData>,
        // where the asset type is a derived class of AZ::Data::AssetData.
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (const auto* genericInfo = serializeContext->FindGenericClassInfo(typeId))
        {
            if (genericInfo->GetGenericTypeId() != typeId)
            {
                auto genericTypeId = genericInfo->GetGenericTypeId();
                return GetPropertyHandlerForType(handlerName, genericTypeId);
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
