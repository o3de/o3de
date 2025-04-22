/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/PropertyTreeEditor/PropertyTreeEditor.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzCore/Asset/AssetManagerBus.h>

namespace AzToolsFramework
{
    // PropertyTreeEditor::PropertyTreeEditorNode

    PropertyTreeEditor::PropertyTreeEditorNode::PropertyTreeEditorNode(const AZ::TypeId& typeId, InstanceDataNode* nodePtr, AZStd::optional<AZStd::string> newName)
        : m_typeId(typeId)
        , m_nodePtr(nodePtr)
        , m_newName(newName)
    {
    }

    PropertyTreeEditor::PropertyTreeEditorNode::PropertyTreeEditorNode(InstanceDataNode* nodePtr, AZStd::optional<AZStd::string> newName)
        : m_nodePtr(nodePtr)
        , m_newName(newName)
    {
    }

    PropertyTreeEditor::PropertyTreeEditorNode::PropertyTreeEditorNode(InstanceDataNode* nodePtr, AZStd::optional<AZStd::string> newName,
        const AZStd::string& parentPath, const AZStd::vector<ChangeNotification>& notifiers)
        : m_nodePtr(nodePtr)
        , m_newName(newName)
        , m_parentPath(parentPath)
        , m_notifiers(notifiers)
    {
    }

    AZStd::optional<AZ::Attribute*> PropertyTreeEditor::PropertyTreeEditorNode::FindAttribute(AZ::Edit::AttributeId nameCrc) const
    {
        AZ_Error("Editor", m_nodePtr, "Null m_nodePtr has no attributes.");
        if (m_nodePtr)
        {
            AZ::Attribute* attribute = m_nodePtr->FindAttribute(nameCrc);
            if (attribute)
            {
                return { attribute };
            }
        }
        return AZStd::nullopt;
    }

    // PropertyTreeEditor

    PropertyTreeEditor::PropertyTreeEditor(void* pointer, AZ::TypeId typeId)
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        AZ_Error("Editor", m_serializeContext, "Serialize context not available");
        if (!m_serializeContext)
        {
            return;
        }

        m_instanceDataHierarchy = AZStd::shared_ptr<InstanceDataHierarchy>(aznew InstanceDataHierarchy());
        m_instanceDataHierarchy->AddRootInstance(pointer, typeId);
        m_instanceDataHierarchy->Build(m_serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

        PopulateNodeMap(m_instanceDataHierarchy->GetChildren());
    }

    void PropertyTreeEditor::SetVisibleEnforcement(bool enforceVisiblity)
    {
        if (m_enforceVisiblity != enforceVisiblity)
        {
            m_enforceVisiblity = enforceVisiblity;

            // rebuild the nodes enforcing using the ShowChildrenOnly flag
            m_nodeMap.clear();
            PopulateNodeMap(m_instanceDataHierarchy->GetChildren());
        }
    }

    const AZStd::vector<AZStd::string> PropertyTreeEditor::BuildPathsListWithTypes()
    {
        AZStd::vector<AZStd::string> pathsList;
        AZStd::string buffer;

        for (auto&& node : m_nodeMap)
        {
            InstanceDataNode* nodePtr = node.second.m_nodePtr;

            // include the 'type name' and common Editor property attributes to the descriptions
            buffer = node.first + " (";
            buffer += nodePtr->GetClassMetadata()->m_name;
            buffer += ",";

            NodeDisplayVisibility visibility = CalculateNodeDisplayVisibility(*nodePtr);
            if (visibility == NodeDisplayVisibility::NotVisible)
            {
                buffer += "NotVisible";
            }
            else if (visibility == NodeDisplayVisibility::Visible)
            {
                buffer += "Visible";
            }
            else if (visibility == NodeDisplayVisibility::ShowChildrenOnly)
            {
                buffer += "ShowChildrenOnly";
            }
            else if (visibility == NodeDisplayVisibility::HideChildren)
            {
                buffer += "HideChildren";
            }

            if (AZ::Attribute* attribute = nodePtr->FindAttribute(AZ::Edit::Attributes::ReadOnly))
            {
                PropertyAttributeReader reader(static_cast<AZ::Component*>(nodePtr->FirstInstance()), attribute);
                bool readOnlyValue = false;
                reader.Read<bool>(readOnlyValue);
                if (readOnlyValue)
                {
                    buffer += ",ReadOnly";
                }
            }
            buffer += ")";

            pathsList.push_back(buffer);
        }

        return pathsList;
    }

    const AZStd::vector<AZStd::string> PropertyTreeEditor::BuildPathsList() const
    {
        AZStd::vector<AZStd::string> pathsList;

        for (auto node : m_nodeMap)
        {
            pathsList.push_back(node.first);
        }

        return pathsList;
    }

    AZStd::optional<PropertyTreeEditor::PropertyTreeEditorNode*> PropertyTreeEditor::FetchPropertyTreeEditorNode(AZStd::string_view propertyPath)
    {
        auto&& propertyTreeEditorNodeIterator = m_nodeMap.find(propertyPath);
        if (propertyTreeEditorNodeIterator == m_nodeMap.end())
        {
            AZ_Warning("PropertyTreeEditor", false, "Property path provided [ %.*s ] was not found in tree.", aznumeric_cast<int>(propertyPath.size()), propertyPath.data());
            return AZStd::nullopt;
        }
        if (m_enforceVisiblity && IsPropertyTreeEditorNodeHidden(propertyTreeEditorNodeIterator->second))
        {
            AZ_TracePrintf("PropertyTreeEditor", "Property path provided [ %.*s ] is hidden.", aznumeric_cast<int>(propertyPath.size()), propertyPath.data());
            return AZStd::nullopt;
        }
        return AZStd::make_optional(&propertyTreeEditorNodeIterator->second);
    }

    AZStd::string PropertyTreeEditor::GetPropertyType(const AZStd::string_view propertyPath)
    {
        if (m_nodeMap.find(propertyPath) == m_nodeMap.end())
        {
            AZ_Warning("PropertyTreeEditor", false, "GetPropertyType - path provided was not found in tree.");
            return AZStd::string("");
        }

        const PropertyTreeEditorNode& pteNode = m_nodeMap[propertyPath];

        // Notify the user that they should not use the deprecated name any more, and what it has been replaced with.
        AZ_Warning("PropertyTreeEditor", !pteNode.m_newName, "GetPropertyType - This path is deprecated; property name has been changed to %s.", pteNode.m_newName.value().c_str());

        return AZStd::string(pteNode.m_nodePtr->GetClassMetadata()->m_name);
    }

    PropertyTreeEditor::PropertyAccessOutcome PropertyTreeEditor::GetProperty(const AZStd::string_view propertyPath)
    {
        auto&& propertyNodeHandle = FetchPropertyTreeEditorNode(propertyPath);
        if (!propertyNodeHandle)
        {
            return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("GetProperty - path provided was not found in tree.")};
        }

        const PropertyTreeEditorNode& pteNode = *propertyNodeHandle.value();

        // Notify the user that they should not use the deprecated name any more, and what it has been replaced with.
        AZ_Warning("PropertyTreeEditor", !pteNode.m_newName, "GetProperty - This path [ %.*s ] is deprecated; property name has been changed to %s.", aznumeric_cast<int>(propertyPath.size()), propertyPath.data(), pteNode.m_newName.value().c_str());

        void* nodeData = nullptr;
        const AZ::TypeId& underlyingTypeId = AZ::Internal::GetUnderlyingTypeId(*pteNode.m_nodePtr->GetClassMetadata()->m_azRtti);
        AZ::TypeId typeId = pteNode.m_nodePtr->GetClassMetadata()->m_typeId;

        // If the type can be downcasted to its underlying type, we return the value in the latter.
        // For example, this allows to get the value of enums defined with AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE.
        if (!underlyingTypeId.IsNull() && m_serializeContext->CanDowncast(typeId, underlyingTypeId))
        {
            typeId = underlyingTypeId;
        }

        if (!pteNode.m_nodePtr->ReadRaw(nodeData, typeId))
        {
            AZ_Warning("PropertyTreeEditor", false, "GetProperty - path provided [ %.*s ] was found, but read operation failed.", aznumeric_cast<int>(propertyPath.size()), propertyPath.data());
            return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("GetProperty - path provided was found, but read operation failed.")};
        }

        if (pteNode.m_nodePtr->GetClassMetadata()->m_azRtti && pteNode.m_nodePtr->GetClassMetadata()->m_azRtti->IsTypeOf(azrtti_typeid<AzFramework::SimpleAssetReferenceBase>()))
        {
            // Handle SimpleAssetReference (it should return an AssetId)
            AzFramework::SimpleAssetReferenceBase* instancePtr = reinterpret_cast<AzFramework::SimpleAssetReferenceBase*>(nodeData);

            AZ::Data::AssetId result = AZ::Data::AssetId();
            AZStd::string assetPath = instancePtr->GetAssetPath();

            if (!assetPath.empty())
            {
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(result, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, assetPath.c_str(), AZ::Uuid(), false);
            }

            return {PropertyAccessOutcome::ValueType(result)};
        }
        else if (pteNode.m_nodePtr->GetClassMetadata()->m_azRtti && pteNode.m_nodePtr->GetClassMetadata()->m_azRtti->GetGenericTypeId() == azrtti_typeid<AZ::Data::Asset>())
        {
            // Handle Asset<> (it should return an AssetId)
            AZ::Data::Asset<AZ::Data::AssetData>* instancePtr = reinterpret_cast<AZ::Data::Asset<AZ::Data::AssetData>*>(nodeData);
            return {PropertyAccessOutcome::ValueType(instancePtr->GetId())};
        }

        // Default case - just return the value wrapped into an any
        AZStd::any typeInfoHelper = m_serializeContext->CreateAny(typeId);
        return {PropertyAccessOutcome::ValueType(nodeData, typeInfoHelper.get_type_info())};
    }

    bool PropertyTreeEditor::SetSimpleAssetPath(const AZ::Data::AssetId& assetId, const PropertyTreeEditorNode& pteNode)
    {
        // Handle SimpleAssetReference

        bool updated = false;
        size_t numInstances = pteNode.m_nodePtr->GetNumInstances();

        // Get Asset Id from any
        AZStd::string assetPath;
        if (assetId.IsValid())
        {
            // Get Asset Path from Asset Id
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, assetId);
        }

        for (size_t idx = 0; idx < numInstances; ++idx)
        {
            AzFramework::SimpleAssetReferenceBase* instancePtr = reinterpret_cast<AzFramework::SimpleAssetReferenceBase*>(pteNode.m_nodePtr->GetInstance(idx));

            // Check if valid assetId was provided
            if (assetId.IsValid())
            {
                // Set Asset Path in Asset Reference
                instancePtr->SetAssetPath(assetPath.c_str());
                updated = true;
            }
            else
            {
                // Clear Asset Path in Asset Reference
                instancePtr->SetAssetPath("");
                updated = true;
            }
        }

        return updated;
    }

    bool PropertyTreeEditor::SetAssetData(const AZ::Data::AssetId& assetId, const PropertyTreeEditorNode& pteNode)
    {
        using namespace AZ::Data;
        bool updated = false;
        size_t numInstances = pteNode.m_nodePtr->GetNumInstances();
        for (size_t idx = 0; idx < numInstances; ++idx)
        {
            Asset<AssetData>* instancePtr = reinterpret_cast<Asset<AssetData>*>(pteNode.m_nodePtr->GetInstance(idx));
            if (assetId.IsValid())
            {
                *instancePtr = AssetManager::Instance().GetAsset(assetId, instancePtr->GetType(), instancePtr->GetAutoLoadBehavior());
                updated = true;
            }
            else
            {
                *instancePtr = Asset<AssetData>(AssetId(), instancePtr->GetType());
                updated = true;
            }
        }
        return updated;
    }

    PropertyTreeEditor::PropertyAccessOutcome PropertyTreeEditor::SetProperty(const AZStd::string_view propertyPath, const AZStd::any& value)
    {
        auto&& propertyNodeHandle = FetchPropertyTreeEditorNode(propertyPath);
        if (!propertyNodeHandle)
        {
            return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("SetProperty - path provided was not found in tree.") };
        }

        const PropertyTreeEditorNode& pteNode = *propertyNodeHandle.value();

        // Notify the user that they should not use the deprecated name any more, and what it has been replaced with.
        AZ_Warning("PropertyTreeEditor", !pteNode.m_newName, "SetProperty - This path [ %.*s ] is deprecated; property name has been changed to %s.", aznumeric_cast<int>(propertyPath.size()), propertyPath.data(), pteNode.m_newName.value().c_str());

        // If the incoming 'value' is an empty any<> then set the node's property to a default value
        if (value.empty())
        {
            auto&& classMetaDataRtti = pteNode.m_nodePtr->GetClassMetadata()->m_azRtti;
            if (classMetaDataRtti && classMetaDataRtti->IsTypeOf(azrtti_typeid<AzFramework::SimpleAssetReferenceBase>()))
            {
                AZ::Data::AssetId emptyAssetId;
                SetSimpleAssetPath(emptyAssetId, pteNode);
            }
            else if (classMetaDataRtti && classMetaDataRtti->GetGenericTypeId() == azrtti_typeid<AZ::Data::Asset>())
            {
                AZ::Data::AssetId emptyAssetId;
                SetAssetData(emptyAssetId, pteNode);
            }
            else
            {
                const AZ::TypeId& defaultTypeId = pteNode.m_nodePtr->GetClassMetadata()->m_typeId;
                AZStd::any defaultValue = m_serializeContext->CreateAny(defaultTypeId);
                pteNode.m_nodePtr->WriteRaw(AZStd::any_cast<void>(&defaultValue), defaultTypeId);
            }

            PropertyNotify(&pteNode);

            return { PropertyAccessOutcome::ValueType(value) };
        }

        // Handle Asset cases differently
        if (value.type() == azrtti_typeid<AZ::Data::AssetId>() && pteNode.m_nodePtr->GetClassMetadata()->m_azRtti && pteNode.m_nodePtr->GetClassMetadata()->m_azRtti->IsTypeOf(azrtti_typeid<AzFramework::SimpleAssetReferenceBase>()))
        {
            // Handle SimpleAssetReference
            if (SetSimpleAssetPath(AZStd::any_cast<AZ::Data::AssetId>(value), pteNode))
            {
                PropertyNotify(&pteNode);
            }

            return {PropertyAccessOutcome::ValueType(value)};
        }
        else if (value.type() == azrtti_typeid<AZ::Data::AssetId>() && pteNode.m_nodePtr->GetClassMetadata()->m_azRtti && pteNode.m_nodePtr->GetClassMetadata()->m_azRtti->GetGenericTypeId() == azrtti_typeid<AZ::Data::Asset>())
        {
            // Handle Asset<>

            // Get Asset Id from any
            AZ::Data::AssetId assetId = AZStd::any_cast<AZ::Data::AssetId>(value);

            if (SetAssetData(assetId, pteNode))
            {
                PropertyNotify(&pteNode);
            }

            return {PropertyAccessOutcome::ValueType(value)};
        }

        // Default handler

        //A temporary conversion buffer in case the src and dst data types are different.
        AZStd::any convertedValue;


        // Check if types match, or convert the value if its type supports it.
        const void* valuePtr = HandleTypeConversion(value.type(), pteNode.m_nodePtr->GetClassMetadata()->m_typeId, AZStd::any_cast<void>(&value), convertedValue);
        if (valuePtr)
        {
            pteNode.m_nodePtr->WriteRaw(valuePtr, pteNode.m_nodePtr->GetClassMetadata()->m_typeId);
            PropertyNotify(&pteNode);
            return {PropertyAccessOutcome::ValueType(value)};
        }

        // If it could not be converted, verify the underlying type.
        const AZ::TypeId& underlyingTypeId = AZ::Internal::GetUnderlyingTypeId(*pteNode.m_nodePtr->GetClassMetadata()->m_azRtti);

        if (!underlyingTypeId.IsNull() && underlyingTypeId != pteNode.m_nodePtr->GetClassMetadata()->m_typeId)
        {
            const void* underlyingValuePtr = HandleTypeConversion(value.type(), underlyingTypeId, AZStd::any_cast<void>(&value), convertedValue);
            if (underlyingValuePtr)
            {
                pteNode.m_nodePtr->WriteRaw(underlyingValuePtr, pteNode.m_nodePtr->GetClassMetadata()->m_typeId);
                PropertyNotify(&pteNode);
                return {PropertyAccessOutcome::ValueType(value)};
            }
        }

        AZ_Warning("PropertyTreeEditor", false, "SetProperty - value type cannot be converted to the property's type.");
        return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("SetProperty - value type cannot be converted to the property's type.") };

    }

    bool PropertyTreeEditor::CompareProperty(const AZStd::string_view propertyPath, const AZStd::any& value)
    {
        auto&& propertyNodeHandle = FetchPropertyTreeEditorNode(propertyPath);
        if (!propertyNodeHandle)
        {
            return false;
        }

        const PropertyTreeEditorNode& pteNode = *propertyNodeHandle.value();

        // Notify the user that they should not use the deprecated name any more, and what it has been replaced with.
        AZ_Warning("PropertyTreeEditor", !pteNode.m_newName, "CompareProperty - This path [ %.*s ] is deprecated; property name has been changed to %s.", aznumeric_cast<int>(propertyPath.size()), propertyPath.data(), pteNode.m_newName.value().c_str());

        void* nodeData = nullptr;
        AZ::TypeId type = pteNode.m_nodePtr->GetClassMetadata()->m_typeId;

        if (!pteNode.m_nodePtr->ReadRaw(nodeData, type))
        {
            AZ_Warning("PropertyTreeEditor", false, "CompareProperty - path provided [ %.*s ] was found, but read operation failed.", aznumeric_cast<int>(propertyPath.size()), propertyPath.data());
            return false;
        }

        if (pteNode.m_nodePtr->GetClassMetadata()->m_azRtti && pteNode.m_nodePtr->GetClassMetadata()->m_azRtti->IsTypeOf(azrtti_typeid<AzFramework::SimpleAssetReferenceBase>()))
        {
            // Handle SimpleAssetReference (it should return an AssetId)
            AzFramework::SimpleAssetReferenceBase* instancePtr = reinterpret_cast<AzFramework::SimpleAssetReferenceBase*>(nodeData);

            AZ::Data::AssetId result = AZ::Data::AssetId();
            AZStd::string assetPath = instancePtr->GetAssetPath();

            if (!assetPath.empty())
            {
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(result, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, assetPath.c_str(), AZ::Uuid(), false);
            }

            return result == AZStd::any_cast<AZ::Data::AssetId>(value);
        }
        else if (pteNode.m_nodePtr->GetClassMetadata()->m_azRtti && pteNode.m_nodePtr->GetClassMetadata()->m_azRtti->GetGenericTypeId() == azrtti_typeid<AZ::Data::Asset>())
        {
            // Handle Asset<> (it should return an AssetId)
            AZ::Data::Asset<AZ::Data::AssetData>* instancePtr = reinterpret_cast<AZ::Data::Asset<AZ::Data::AssetData>*>(nodeData);

            return instancePtr->GetId() == AZStd::any_cast<AZ::Data::AssetId>(value);
        }
        else
        {
            //A temporary conversion buffer in case the src and dst data types are different.
            AZStd::any convertedValue;

            // Check if types match, or convert the value if its type supports it.
            const void* valuePtr = HandleTypeConversion(value.type(), pteNode.m_nodePtr->GetClassMetadata()->m_typeId, AZStd::any_cast<void>(&value), convertedValue);
            if (!valuePtr)
            {
                // If types are different and cannot be converted, bail
                AZ_Warning("PropertyTreeEditor", false, "CompareProperty - value type cannot be converted to the property's type.");
                return false;
            }

            auto& serializerPtr = pteNode.m_nodePtr->GetClassMetadata()->m_serializer;
            return serializerPtr && serializerPtr->CompareValueData(nodeData, valuePtr);
        }
    }

    AZStd::optional<PropertyTreeEditor::ContainerData> PropertyTreeEditor::FetchContainerData(AZStd::string_view propertyPath) const
    {
        auto&& nodeIterator = m_nodeMap.find(propertyPath);
        if (nodeIterator == m_nodeMap.end())
        {
            AZ_Warning("PropertyTreeEditor", false, "FetchDataContainer - path provided [ %.*s ] was not found in tree.", aznumeric_cast<int>(propertyPath.size()), propertyPath.data());
            return {};
        }
        const PropertyTreeEditorNode* pteNode = &(nodeIterator->second);
        if (pteNode->m_nodePtr->GetClassMetadata())
        {
            AZ::SerializeContext::IDataContainer* dataContainer = nullptr;
            dataContainer = pteNode->m_nodePtr->GetClassMetadata()->m_container;
            if (dataContainer)
            {
                const AZ::SerializeContext::ClassElement* valueElement = nullptr;
                auto typeEnumCallback = [&valueElement](const AZ::Uuid&, const AZ::SerializeContext::ClassElement* genericClassElement)
                {
                    AZ_Error("PropertyTreeEditor", !valueElement, "AddContainerItem - container is expected to only have one element type.");
                    valueElement = genericClassElement;
                    return true;
                };
                dataContainer->EnumTypes(typeEnumCallback);

                AZ_Warning("PropertyTreeEditor", valueElement, "FetchDataContainer - path provided [ %.*s ] does not have a valid valueElement", aznumeric_cast<int>(propertyPath.size()), propertyPath.data());
                if (valueElement)
                {
                    return { ContainerData{ pteNode, dataContainer, valueElement } };
                }
            }
            else
            {
                AZ_Warning("PropertyTreeEditor", false, "FetchDataContainer - path provided [ %.*s ] is not a container.", aznumeric_cast<int>(propertyPath.size()), propertyPath.data());
            }
        }
        else
        {
            AZ_Warning("PropertyTreeEditor", false, "FetchDataContainer - path provided [ %.*s ] had no class metadata", aznumeric_cast<int>(propertyPath.size()), propertyPath.data());
        }
        return {};
    }

    AZStd::optional<PropertyTreeEditor::AssociatePairInfo> PropertyTreeEditor::MakeAssociatePair(const AZ::SerializeContext::ClassElement* containerElement) const
    {
        const AZ::SerializeContext::ClassData* pairClass = m_serializeContext->FindClassData(containerElement->m_typeId);
        const AZ::SerializeContext::ClassElement* keyElement = nullptr;
        const AZ::SerializeContext::ClassElement* valueElement = nullptr;
        auto keyValueTypeEnumCallback = [&keyElement, &valueElement](const AZ::Uuid&, const AZ::SerializeContext::ClassElement* genericClassElement)
        {
            if (!keyElement)
            {
                keyElement = genericClassElement;
            }
            else if (!valueElement)
            {
                valueElement = genericClassElement;
            }
            else
            {
                AZ_Error("PropertyTreeEditor", !valueElement, "MakeAssociatePair - The pair element in a container can't have more than 2 elements.");
                return false;
            }
            return true;
        };
        pairClass->m_container->EnumTypes(keyValueTypeEnumCallback);
        if (!keyElement || !valueElement)
        {
            AZ_Error("PropertyTreeEditor", false, "MakeAssociatePair - unsupported associative container");
            return {};
        }
        return { PropertyTreeEditor::AssociatePairInfo{pairClass, keyElement, valueElement} };
    }

    bool PropertyTreeEditor::IsContainer(AZStd::string_view propertyPath) const
    {
        return FetchContainerData(propertyPath).has_value();
    }

    PropertyTreeEditor::PropertyAccessOutcome PropertyTreeEditor::GetContainerCount(AZStd::string_view propertyPath) const
    {
        auto&& data = FetchContainerData(propertyPath);
        if (!data)
        {
            return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("GetContainerCount - could not get item count from a non-container") };
        }
        const InstanceDataNode* nodePtr = data.value().m_propertyTreeEditorNode->m_nodePtr;
        const AZ::u64 count = aznumeric_cast<AZ::u64>(data.value().m_dataContainer->Size(nodePtr->FirstInstance()));
        return { PropertyAccessOutcome::ValueType(count) };
    }

    PropertyTreeEditor::PropertyAccessOutcome PropertyTreeEditor::ResetContainer(AZStd::string_view propertyPath)
    {
        auto&& data = FetchContainerData(propertyPath);
        if (!data)
        {
            return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("ResetContainer - could not reset items in a non-container") };
        }
        const InstanceDataNode* nodePtr = data.value().m_propertyTreeEditorNode->m_nodePtr;
        data.value().m_dataContainer->ClearElements(nodePtr->FirstInstance(), m_serializeContext);
        return { PropertyAccessOutcome::ValueType(true) };
    }

    PropertyTreeEditor::PropertyAccessOutcome PropertyTreeEditor::AppendContainerItem(AZStd::string_view propertyPath, const AZStd::any& value)
    {
        auto&& data = FetchContainerData(propertyPath);
        if (!data)
        {
            return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("AppendContainerItem - could not append an item to a non-container") };
        }
        if (data.value().m_dataContainer->GetAssociativeContainerInterface())
        {
            return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("AppendContainerItem - cannot use append an item to an associative container") };
        }
        const AZStd::any nullKey{};
        return AddContainerItem(propertyPath, nullKey, value);
    }

    PropertyTreeEditor::PropertyAccessOutcome PropertyTreeEditor::AddContainerItem(AZStd::string_view propertyPath, const AZStd::any& key, const AZStd::any& value)
    {
        auto&& data = FetchContainerData(propertyPath);
        if (!data)
        {
            return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("AddContainerItem - could not add item to a non-container") };
        }

        const InstanceDataNode* nodePtr = data.value().m_propertyTreeEditorNode->m_nodePtr;
        if (data.value().m_dataContainer->GetAssociativeContainerInterface())
        {
            auto&& keyPairInfo = MakeAssociatePair(data.value().m_valueElement);
            if (!keyPairInfo)
            {
                return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("RemoveContainerItem - invalid key pair for associative container") };
            }
            void* newEntry = data.value().m_dataContainer->ReserveElement(nodePtr->FirstInstance(), data.value().m_valueElement);
            if (newEntry)
            {
                void* keyAddress = keyPairInfo.value().m_pairClass->m_container->GetElementByIndex(newEntry, keyPairInfo.value().m_keyElement, 0);
                void* valueAddress = keyPairInfo.value().m_pairClass->m_container->GetElementByIndex(newEntry, keyPairInfo.value().m_valueElement, 1);
                if (keyAddress && valueAddress)
                {
                    m_serializeContext->CloneObjectInplace(keyAddress, AZStd::any_cast<void>(&key), key.type());
                    m_serializeContext->CloneObjectInplace(valueAddress, AZStd::any_cast<void>(&value), value.type());
                    data.value().m_dataContainer->StoreElement(nodePtr->FirstInstance(), newEntry);
                    return { PropertyAccessOutcome::ValueType(true) };
                }
                else
                {
                    data.value().m_dataContainer->FreeReservedElement(nodePtr->FirstInstance(), newEntry, m_serializeContext);
                }
            }
        }
        else
        {
            void* destination = data.value().m_dataContainer->ReserveElement(nodePtr->FirstInstance(), data.value().m_valueElement);
            if (destination)
            {
                const void* source = AZStd::any_cast<void>(&value);
                m_serializeContext->CloneObjectInplace(destination, source, value.type());
                return { PropertyAccessOutcome::ValueType(true) };
            }
            else
            {
                return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("AddContainerItem - could not allocate via ReserveElement()") };
            }
        }
        return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("AddContainerItem - could not add a value in the container") };
    }

    PropertyTreeEditor::PropertyAccessOutcome PropertyTreeEditor::RemoveContainerItem(AZStd::string_view propertyPath, const AZStd::any& key)
    {
        auto&& data = FetchContainerData(propertyPath);
        if (!data)
        {
            return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("RemoveContainerItem - could not remove item from a non-container") };
        }
        const InstanceDataNode* nodePtr = data.value().m_propertyTreeEditorNode->m_nodePtr;

        if (data.value().m_dataContainer->CanAccessElementsByIndex())
        {
            size_t index = 0;
            if (!AZStd::any_numeric_cast(&key, index))
            {
                AZ_Error("PropertyTreeEditor", false, "RemoveContainerItem - key value type should convert into size_t");
                return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("RemoveContainerItem - key value type should convert into size_t") };
            }
            void* value = data.value().m_dataContainer->GetElementByIndex(nodePtr->FirstInstance(), data.value().m_valueElement, index);
            if (value)
            {
                bool removed = data.value().m_dataContainer->RemoveElement(nodePtr->FirstInstance(), value, m_serializeContext);
                return { PropertyAccessOutcome::ValueType(removed) };
            }
        }
        else if (data.value().m_dataContainer->GetAssociativeContainerInterface())
        {
            auto&& keyPairInfo = MakeAssociatePair(data.value().m_valueElement);
            if (!keyPairInfo)
            {
                return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("RemoveContainerItem - invalid key pair for associative container") };
            }
            auto&& associativeContainer = data.value().m_dataContainer->GetAssociativeContainerInterface();
            void* keyPairValue = associativeContainer->GetElementByKey(nodePtr->FirstInstance(), nodePtr->GetElementMetadata(), AZStd::any_cast<void>(&key));
            if (keyPairValue)
            {
                bool removed = data.value().m_dataContainer->RemoveElement(nodePtr->FirstInstance(), keyPairValue, m_serializeContext);
                return { PropertyAccessOutcome::ValueType(removed) };
            }
        }
        return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("RemoveContainerItem - key did not remove a value") };
    }

    PropertyTreeEditor::PropertyAccessOutcome PropertyTreeEditor::UpdateContainerItem(AZStd::string_view propertyPath, const AZStd::any& key, const AZStd::any& value)
    {
        auto&& data = FetchContainerData(propertyPath);
        if (!data)
        {
            return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("UpdateContainerItem - could not remove item from a non-container") };
        }
        const InstanceDataNode* nodePtr = data.value().m_propertyTreeEditorNode->m_nodePtr;

        if (data.value().m_dataContainer->CanAccessElementsByIndex())
        {
            size_t index = 0;
            if (!AZStd::any_numeric_cast(&key, index))
            {
                AZ_Error("PropertyTreeEditor", false, "UpdateContainerItem - key value type should convert into size_t");
                return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("UpdateContainerItem - key value type should convert into size_t") };
            }
            void* destination = data.value().m_dataContainer->GetElementByIndex(nodePtr->FirstInstance(), data.value().m_valueElement, index);
            if (destination)
            {
                const void* source = AZStd::any_cast<void>(&value);
                m_serializeContext->CloneObjectInplace(destination, source, value.type());
                return { PropertyAccessOutcome::ValueType(true) };
            }
        }
        else if (data.value().m_dataContainer->GetAssociativeContainerInterface())
        {
            auto&& keyPairInfo = MakeAssociatePair(data.value().m_valueElement);
            if (!keyPairInfo)
            {
                return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("UpdateContainerItem - invalid key pair for associative container") };
            }
            auto&& associativeContainer = data.value().m_dataContainer->GetAssociativeContainerInterface();
            void* keyPairValue = associativeContainer->GetElementByKey(nodePtr->FirstInstance(), nodePtr->GetElementMetadata(), AZStd::any_cast<void>(&key));
            if (keyPairValue)
            {
                void* destination = keyPairInfo.value().m_pairClass->m_container->GetElementByIndex(keyPairValue, keyPairInfo.value().m_valueElement, 1);
                const void* source = AZStd::any_cast<void>(&value);
                m_serializeContext->CloneObjectInplace(destination, source, value.type());
                return { PropertyAccessOutcome::ValueType(true) };
            }
        }
        return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("UpdateContainerItem - key did not remove a value") };
    }

    PropertyTreeEditor::PropertyAccessOutcome PropertyTreeEditor::GetContainerItem(AZStd::string_view propertyPath, const AZStd::any& key) const
    {
        auto&& data = FetchContainerData(propertyPath);
        if (!data)
        {
            return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("GetContainerItem - could not get an item from a non-container") };
        }
        const InstanceDataNode* nodePtr = data.value().m_propertyTreeEditorNode->m_nodePtr;

        if (data.value().m_dataContainer->CanAccessElementsByIndex())
        {
            size_t index = 0;
            if (!AZStd::any_numeric_cast(&key, index))
            {
                AZ_Error("PropertyTreeEditor", false, "GetContainerItem - key value type should convert into size_t");
                return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("GetContainerItem - key value type should convert into size_t") };
            }

            void* value = data.value().m_dataContainer->GetElementByIndex(nodePtr->FirstInstance(), data.value().m_valueElement, index);
            if (value)
            {
                // Create temporary one with the type id then we can get its type info to construct a new AZStd::any with new data
                auto tempAny = m_serializeContext->CreateAny(data.value().m_valueElement->m_typeId);
                AZStd::any::type_info typeInfo = tempAny.get_type_info();
                return { PropertyAccessOutcome::ValueType(AZStd::move(AZStd::any(value, typeInfo))) };
            }
        }
        else if (data.value().m_dataContainer->GetAssociativeContainerInterface())
        {
            auto&& keyPairInfo = MakeAssociatePair(data.value().m_valueElement);
            if (!keyPairInfo)
            {
                return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("GetContainerItem - invalid key pair for associative container") };
            }
            auto&& associativeContainer = data.value().m_dataContainer->GetAssociativeContainerInterface();
            void* keyPairValue = associativeContainer->GetElementByKey(nodePtr->FirstInstance(), nodePtr->GetElementMetadata(), AZStd::any_cast<void>(&key));
            if (keyPairValue)
            {
                // Create temporary one with the type id then we can get its type info to construct a new AZStd::any with new data
                auto tempAny = m_serializeContext->CreateAny(keyPairInfo.value().m_valueElement->m_typeId);
                AZStd::any::type_info typeInfo = tempAny.get_type_info();
                void* value = keyPairInfo.value().m_pairClass->m_container->GetElementByIndex(keyPairValue, keyPairInfo.value().m_valueElement, 1);
                return { PropertyAccessOutcome::ValueType(AZStd::move(AZStd::any(value, typeInfo))) };
            }
        }
        return PropertyAccessOutcome{ AZStd::unexpect, PropertyAccessOutcome::ErrorType("GetContainerItem - keyed value could not be returned") };
    }

    bool PropertyTreeEditor::IsPropertyTreeEditorNodeHidden(const PropertyTreeEditorNode& propertyTreeEditorNode) const
    {
        NodeDisplayVisibility visibility = CalculateNodeDisplayVisibility(*propertyTreeEditorNode.m_nodePtr);
        return (visibility == NodeDisplayVisibility::NotVisible ||
                visibility == NodeDisplayVisibility::ShowChildrenOnly);
    }

    bool PropertyTreeEditor::HasAttribute(AZStd::string_view propertyPath, AZStd::string_view attribute) const
    {
        auto&& nodeIterator = m_nodeMap.find(propertyPath);
        if (nodeIterator == m_nodeMap.end())
        {
            AZ_Warning("PropertyTreeEditor", false, "HasAttribute - path provided [ %.*s ] was not found in tree.", aznumeric_cast<int>(propertyPath.size()), propertyPath.data());
            return false;
        }
        const PropertyTreeEditorNode* pteNode = &(nodeIterator->second);
        return pteNode->FindAttribute(AZ::Crc32(attribute)).has_value();
    }

    const void* PropertyTreeEditor::HandleTypeConversion(AZ::TypeId fromType, AZ::TypeId toType, const void* sourceValuePtr, AZStd::any& convertedValue)
    {
        // If types match we don't need to convert.
        if (fromType == toType)
        {
            return sourceValuePtr;
        }

        AZ_Assert(sourceValuePtr, "Invalid pointer to the source value");

        if (fromType == AZ::AzTypeInfo<double>::Uuid())
        {
            double value = *static_cast<const double*>(sourceValuePtr);
            return HandleTypeConversion(value, toType, convertedValue);
        }

        if (fromType == AZ::AzTypeInfo<AZ::s64>::Uuid())
        {
            AZ::s64 value = *static_cast<const AZ::s64*>(sourceValuePtr);
            return HandleTypeConversion(value, toType, convertedValue);
        }

        return nullptr;
    }

    template <typename V>
    const void* PropertyTreeEditor::HandleTypeConversion(V fromValue, AZ::TypeId toType, AZStd::any& convertedValue)
    {
        if (toType == AZ::AzTypeInfo<float>::Uuid())
        {
            convertedValue = aznumeric_cast<float>(fromValue);
            return &convertedValue;
        }

        if (toType == AZ::AzTypeInfo<AZ::u32>::Uuid())
        {
            convertedValue = aznumeric_cast<AZ::u32>(fromValue);
            return &convertedValue;
        }

        if (toType == AZ::AzTypeInfo<AZ::s32>::Uuid())
        {
            convertedValue = aznumeric_cast<AZ::s32>(fromValue);
            return &convertedValue;
        }

        if (toType == AZ::AzTypeInfo<AZ::u16>::Uuid())
        {
            convertedValue = aznumeric_cast<AZ::u16>(fromValue);
            return &convertedValue;
        }

        if (toType == AZ::AzTypeInfo<AZ::s16>::Uuid())
        {
            convertedValue = aznumeric_cast<AZ::s16>(fromValue);
            return &convertedValue;
        }

        if (toType == AZ::AzTypeInfo<AZ::u8>::Uuid())
        {
            convertedValue = aznumeric_cast<AZ::u8>(fromValue);
            return &convertedValue;
        }

        if (toType == AZ::AzTypeInfo<AZ::s8>::Uuid())
        {
            convertedValue = aznumeric_cast<AZ::s8>(fromValue);
            return &convertedValue;
        }
        return nullptr;
    }

    void PropertyTreeEditor::HandleChangeNotifyAttribute(PropertyAttributeReader& reader, InstanceDataNode* node, AZStd::vector<ChangeNotification>& notifiers)
    {
        AZ::Edit::AttributeFunction<void()>* funcVoid = azdynamic_cast<AZ::Edit::AttributeFunction<void()>*>(reader.GetAttribute());
        AZ::Edit::AttributeFunction<AZ::u32()>* funcU32 = azdynamic_cast<AZ::Edit::AttributeFunction<AZ::u32()>*>(reader.GetAttribute());
        AZ::Edit::AttributeFunction<AZ::Crc32()>* funcCrc32 = azdynamic_cast<AZ::Edit::AttributeFunction<AZ::Crc32()>*>(reader.GetAttribute());

        const AZ::Uuid handlerTypeId = funcVoid ?
            funcVoid->GetInstanceType() : (funcU32 ? funcU32->GetInstanceType() :
            (funcCrc32 ? funcCrc32->GetInstanceType() : AZ::Uuid::CreateNull()));
        InstanceDataNode* targetNode = node;

        if (!handlerTypeId.IsNull())
        {
            // Walk up the chain looking for the first correct class type to handle the callback
            while (targetNode)
            {
                if (targetNode->GetClassMetadata())
                {
                    if (targetNode->GetClassMetadata()->m_azRtti)
                    {
                        if (targetNode->GetClassMetadata()->m_azRtti->IsTypeOf(handlerTypeId))
                        {
                            // Instance has RTTI, and derives from type expected by the handler.
                            break;
                        }
                    }
                    else
                    {
                        if (handlerTypeId == targetNode->GetClassMetadata()->m_typeId)
                        {
                            // Instance does not have RTTI, and is the type expected by the handler.
                            break;
                        }
                    }
                }

                targetNode = targetNode->GetParent();
            }
        }

        if (targetNode)
        {
            notifiers.push_back(ChangeNotification(targetNode, reader.GetAttribute()));
        }
    }

    //! Recursive function - explores the whole hierarchy in search of all class properties
    //! and saves them in m_nodeMap along with their pipe-separated path for easier access.
    //!     nodeList        A list of InstanceDataNodes to add to the map.
    //!                     The function will call itself recursively on all node's children
    //!     previousPath    The property path to the current nodeList.
    void PropertyTreeEditor::PopulateNodeMap(AZStd::list<InstanceDataNode>& nodeList, const AZStd::string_view& previousPath)
    {
        for (InstanceDataNode& node : nodeList)
        {
            AZStd::string path = previousPath;
            AZStd::vector<ChangeNotification> changeNotifiers;

            bool addChildren = true;
            auto editMetaData = node.GetElementEditMetadata();

            AZ_Warning("PropertyTreeEditor",
                !editMetaData || editMetaData->m_name,
                "Found ElementData with no name on node \"%s\" with path\"%s\", skipping.",
                node.GetElementMetadata() && node.GetElementMetadata()->m_name ? node.GetElementMetadata()->m_name : "",
                path.c_str());

            if (editMetaData && editMetaData->m_name)
            {
                if (!path.empty())
                {
                    path += '|';
                }

                // If this element is nested under a group, we need to include the group name (which is stored in the description)
                // in the path as well in order for the path to be unique
                auto groupMetaData = node.GetGroupElementMetadata();
                if (groupMetaData && groupMetaData->m_description)
                {
                    path += groupMetaData->m_description;
                    path += '|';
                }

                bool addNodeToMap = true;
                if (m_enforceVisiblity)
                {
                    NodeDisplayVisibility visibility = CalculateNodeDisplayVisibility(node);
                    if (visibility == NodeDisplayVisibility::NotVisible)
                    {
                        // this skips this node and all of its children
                        continue;
                    }
                    else if (visibility == NodeDisplayVisibility::ShowChildrenOnly)
                    {
                        // skip this node, but show all of its children
                        addNodeToMap = false;
                    }
                    else if (visibility == NodeDisplayVisibility::HideChildren)
                    {
                        // show this node, but skip all of its children
                        addChildren = false;
                    }
                }

                // there are nodes that are skipped and only show their children nodes such as ShowChildrenOnly
                if (addNodeToMap)
                {
                    path += editMetaData->m_name;
                }

                if (auto notifyAttribute = editMetaData->FindAttribute(AZ::Edit::Attributes::ChangeNotify))
                {
                    PropertyAttributeReader reader(node.FirstInstance(), notifyAttribute);
                    HandleChangeNotifyAttribute(reader, &node, changeNotifiers);
                }

                auto classMetaData = node.GetClassMetadata();
                if (classMetaData)
                {
                    // some non-visible type paths are not added to the node map
                    if (addNodeToMap)
                    {
                        m_nodeMap.emplace(path, PropertyTreeEditorNode(&node, {}, previousPath, changeNotifiers));
                    }

                    // Add support for deprecated names.
                    // Note that the property paths are unique, but deprecated paths can introduce collisions.
                    // In those cases, the non-deprecated paths have precedence and hide the deprecated ones.
                    // Also, in the case of multiple properties with the same deprecated paths, one will hide the other.
                    if (editMetaData->m_deprecatedName && AZStd::char_traits<char>::length(editMetaData->m_deprecatedName) > 0)
                    {
                        AZStd::string deprecatedPath = previousPath;

                        if (!deprecatedPath.empty())
                        {
                            deprecatedPath += '|';
                        }

                        deprecatedPath += editMetaData->m_deprecatedName;

                        // Only add the name to the map if it's not already there.
                        if (m_nodeMap.find(deprecatedPath) == m_nodeMap.end())
                        {
                            m_nodeMap.emplace(deprecatedPath, PropertyTreeEditorNode(&node, editMetaData->m_name, previousPath, changeNotifiers));
                        }
                    }
                }
            }

            if (addChildren)
            {
                PopulateNodeMap(node.GetChildren(), path);
            }
        }
    }

    PropertyModificationRefreshLevel PropertyTreeEditor::PropertyNotify(const PropertyTreeEditorNode* node, size_t optionalIndex)
    {
        // Notify from this node all the way up through parents recursively.
        // Maintain the highest level of requested refresh along the way.

        PropertyModificationRefreshLevel level = Refresh_None;

        if (node->m_nodePtr)
        {
            for (const ChangeNotification& notifier : node->m_notifiers)
            {
                // execute the function or read the value.
                InstanceDataNode* nodeToNotify = notifier.m_node;
                if (nodeToNotify && nodeToNotify->GetClassMetadata()->m_container)
                {
                    nodeToNotify = nodeToNotify->GetParent();
                }

                if (nodeToNotify)
                {
                    for (size_t idx = 0; idx < nodeToNotify->GetNumInstances(); ++idx)
                    {
                        PropertyAttributeReader reader(nodeToNotify->GetInstance(idx), notifier.m_attribute);
                        AZ::u32 refreshLevelCRC = 0;
                        if (!reader.Read<AZ::u32>(refreshLevelCRC))
                        {
                            AZ::Crc32 refreshLevelCrc32;
                            if (reader.Read<AZ::Crc32>(refreshLevelCrc32))
                            {
                                refreshLevelCRC = refreshLevelCrc32;
                            }
                        }

                        if (refreshLevelCRC != 0)
                        {
                            if (refreshLevelCRC == AZ::Edit::PropertyRefreshLevels::None)
                            {
                                level = AZStd::GetMax(Refresh_None, level);
                            }
                            else if (refreshLevelCRC == AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            {
                                level = AZStd::GetMax(Refresh_Values, level);
                            }
                            else if (refreshLevelCRC == AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                            {
                                level = AZStd::GetMax(Refresh_AttributesAndValues, level);
                            }
                            else if (refreshLevelCRC == AZ::Edit::PropertyRefreshLevels::EntireTree)
                            {
                                level = AZStd::GetMax(Refresh_EntireTree, level);
                            }
                            else
                            {
                                AZ_WarningOnce("Property Editor", false,
                                    "Invalid value returned from change notification handler for %s. "
                                    "A CRC of one of the following refresh levels should be returned: "
                                    "RefreshNone, RefreshValues, RefreshAttributesAndValues, RefreshEntireTree.",
                                    nodeToNotify->GetElementEditMetadata() ? nodeToNotify->GetElementEditMetadata()->m_name : nodeToNotify->GetClassMetadata()->m_name);
                            }
                        }
                        else
                        {
                            // Support invoking a void handler (either taking no parameters or an index)
                            // (doesn't return a refresh level)
                            AZ::Edit::AttributeFunction<void()>* func =
                                azdynamic_cast<AZ::Edit::AttributeFunction<void()>*>(reader.GetAttribute());
                            AZ::Edit::AttributeFunction<void(size_t)>* funcWithIndex =
                                azdynamic_cast<AZ::Edit::AttributeFunction<void(size_t)>*>(reader.GetAttribute());

                            if (func)
                            {
                                func->Invoke(nodeToNotify->GetInstance(idx));
                            }
                            else if (funcWithIndex)
                            {
                                // if a function has been provided that takes an index, use this version
                                // passing through the index of the element being modified
                                funcWithIndex->Invoke(nodeToNotify->GetInstance(idx), optionalIndex);
                            }
                            else
                            {
                                AZ_WarningOnce("Property Editor", false,
                                    "Unable to invoke change notification handler for %s. "
                                    "Handler must return void, or the CRC of one of the following refresh levels: "
                                    "RefreshNone, RefreshValues, RefreshAttributesAndValues, RefreshEntireTree.",
                                    nodeToNotify->GetElementEditMetadata() ? nodeToNotify->GetElementEditMetadata()->m_name : nodeToNotify->GetClassMetadata()->m_name);
                            }
                        }
                    }
                }
            }
        }

        if (!node->m_parentPath.empty() && m_nodeMap.find(node->m_parentPath) != m_nodeMap.end())
        {
            return static_cast<PropertyModificationRefreshLevel>(
                AZStd::GetMax(
                    aznumeric_cast<int>(PropertyNotify(&m_nodeMap[node->m_parentPath], optionalIndex)),
                    aznumeric_cast<int>(level)
                )
            );
        }

        return level;
    }
}
