/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/DOM/DomPath.h>
#include <AzCore/DOM/DomUtils.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorSystemInterface.h>
#include <AzFramework/DocumentPropertyEditor/Reflection/LegacyReflectionBridge.h>

namespace AZ::Reflection
{
    namespace DescriptorAttributes
    {
        const Name Handler = Name::FromStringLiteral("Handler", AZ::Interface<AZ::NameDictionary>::Get());
        const Name Label = Name::FromStringLiteral("Label", AZ::Interface<AZ::NameDictionary>::Get());
        const Name Description = Name::FromStringLiteral("Description", AZ::Interface<AZ::NameDictionary>::Get());
        const Name SerializedPath = Name::FromStringLiteral("SerializedPath", AZ::Interface<AZ::NameDictionary>::Get());
        const Name Container = Name::FromStringLiteral("Container", AZ::Interface<AZ::NameDictionary>::Get());
        const Name ParentContainer = Name::FromStringLiteral("ParentContainer", AZ::Interface<AZ::NameDictionary>::Get());
        const Name ParentContainerInstance = Name::FromStringLiteral("ParentContainerInstance", AZ::Interface<AZ::NameDictionary>::Get());
        const Name ParentContainerCanBeModified =
            Name::FromStringLiteral("ParentContainerCanBeModified", AZ::Interface<AZ::NameDictionary>::Get());
        const Name ContainerElementOverride = Name::FromStringLiteral("ContainerElementOverride", AZ::Interface<AZ::NameDictionary>::Get());
    } // namespace DescriptorAttributes

    // attempts to stringify the passed instance; useful for things like maps where the key element should not be user editable
    bool GetValueStringHelper(
        void* instance,
        const AZ::SerializeContext::ClassData* classData,
        const AZ::SerializeContext::ClassElement* classElement,
        AZ::SerializeContext* serializeContext,
        AZStd::string& value)
    {
        if (!classData)
        {
            return false;
        }
        if (!serializeContext)
        {
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        }

        const DocumentPropertyEditor::PropertyEditorSystemInterface* propertyEditorSystem =
            AZ::Interface<DocumentPropertyEditor::PropertyEditorSystemInterface>::Get();

        auto extractUuidProperty = [&propertyEditorSystem](auto& attributes, const AZ::Name& attributeName, AZ::Uuid& destination) -> bool
        {
            for (auto it = attributes.begin(); it != attributes.end(); ++it)
            {
                AZ::Name currentName = propertyEditorSystem->LookupNameFromId(it->first);
                if (currentName == attributeName)
                {
                    AZ::AttributeReader reader(nullptr, it->second.get());
                    if (reader.Read<AZ::Uuid>(destination))
                    {
                        return true;
                    }
                }
            }
            return false;
        };

        // Get our enum string value from the edit context if we're actually an enum
        AZ::Uuid enumId;
        extractUuidProperty(classElement->m_attributes, Name("EnumType"), enumId);

        if (serializeContext && !enumId.IsNull())
        {
            // got the enumID, now get the enum's underlying type
            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                const AZ::Edit::ElementData* enumElementData = editContext->GetEnumElementData(enumId);
                if (enumElementData)
                {
                    AZ::Uuid underlyingTypeId;
                    extractUuidProperty(classData->m_attributes, Name("EnumUnderlyingType"), underlyingTypeId);

                    // Check all underlying enum storage types for the actual representation type to query
                    if (!underlyingTypeId.IsNull() &&
                        AZ::Utils::GetEnumStringRepresentation<AZ::u8, AZ::u16, AZ::u32, AZ::u64, AZ::s8, AZ::s16, AZ::s32, AZ::s64>(
                            value, enumElementData, instance, underlyingTypeId))
                    {
                        return true;
                    }
                }
            }
        }

        // Just use our underlying AZStd::string if we're a string
        if (classData->m_typeId == azrtti_typeid<AZStd::string>())
        {
            value = *reinterpret_cast<AZStd::string*>(instance);
            return true;
        }

        // Fall back on using our serializer's DataToText
        if (classElement)
        {
            if (auto& serializer = classData->m_serializer)
            {
                AZ::IO::MemoryStream memStream(instance, 0, classElement->m_dataSize);
                AZ::IO::ByteContainerStream outStream(&value);
                serializer->DataToText(memStream, outStream, false);
                return !value.empty();
            }
        }

        return false;
    }

    namespace LegacyReflectionInternal
    {
        struct InstanceVisitor
            : IObjectAccess
            , IAttributes
        {
            IReadWrite* m_visitor;
            SerializeContext* m_serializeContext;
            SerializeContext::EnumerateInstanceCallContext* m_enumerateContext;

            struct AttributeData
            {
                // Group from the attribute metadata (generally empty with Serialize/EditContext data)
                Name m_group;
                // Name of the attribute
                Name m_name;
                // DOM value of the attribute - currently only primitive attributes are supported,
                // but other types may later be supported via opaque values
                Dom::Value m_value;
            };

            struct StackEntry
            {
                void* m_instance;
                // Instance that is used to retrieve attribute values that are tied to invokable functions.
                // Commonly, it is the parent instance for property nodes, and the instance for UI element nodes.
                void* m_instanceToInvoke = nullptr;
                AZ::TypeId m_typeId;
                const SerializeContext::ClassData* m_classData = nullptr;
                const SerializeContext::ClassElement* m_classElement = nullptr;
                AZStd::vector<AttributeData> m_cachedAttributes;
                AZStd::string m_path;
                DocumentPropertyEditor::Nodes::PropertyVisibility m_computedVisibility =
                    DocumentPropertyEditor::Nodes::PropertyVisibility::Show;
                bool m_entryClosed = false;
                size_t m_childElementIndex = 0; // TODO: this should store a PathEntry and support text for associative containers

                AZStd::string_view m_group;
                bool m_isOpenGroup = false;

                // special handling for labels and editors
                bool m_skipLabel = false;
                AZStd::string m_labelOverride;
                bool m_disableEditor = false;
                bool m_isAncestorDisabled = false;

                bool m_skipHandler = false;
                bool m_createdByEnumerate = false;

                // extra data necessary to support Containers composed of pair<> children (like maps!)
                bool m_extractKeyedPair = false;
                AZ::SerializeContext::IDataContainer* m_parentContainerInfo = nullptr;
                void* m_parentContainerOverride = nullptr;
                void* m_containerElementOverride = nullptr;

                const AZ::Edit::ElementData* GetElementEditMetadata() const
                {
                    if (m_classElement)
                    {
                        return m_classElement->m_editData;
                    }

                    return nullptr;
                }

                AZStd::vector<AZStd::pair<AZStd::string, AZStd::optional<StackEntry>>> m_groups;
                AZStd::map<AZStd::string, AZStd::vector<StackEntry>> m_groupEntries;
                AZStd::map<AZStd::string, AZStd::string> m_propertyToGroupMap;
            };
            AZStd::deque<StackEntry> m_stack;
            AZStd::vector<AZStd::pair<AZStd::string, StackEntry>> m_nonSerializedElements;

            bool m_nodeWasSkipped = false;

            using HandlerCallback = AZStd::function<bool()>;
            AZStd::unordered_map<AZ::TypeId, HandlerCallback> m_handlers;

            static constexpr auto VisibilityBoolean = AZ::DocumentPropertyEditor::AttributeDefinition<bool>("VisibilityBoolean");

            // Specify whether the visit starts from the root of the instance.
            bool m_visitFromRoot = true;

            InstanceVisitor(
                IReadWrite* visitor,
                void* instance,
                const AZ::TypeId& typeId,
                SerializeContext* serializeContext,
                bool visitFromRoot = true)
                : m_visitor(visitor)
                , m_serializeContext(serializeContext)
            {
                // Push a dummy node into stack, which serves as the parent node for the first node.
                m_stack.push_back({ instance, nullptr, typeId });

                m_visitFromRoot = visitFromRoot;

                RegisterPrimitiveHandlers<
                    bool,
                    char,
                    AZ::u8,
                    AZ::u16,
                    AZ::u32,
                    AZ::u64,
                    AZ::s8,
                    AZ::s16,
                    AZ::s32,
                    AZ::s64,
                    float,
                    double>();

                m_enumerateContext = new SerializeContext::EnumerateInstanceCallContext(
                    [this](
                        void* instance,
                        const AZ::SerializeContext::ClassData* classData,
                        const AZ::SerializeContext::ClassElement* classElement)
                    {
                        return BeginNode(instance, classData, classElement);
                    },
                    [this]()
                    {
                        return EndNode();
                    },
                    m_serializeContext,
                    SerializeContext::EnumerationAccessFlags::ENUM_ACCESS_FOR_READ,
                    nullptr);
            }

            virtual ~InstanceVisitor()
            {
                delete m_enumerateContext;
            }

            template<typename T>
            void RegisterHandler(AZStd::function<bool(T&)> handler)
            {
                m_handlers[azrtti_typeid<T>()] = [this, handler = AZStd::move(handler)]() -> bool
                {
                    return handler(*reinterpret_cast<T*>(m_stack.back().m_instance));
                };
            }

            template<typename... T>
            void RegisterPrimitiveHandlers()
            {
                (RegisterHandler<T>(
                     [this](T& value) -> bool
                     {
                         m_visitor->Visit(value, *this);
                         return false;
                     }),
                 ...);
            }

            void Visit()
            {
                AZ_Assert(m_serializeContext->GetEditContext() != nullptr, "LegacyReflectionBridge relies on EditContext");
                if (m_serializeContext->GetEditContext() == nullptr)
                {
                    return;
                }

                // Note that this is the dummy parent node for the root node. It contains null classData and classElement.
                const StackEntry& nodeData = m_stack.back();
                m_serializeContext->EnumerateInstance(m_enumerateContext, nodeData.m_instance, nodeData.m_typeId, nullptr, nullptr);
            }

            void GenerateNodePath(const StackEntry& parentData, StackEntry& nodeData)
            {
                AZStd::string path = parentData.m_path;
                if (parentData.m_classData && parentData.m_classData->m_container)
                {
                    path.append("/");
                    path.append(AZStd::string::format("%zu", parentData.m_childElementIndex));
                }
                else if (nodeData.m_classElement)
                {
                    AZStd::string_view elementName = nodeData.m_classElement->m_name;

                    // Construct the serialized path for only those elements that have valid edit data. Otherwise, you can end up with
                    // serialized paths looking like "token1////token2/token3"
                    if (!elementName.empty())
                    {
                        path.append("/");
                        path.append(elementName);
                    }
                }

                nodeData.m_path = AZStd::move(path);
            }

            bool HandleNodeInParentGroup(const StackEntry& nodeData, StackEntry& parentData)
            {
                if (parentData.m_propertyToGroupMap.contains(nodeData.m_path))
                {
                    // Add to group vector.
                    AZStd::string groupName = parentData.m_propertyToGroupMap[nodeData.m_path];
                    parentData.m_groupEntries[groupName].push_back(AZStd::move(nodeData));
                    m_stack.pop_back();

                    return true;
                }

                return false;
            }

            void HandleNodeGroups(StackEntry& nodeData)
            {
                // Search through classData for Groups.
                if (nodeData.m_classData && nodeData.m_classData->m_editData)
                {
                    AZStd::string groupName = "";
                    int groupCounter = 0;

                    for (auto iter = nodeData.m_classData->m_editData->m_elements.begin(),
                              endIter = nodeData.m_classData->m_editData->m_elements.end();
                         iter != endIter;
                         ++iter)
                    {
                        auto& currElement = *iter;

                        //! If this node has group definitions in its element, create the vectors.
                        if (currElement.m_elementId == AZ::Edit::ClassElements::Group)
                        {
                            AZStd::string_view itemDescription(currElement.m_description);

                            if (!itemDescription.empty())
                            {
                                // Update groupName to new group's name
                                groupName = currElement.m_description;
                                nodeData.m_groupEntries.insert(groupName);

                                if (currElement.m_serializeClassElement)
                                {
                                    // groups with a serialize class element are toggle groups, make an entry for their bool value
                                    void* boolAddress = reinterpret_cast<void*>(
                                        reinterpret_cast<size_t>(nodeData.m_instance) + currElement.m_serializeClassElement->m_offset);

                                    StackEntry entry = { boolAddress,
                                                         nodeData.m_instance,
                                                         currElement.m_serializeClassElement->m_typeId,
                                                         nullptr,
                                                         currElement.m_serializeClassElement };
                                    nodeData.m_groups.emplace_back(AZStd::make_pair(groupName, AZStd::move(entry)));

                                    AZStd::string propertyPath = AZStd::string::format(
                                        "%s/%s", nodeData.m_path.c_str(), currElement.m_serializeClassElement->m_name);
                                    nodeData.m_propertyToGroupMap.insert({ propertyPath, groupName });
                                }
                                else
                                {
                                    // not a toggle group, just make the normal group UI element
                                    AZ::SerializeContext::ClassElement* UIElement = new AZ::SerializeContext::ClassElement();
                                    UIElement->m_editData = &currElement;
                                    UIElement->m_flags = SerializeContext::ClassElement::Flags::FLG_UI_ELEMENT;
                                    StackEntry entry = { nodeData.m_instance,
                                                         nodeData.m_instance,
                                                         nodeData.m_classData->m_typeId,
                                                         nodeData.m_classData,
                                                         UIElement };

                                    nodeData.m_groups.emplace_back(AZStd::make_pair(groupName, AZStd::move(entry)));
                                }
                            }
                            else
                            {
                                // If the group name is empty, this is the end of the previous group.
                                // Create a group for entries that should be displayed after the previous group.
                                if (!groupName.empty())
                                {
                                    groupName = AZStd::string::format("_GROUP[%d]", ++groupCounter);
                                    nodeData.m_groupEntries.insert(groupName);
                                    nodeData.m_groups.emplace_back(AZStd::make_pair(groupName, AZStd::nullopt));
                                }
                            }
                        }
                        else
                        {
                            if (!groupName.empty() && currElement.m_serializeClassElement)
                            {
                                AZStd::string propertyPath =
                                    AZStd::string::format("%s/%s", nodeData.m_path.c_str(), currElement.m_serializeClassElement->m_name);
                                nodeData.m_propertyToGroupMap.insert({ propertyPath, groupName });
                            }
                        }
                    }
                }
            }

            void HandleNodeUiElementsRetrieval(const StackEntry& nodeData)
            {
                // Search through classData for UIElements and Editor Data.
                if (nodeData.m_classData && nodeData.m_classData->m_editData)
                {
                    // Store current group.
                    AZStd::string groupName = "";
                    AZStd::string lastValidElementName = "";

                    for (auto iter = nodeData.m_classData->m_editData->m_elements.begin();
                         iter != nodeData.m_classData->m_editData->m_elements.end();
                         ++iter)
                    {
                        if (iter->m_elementId == AZ::Edit::ClassElements::Group)
                        {
                            // The group name is stored in the description.
                            groupName = iter->m_description;

                            if (groupName.empty())
                            {
                                // If the group name is empty, this is the end of the previous group.
                                // As such, we reset the lastValidElementName.
                                lastValidElementName = "";
                            }
                        }
                        else if (iter->m_elementId == AZ::Edit::ClassElements::UIElement)
                        {
                            AZ::SerializeContext::ClassElement* UIElement = new AZ::SerializeContext::ClassElement();
                            UIElement->m_editData = &*iter;
                            UIElement->m_flags = SerializeContext::ClassElement::Flags::FLG_UI_ELEMENT;
                            // Use the instance itself to retrieve parameter values that invoke functions on the UI Element.
                            StackEntry entry = {
                                nodeData.m_instance, nodeData.m_instance, nodeData.m_classData->m_typeId, nodeData.m_classData, UIElement
                            };

                            AZStd::string pathString = nodeData.m_path;

                            if (!groupName.empty())
                            {
                                pathString.append("/");
                                pathString.append(groupName);
                            }

                            if (!lastValidElementName.empty())
                            {
                                pathString.append("/");
                                pathString.append(lastValidElementName);
                            }

                            m_nonSerializedElements.push_back(AZStd::make_pair(pathString.c_str(), entry));
                        }
                        else
                        {
                            // Keep track of the last valid element name
                            if (iter->m_serializeClassElement && strlen(iter->m_serializeClassElement->m_name) > 0)
                            {
                                lastValidElementName = iter->m_serializeClassElement->m_name;
                            }
                        }
                    }
                }
            }

            void HandleNodeUiElementsCreation(const AZStd::string_view path)
            {
                // Iterate over non serialized elements to see if any of them should be added
                auto iter = m_nonSerializedElements.begin();
                while (iter != m_nonSerializedElements.end())
                {
                    // If the parent of the element that was just created has the same name as the parent of any non serialized
                    // elements, and the element that was just created is the element immediately before any non serialized element,
                    // create that serialized element
                    if (path == iter->first && iter->second.m_classElement->m_editData->m_elementId == AZ::Edit::ClassElements::UIElement)
                    {
                        m_stack.push_back(iter->second);
                        CacheAttributes();
                        m_stack.back().m_entryClosed = true;
                        m_visitor->VisitObjectBegin(*this, *this);
                        m_visitor->VisitObjectEnd(*this, *this);
                        m_stack.pop_back();

                        iter = m_nonSerializedElements.erase(iter);
                    }
                    else
                    {
                        ++iter;
                    }
                }
            }

            AZStd::optional<bool> HandleNodeAssociativeInterface(StackEntry& parentData, StackEntry& nodeData)
            {
                if (parentData.m_classData && parentData.m_classData->m_container)
                {
                    auto& containerInfo = parentData.m_classData->m_container;
                    AZ::SerializeContext::IDataContainer::IAssociativeDataContainer* parentAssociativeInterface =
                        containerInfo->GetAssociativeContainerInterface();

                    if (parentAssociativeInterface)
                    {
                        if (nodeData.m_instance == parentAssociativeInterface->GetValueByKey(parentData.m_instance, nodeData.m_instance))
                        {
                            // the element *is* the key. This is some kind of set and has only one read-only value per entry
                            nodeData.m_skipLabel = true;
                            nodeData.m_disableEditor = true;

                            // TODO: if preferred, we can still include a label for set elements,
                            // in this case, the stringified value
                            /*  GetValueStringHelper(
                                nodeData->m_instance,
                                nodeData->m_classData,
                                nodeData->m_classElement,
                                m_serializeContext,
                                nodeData->m_labelOverride); */
                        }
                        else
                        {
                            // parent is a real associative container with pair<> children,
                            // extract the info from the parent and just show label/editor pairs
                            nodeData.m_extractKeyedPair = true;
                            nodeData.m_parentContainerInfo = parentData.m_classData->m_container;
                            nodeData.m_parentContainerOverride = parentData.m_instance;
                            nodeData.m_containerElementOverride = nodeData.m_instance;
                            nodeData.m_entryClosed = true;
                            return true;
                        }
                    }
                }

                if (parentData.m_extractKeyedPair)
                {
                    // alternate behavior for the pair children. The first is the key, stringify it
                    if (parentData.m_childElementIndex % 2 == 0)
                    {
                        // store the label override in the parent for the next child to consume
                        GetValueStringHelper(
                            nodeData.m_instance,
                            nodeData.m_classData,
                            nodeData.m_classElement,
                            m_serializeContext,
                            parentData.m_labelOverride);
                        nodeData.m_entryClosed = true;
                        return true;
                    }
                    else // the second is the value, make an editor as normal
                    {
                        // this is the second pair<> child, consume the label override stored above
                        nodeData.m_labelOverride = parentData.m_labelOverride;
                    }
                }

                return AZStd::nullopt;
            }

            AZStd::optional<bool> HandleNodeAttributes(StackEntry& parentData, StackEntry& nodeData)
            {
                // If our parent node is disabled then we should inherit its disabled state
                if (parentData.m_disableEditor || parentData.m_isAncestorDisabled)
                {
                    nodeData.m_isAncestorDisabled = true;
                }

                // Cache attributes for the current node. Attribute data will be used in ReflectionAdapter.
                CacheAttributes();

                // Inherit the change notify attribute from our parent, if it exists
                const auto changeNotifyName = DocumentPropertyEditor::Nodes::PropertyEditor::ChangeNotify.GetName();
                auto parentValue = Find(Name(), changeNotifyName, parentData);
                if (parentValue && !parentValue->IsNull())
                {
                    Dom::Value* existingValue = nullptr;
                    auto it = AZStd::find_if(
                        nodeData.m_cachedAttributes.begin(),
                        nodeData.m_cachedAttributes.end(),
                        [&changeNotifyName](const AttributeData& attributeData)
                        {
                            return (attributeData.m_name == changeNotifyName);
                        });

                    if (it != nodeData.m_cachedAttributes.end())
                    {
                        existingValue = &it->m_value;
                    }

                    auto addValueToArray = [](const Dom::Value& source, Dom::Value& destination)
                    {
                        if (source.IsArray())
                        {
                            auto& destinationArray = destination.GetMutableArray();
                            destinationArray.insert(destinationArray.end(), source.ArrayBegin(), source.ArrayEnd());
                        }
                        else
                        {
                            destination.ArrayPushBack(source);
                        }
                    };

                    // calling order matters! Add parent's attributes first then existing attribute
                    if (existingValue)
                    {
                        Dom::Value newChangeNotifyValue;
                        newChangeNotifyValue.SetArray();
                        addValueToArray(*parentValue, newChangeNotifyValue);
                        addValueToArray(*existingValue, newChangeNotifyValue);
                        nodeData.m_cachedAttributes.push_back({ Name(), changeNotifyName, newChangeNotifyValue });
                    }
                    else
                    {
                        // no existing changeNotify, so let's just inherit the parent's one
                        nodeData.m_cachedAttributes.push_back({ Name(), changeNotifyName, *parentValue });
                    }
                }

                const auto& EnumTypeAttribute = DocumentPropertyEditor::Nodes::PropertyEditor::EnumUnderlyingType;
                AZStd::optional<AZ::TypeId> enumTypeId = {};
                if (auto enumTypeValue = Find(EnumTypeAttribute.GetName()); enumTypeValue)
                {
                    enumTypeId = EnumTypeAttribute.DomToValue(*enumTypeValue);
                }

                const AZ::TypeId* typeIdForHandler = &nodeData.m_typeId;
                if (enumTypeId.has_value())
                {
                    typeIdForHandler = &enumTypeId.value();
                }

                if (auto handlerIt = m_handlers.find(*typeIdForHandler); handlerIt != m_handlers.end())
                {
                    nodeData.m_entryClosed = true;
                    return handlerIt->second();
                }

                return AZStd::nullopt;
            }

            bool BeginNode(
                void* instance, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* classElement)
            {
                // Ensure our instance pointer is resolved and safe to bind to member attributes.
                if (classElement)
                {
                    instance = AZ::Utils::ResolvePointer(instance, *classElement, *m_serializeContext);
                }

                // Prepare the node references for the handlers.
                StackEntry& parentData = m_stack.back();

                // search up the stack for the "true parent", which is the last entry created by the serialize enumerate itself
                void* instanceToInvoke = instance;
                for (auto rIter = m_stack.rbegin(), rEnd = m_stack.rend(); rIter != rEnd; ++rIter)
                {
                    auto* currInstance = rIter->m_instance;
                    if (rIter->m_createdByEnumerate && currInstance)
                    {
                        instanceToInvoke = currInstance;
                        break;
                    }
                }

                StackEntry newEntry = {
                    instance, instanceToInvoke, classData ? classData->m_typeId : Uuid::CreateNull(), classData, classElement
                };
                newEntry.m_createdByEnumerate = true;

                m_stack.push_back(newEntry);

                StackEntry& nodeData = m_stack.back();

                // Generate this node's path (will be stored in nodeData.m_path)
                GenerateNodePath(parentData, nodeData);

                // If this instance is part of a group in its parent, postpone this iteration.
                // It will be re-executed in the parent's EndNode.
                if (HandleNodeInParentGroup(nodeData, parentData))
                {
                    m_nodeWasSkipped = true;
                    // Return false to prevent descendants from being enumerated.
                    return false;
                }

                HandleNodeGroups(nodeData);
                HandleNodeUiElementsRetrieval(nodeData);
                HandleNodeUiElementsCreation("");

                if (auto result = HandleNodeAssociativeInterface(parentData, nodeData); result.has_value())
                {
                    return result.value();
                }

                if (auto result = HandleNodeAttributes(parentData, nodeData); result.has_value())
                {
                    return result.value();
                }

                m_visitor->VisitObjectBegin(*this, *this);

                return true;
            }

            bool EndNode()
            {
                // Return early if node was skipped.
                if (m_nodeWasSkipped)
                {
                    m_nodeWasSkipped = false;
                    return true;
                }

                // Use current node before popping it from the stack.
                {
                    StackEntry& nodeData = m_stack.back();

                    if (!nodeData.m_entryClosed)
                    {
                        m_visitor->VisitObjectEnd(*this, *this);
                    }

                    // Handle groups
                    if (nodeData.m_groups.size() > 0)
                    {
                        for (auto& groupPair : nodeData.m_groups)
                        {
                            if (groupPair.second.has_value())
                            {
                                auto& groupStackEntry = groupPair.second.value();
                                groupStackEntry.m_group = groupPair.first;

                                if (groupStackEntry.m_classElement->m_editData->m_serializeClassElement)
                                {
                                    groupStackEntry.m_skipHandler = true;
                                }
                                m_stack.push_back(groupStackEntry);
                                CacheAttributes();
                                m_visitor->VisitObjectBegin(*this, *this);
                            }

                            AZStd::string path = AZStd::string::format("%s/%s", nodeData.m_path.c_str(), groupPair.first.c_str());
                            HandleNodeUiElementsCreation(path);

                            for (const auto& groupEntry : nodeData.m_groupEntries[groupPair.first])
                            {
                                if (groupPair.second.has_value() &&
                                    groupPair.second.value().m_classElement->m_editData->m_serializeClassElement ==
                                        groupEntry.m_classElement)
                                {
                                    // skip the bool that represented the group toggle, it's already in-line with the group
                                    continue;
                                }
                                m_stack.push_back({ groupEntry.m_instance, nullptr, AZ::TypeId() });
                                m_serializeContext->EnumerateInstance(
                                    m_enumerateContext,
                                    groupEntry.m_instance,
                                    groupEntry.m_typeId,
                                    groupEntry.m_classData,
                                    groupEntry.m_classElement);
                                m_stack.pop_back();
                            }

                            if (groupPair.second.has_value())
                            {
                                m_visitor->VisitObjectEnd(*this, *this);
                                m_stack.pop_back();
                            }
                        }

                        nodeData.m_propertyToGroupMap.clear();
                        nodeData.m_groupEntries.clear();
                        nodeData.m_groups.clear();
                    }

                    if (nodeData.m_classElement && strlen(nodeData.m_classElement->m_name) > 0)
                    {
                        HandleNodeUiElementsCreation(nodeData.m_path);
                    }

                    m_stack.pop_back();
                }

                // The back of the stack now holds the parent.
                {
                    StackEntry& parentData = m_stack.back();

                    if (!parentData.m_group.empty())
                    {
                        EndNode();
                    }

                    if (!m_stack.empty() && parentData.m_computedVisibility == DocumentPropertyEditor::Nodes::PropertyVisibility::Show)
                    {
                        ++m_stack.back().m_childElementIndex;
                    }
                }

                return true;
            }

            const AZ::TypeId& GetType() const override
            {
                return m_stack.back().m_typeId;
            }

            AZStd::string_view GetTypeName() const override
            {
                const AZ::SerializeContext::ClassData* classData = m_serializeContext->FindClassData(m_stack.back().m_typeId);
                return classData ? classData->m_name : "<unregistered type>";
            }

            void* Get() override
            {
                return m_stack.back().m_instance;
            }

            const void* Get() const override
            {
                return m_stack.back().m_instance;
            }

            AZStd::string_view GetNodeDisplayLabel(StackEntry& nodeData, AZStd::fixed_string<128>& labelAttributeBuffer)
            {
                using DocumentPropertyEditor::Nodes::PropertyEditor;

                // First check for overrides or for presence of parent container
                if (!nodeData.m_labelOverride.empty())
                {
                    return nodeData.m_labelOverride;
                }
                else if (auto nameLabelOverrideAttribute = Find(PropertyEditor::NameLabelOverride.GetName()); nameLabelOverrideAttribute)
                {
                    nodeData.m_labelOverride = PropertyEditor::NameLabelOverride.DomToValue(*nameLabelOverrideAttribute).value_or("");
                    return nodeData.m_labelOverride;
                }
                else if (!nodeData.m_group.empty())
                {
                    return nodeData.m_group;
                }
                else if (m_stack.size() > 1)
                {
                    const StackEntry& parentNode = m_stack[m_stack.size() - 2];
                    if (parentNode.m_classData && parentNode.m_classData->m_container)
                    {
                        labelAttributeBuffer = AZStd::fixed_string<128>::format("[%zu]", parentNode.m_childElementIndex);
                        return labelAttributeBuffer;
                    }
                }

                // No overrides, so check the element edit data, class data, and class element
                if (const auto metadata = nodeData.GetElementEditMetadata(); metadata && metadata->m_name)
                {
                    return metadata->m_name;
                }
                else if (nodeData.m_classData)
                {
                    if (nodeData.m_classData->m_editData && nodeData.m_classData->m_editData->m_name)
                    {
                        return nodeData.m_classData->m_editData->m_name;
                    }
                    else if (
                        nodeData.m_classElement && nodeData.m_classElement->m_name && nodeData.m_classData->m_container &&
                        nodeData.m_classElement->m_nameCrc != nodeData.m_classData->m_container->GetDefaultElementNameCrc())
                    {
                        return nodeData.m_classElement->m_name;
                    }
                    else if (nodeData.m_classData->m_name)
                    {
                        return nodeData.m_classData->m_name;
                    }
                }

                return {};
            }

            void CacheAttributes()
            {
                StackEntry& nodeData = m_stack.back();
                AZStd::vector<AttributeData>& cachedAttributes = nodeData.m_cachedAttributes;
                cachedAttributes.clear();

                // Legacy reflection doesn't have groups, so they're in the root "" group
                const Name group;
                AZStd::unordered_set<Name> visitedAttributes;

                AZStd::string_view labelAttributeValue;
                AZStd::fixed_string<128> labelAttributeBuffer;

                AZStd::string_view descriptionAttributeValue;

                DocumentPropertyEditor::PropertyEditorSystemInterface* propertyEditorSystem =
                    AZ::Interface<DocumentPropertyEditor::PropertyEditorSystemInterface>::Get();
                AZ_Assert(propertyEditorSystem != nullptr, "LegacyReflectionBridge: Unable to retrieve PropertyEditorSystem");

                using DocumentPropertyEditor::Nodes::PropertyEditor;
                using DocumentPropertyEditor::Nodes::PropertyVisibility;

                // DPE defaults to show everything, and picks what to hide.
                PropertyVisibility visibility = PropertyVisibility::Show;

                // If the stack contains 2 nodes, it means we are now processing the root node. The first node is a dummy parent node.
                // Hide the root node itself if the visitor is visiting from the instance's root.
                if (m_stack.size() == 2 && m_visitFromRoot)
                {
                    visibility = PropertyVisibility::ShowChildrenOnly;
                }

                Name handlerName;

                // This array node is for caching related GenericValue and EnumValueKey attributes if any are seen
                Dom::Value genericValueCache = Dom::Value(Dom::Type::Array);
                const Name genericValueName = Name("GenericValue");
                const Name enumValueKeyName = Name("EnumValueKey");
                const Name genericValueListName = Name("GenericValueList");
                const Name enumValuesCrcName = Name(static_cast<u32>(Crc32("EnumValues")));

                auto checkAttribute = [&](const AttributePair* it, void* instance, bool shouldDescribeChildren)
                {
                    bool describesChildren = it->second->m_describesChildren;
                    if (it->second->m_describesChildren != shouldDescribeChildren)
                    {
                        return;
                    }

                    // The m_describesChildren flag is true if the attribute is an ElementAttribute,
                    // in which case the instance we want to invoke on is the actual container element
                    // instance, as opposed to the usual invoke instance.
                    if (describesChildren)
                    {
                        instance = nodeData.m_instance;
                    }

                    Name name = propertyEditorSystem->LookupNameFromId(it->first);
                    if (!name.IsEmpty())
                    {
                        // If an attribute of the same name is already loaded then ignore the new value unless
                        // it is a GenericValue or EnumValueKey attribute since each represents an individual
                        // (value, description) pair destined for a combobox and thus multiple are expected
                        if (visitedAttributes.contains(name) && name != genericValueName && name != enumValueKeyName)
                        {
                            return;
                        }
                        visitedAttributes.insert(name);

                        // Handle visibility calculations internally, as we calculate and emit an aggregate visiblity value.
                        // We also need to handle special cases here, because the Visibility attribute supports 3 different value types:
                        //      1. AZ::Crc32 - This is the default
                        //      2. AZ::u32 - This allows the user to specify a value of 1/0 for Show/Hide, respectively
                        //      3. bool - This allows the user to specify true/false for Show/Hide, respectively
                        //
                        // We need to return out of checkAttribute for Visibility attributes since the attributeValue handling
                        // below doesn't account for these special cases. The Visibility attribute instead gets cached at
                        // the end of the CacheAttributes method after it has done further visibility computations.
                        if (name == PropertyEditor::Visibility.GetName())
                        {
                            auto visibilityValue = PropertyEditor::Visibility.DomToValue(
                                PropertyEditor::Visibility.LegacyAttributeToDomValue(instance, it->second));

                            if (visibilityValue.has_value())
                            {
                                visibility = visibilityValue.value();

                                // The PropertyEditor::Visibility is actually an AZ::u32 enum class, so we need
                                // to check here if we read in a 0 or 1 instead of a hash so we can handle
                                // those special cases.
                                AZ::u32 visibilityNumericValue = static_cast<AZ::u32>(visibility);
                                switch (visibilityNumericValue)
                                {
                                case 0:
                                    visibility = PropertyVisibility::Hide;
                                    break;
                                case 1:
                                    visibility = PropertyVisibility::Show;
                                    break;
                                default:
                                    break;
                                }
                                return;
                            }
                            else if (
                                auto visibilityBoolValue =
                                    VisibilityBoolean.DomToValue(VisibilityBoolean.LegacyAttributeToDomValue(instance, it->second)))
                            {
                                bool isVisible = visibilityBoolValue.value();
                                visibility = isVisible ? PropertyVisibility::Show : PropertyVisibility::Hide;
                                return;
                            }
                        }
                        // The legacy ReadOnly property needs to be converted into the Disabled node property.
                        // If our ancestor is disabled we don't need to read the attribute because this node
                        // will already be disabled as well.
                        else if ((name == PropertyEditor::ReadOnly.GetName()) && !nodeData.m_isAncestorDisabled)
                        {
                            nodeData.m_disableEditor |=
                                PropertyEditor::ReadOnly
                                    .DomToValue(PropertyEditor::ReadOnly.LegacyAttributeToDomValue(instance, it->second))
                                    .value_or(nodeData.m_disableEditor);
                        }

                        // See if any registered attribute definitions can read this attribute
                        Dom::Value attributeValue;
                        propertyEditorSystem->EnumerateRegisteredAttributes(
                            name,
                            [&](const DocumentPropertyEditor::AttributeDefinitionInterface& attributeReader)
                            {
                                if (attributeValue.IsNull())
                                {
                                    attributeValue = attributeReader.LegacyAttributeToDomValue(instance, it->second);
                                }
                            });

                        // Fall back on a generic read that handles primitives.
                        if (attributeValue.IsNull())
                        {
                            attributeValue = ReadGenericAttributeToDomValue(instance, it->second).value_or(Dom::Value());
                        }

                        // If we got a valid DOM value, store it.
                        if (!attributeValue.IsNull())
                        {
                            // If there's an explicitly specified handler (e.g. in the case of a UIElement)
                            // omit our normal synthetic Handler attribute.
                            if (name == DescriptorAttributes::Handler)
                            {
                                handlerName = Name();
                            }

                            // Collect related GenericValue attributes so they can be stored together as GenericValueList
                            if (name == genericValueName)
                            {
                                genericValueCache.ArrayPushBack(attributeValue);
                                return;
                            }
                            // Collect EnumValueKey attributes unless this node has an EnumValues, GenericValue or GenericValueList
                            // attribute. If an EnumValues, GenericValue or GenericValueList attribute is present we do not cache
                            // because such nodes also have internal EnumValueKey attributes that we won't use.
                            // The cached values will be stored as a GenericValueList attribute.
                            if (name == enumValueKeyName)
                            {
                                if (visitedAttributes.contains(enumValuesCrcName) || visitedAttributes.contains(genericValueListName) ||
                                    visitedAttributes.contains(genericValueName))
                                {
                                    return;
                                }

                                genericValueCache.ArrayPushBack(attributeValue);
                                // Forcing the node's typeId to AZ::u64 so the correct property handler will be chosen
                                // in the PropertyEditorSystem.
                                // This is reasonable since the attribute's value is an enum with an underlying integral
                                // type which is safely convertible to AZ::u64.
                                nodeData.m_typeId = AzTypeInfo<u64>::Uuid();
                                return;
                            }

                            cachedAttributes.push_back({ group, AZStd::move(name), AZStd::move(attributeValue) });
                        }
                    }
                    else
                    {
                        AZ_Warning("LegacyReflectionBridge", false, "Unable to lookup name for attribute CRC: %" PRId32, it->first);
                    }
                };

                auto checkNodeAttributes = [&](StackEntry& nodeData, bool isParentAttribute)
                {
                    // Read attributes in order, checking:
                    // 1) Class element edit data attributes (EditContext from the given row of a class)
                    // 2) Class element data attributes (SerializeContext from the given row of a class)
                    // 3) Class data attributes (the base attributes of a class)
                    if (nodeData.m_classElement)
                    {
                        if (const AZ::Edit::ElementData* elementEditData = nodeData.m_classElement->m_editData; elementEditData != nullptr)
                        {
                            if (!isParentAttribute)
                            {
                                if (!nodeData.m_skipHandler && elementEditData->m_elementId)
                                {
                                    handlerName = propertyEditorSystem->LookupNameFromId(elementEditData->m_elementId);
                                }

                                if (elementEditData->m_description)
                                {
                                    descriptionAttributeValue = elementEditData->m_description;
                                }
                            }

                            for (auto it = elementEditData->m_attributes.begin(); it != elementEditData->m_attributes.end(); ++it)
                            {
                                checkAttribute(it, nodeData.m_instanceToInvoke, isParentAttribute);
                            }
                        }

                        for (auto it = nodeData.m_classElement->m_attributes.begin(); it != nodeData.m_classElement->m_attributes.end();
                             ++it)
                        {
                            AZ::AttributePair pair;
                            pair.first = it->first;
                            pair.second = it->second.get();
                            checkAttribute(&pair, nodeData.m_instanceToInvoke, isParentAttribute);
                        }
                    }

                    if (nodeData.m_classData)
                    {
                        if (!isParentAttribute && descriptionAttributeValue.empty() && nodeData.m_classData->m_editData &&
                            nodeData.m_classData->m_editData->m_description)
                        {
                            descriptionAttributeValue = nodeData.m_classData->m_editData->m_description;
                        }

                        for (auto it = nodeData.m_classData->m_attributes.begin(); it != nodeData.m_classData->m_attributes.end(); ++it)
                        {
                            AZ::AttributePair pair;
                            pair.first = it->first;
                            pair.second = it->second.get();
                            checkAttribute(&pair, nodeData.m_instance, isParentAttribute);
                        }

                        // Lastly, check the AZ::SerializeContext::ClassData -> AZ::Edit::ClassData -> EditorData for attributes
                        if (const auto* editClassData = nodeData.m_classData->m_editData; editClassData && !isParentAttribute)
                        {
                            if (const auto* classEditorData = editClassData->FindElementData(AZ::Edit::ClassElements::EditorData))
                            {
                                // Ignore EditorData attributes for UIElements (e.g. groups)
                                bool ignore = false;
                                if (nodeData.m_classElement)
                                {
                                    ignore = (nodeData.m_classElement->m_flags & AZ::SerializeContext::ClassElement::FLG_UI_ELEMENT) != 0;
                                }
                                if (!ignore)
                                {
                                    for (auto it = classEditorData->m_attributes.begin(); it != classEditorData->m_attributes.end(); ++it)
                                    {
                                        checkAttribute(it, nodeData.m_instanceToInvoke, isParentAttribute);
                                    }
                                }
                            }
                        }
                    }
                };

                // Check the current node for attributes without m_describesChildren set.
                checkNodeAttributes(nodeData, false);

                // Check the parent node (if any) for attributes with m_describesChildren set
                if (m_stack.size() > 1)
                {
                    StackEntry& parentNode = m_stack[m_stack.size() - 2];
                    checkNodeAttributes(parentNode, true);

                    if (parentNode.m_classData && parentNode.m_classData->m_container)
                    {
                        auto parentContainerInfo =
                            (parentNode.m_parentContainerInfo ? parentNode.m_parentContainerInfo : parentNode.m_classData->m_container);

                        nodeData.m_cachedAttributes.push_back(
                            { group, DescriptorAttributes::ParentContainer, Dom::Utils::ValueFromType<void*>(parentContainerInfo) });

                        auto parentContainerInstance =
                            (parentNode.m_parentContainerOverride ? parentNode.m_parentContainerOverride : parentNode.m_instance);

                        nodeData.m_cachedAttributes.push_back({ group,
                                                                DescriptorAttributes::ParentContainerInstance,
                                                                Dom::Utils::ValueFromType<void*>(parentContainerInstance) });

                        if (parentNode.m_containerElementOverride)
                        {
                            nodeData.m_cachedAttributes.push_back(
                                { group,
                                  DescriptorAttributes::ContainerElementOverride,
                                  Dom::Utils::ValueFromType<void*>(parentNode.m_containerElementOverride) });
                        }

                        auto canBeModifiedValue =
                            Find(Name(), DocumentPropertyEditor::Nodes::Container::ContainerCanBeModified.GetName(), parentNode);
                        if (canBeModifiedValue)
                        {
                            bool canBeModified = canBeModifiedValue->IsBool() && canBeModifiedValue->GetBool();
                            if (!canBeModified)
                            {
                                nodeData.m_cachedAttributes.push_back(
                                    { group, DescriptorAttributes::ParentContainerCanBeModified, Dom::Value(canBeModified) });
                            }
                        }
                    }
                }

                if (genericValueCache.ArraySize() > 0)
                {
                    nodeData.m_cachedAttributes.push_back(
                        { group, DocumentPropertyEditor::Nodes::PropertyEditor::GenericValueList<AZ::u64>.GetName(), genericValueCache });
                }

                labelAttributeValue = GetNodeDisplayLabel(nodeData, labelAttributeBuffer);

                if (!handlerName.IsEmpty())
                {
                    nodeData.m_cachedAttributes.push_back(
                        { group, DescriptorAttributes::Handler, Dom::Value(handlerName.GetCStr(), false) });
                }
                nodeData.m_cachedAttributes.push_back({ group, DescriptorAttributes::SerializedPath, Dom::Value(nodeData.m_path, true) });
                if (!nodeData.m_skipLabel && !labelAttributeValue.empty())
                {
                    // If we allocated a local label buffer we need to make a copy to store the label
                    const bool shouldCopy = !labelAttributeBuffer.empty();
                    nodeData.m_cachedAttributes.push_back(
                        { group, DescriptorAttributes::Label, Dom::Value(labelAttributeValue, shouldCopy) });
                }

                if (!descriptionAttributeValue.empty())
                {
                    nodeData.m_cachedAttributes.push_back(
                        { group, DescriptorAttributes::Description, Dom::Value(descriptionAttributeValue, false) });
                }

                nodeData.m_cachedAttributes.push_back({ group,
                                                        AZ::DocumentPropertyEditor::Nodes::PropertyEditor::ValueType.GetName(),
                                                        AZ::Dom::Utils::TypeIdToDomValue(nodeData.m_typeId) });

                if (nodeData.m_disableEditor)
                {
                    nodeData.m_cachedAttributes.push_back(
                        { group, AZ::DocumentPropertyEditor::Nodes::NodeWithVisiblityControl::Disabled.GetName(), Dom::Value(true) });
                }

                if (nodeData.m_isAncestorDisabled)
                {
                    nodeData.m_cachedAttributes.push_back(
                        { group,
                          AZ::DocumentPropertyEditor::Nodes::NodeWithVisiblityControl::AncestorDisabled.GetName(),
                          Dom::Value(true) });
                }

                if (nodeData.m_classData && nodeData.m_classData->m_container)
                {
                    nodeData.m_cachedAttributes.push_back(
                        { group, DescriptorAttributes::Container, Dom::Utils::ValueFromType<void*>(nodeData.m_classData->m_container) });
                }

                // RpePropertyHandlerWrapper would cache the parent info from which a wrapped handler may retrieve the parent instance.
                if (nodeData.m_instanceToInvoke)
                {
                    nodeData.m_cachedAttributes.push_back(
                        { group, PropertyEditor::ParentValue.GetName(), Dom::Utils::ValueFromType<void*>(nodeData.m_instanceToInvoke) });
                }

                // Calculate our visibility, going through parent nodes in reverse order to see if we should be hidden
                for (size_t i = 1; i < m_stack.size(); ++i)
                {
                    auto& entry = m_stack[m_stack.size() - 1 - i];
                    if (entry.m_computedVisibility == PropertyVisibility::Hide ||
                        entry.m_computedVisibility == PropertyVisibility::HideChildren)
                    {
                        visibility = PropertyVisibility::Hide;
                        break;
                    }
                }

                // If this node has no edit data and is not the child of a container, only show its children
                auto parentData = m_stack.end() - 2;
                if (nodeData.m_classElement && !nodeData.m_classElement->m_editData &&
                    (parentData->m_classData && !parentData->m_classData->m_container))
                {
                    visibility = DocumentPropertyEditor::Nodes::PropertyVisibility::ShowChildrenOnly;
                }

                nodeData.m_computedVisibility = visibility;
                nodeData.m_cachedAttributes.push_back(
                    { group, PropertyEditor::Visibility.GetName(), Dom::Utils::ValueFromType(visibility) });
            }

            const AttributeDataType* Find(Name name) const override
            {
                return Find(Name(), AZStd::move(name));
            }

            const AttributeDataType* Find(Name group, Name name) const override
            {
                const StackEntry& nodeData = m_stack.back();
                for (auto it = nodeData.m_cachedAttributes.begin(); it != nodeData.m_cachedAttributes.end(); ++it)
                {
                    if (it->m_group == group && it->m_name == name)
                    {
                        return &(it->m_value);
                    }
                }
                return nullptr;
            }

            const AttributeDataType* Find(Name group, Name name, StackEntry& parentData) const
            {
                for (auto it = parentData.m_cachedAttributes.begin(); it != parentData.m_cachedAttributes.end(); ++it)
                {
                    if (it->m_group == group && it->m_name == name)
                    {
                        return &(it->m_value);
                    }
                }
                return nullptr;
            }

            void ListAttributes(const IterationCallback& callback) const override
            {
                const StackEntry& nodeData = m_stack.back();
                for (const AttributeData& data : nodeData.m_cachedAttributes)
                {
                    callback(data.m_group, data.m_name, data.m_value);
                }
            }
        };

        template<typename T>
        bool TryReadAttributeInternal(void* instance, AZ::Attribute* attribute, Dom::Value& result)
        {
            AZ::AttributeReader reader(instance, attribute);
            T value;
            if (reader.Read<T>(value))
            {
                result = Dom::Utils::ValueFromType(value);
                return true;
            }
            return false;
        }

        template<typename... T>
        bool TryReadAttribute(void* instance, AZ::Attribute* attribute, Dom::Value& result)
        {
            return (TryReadAttributeInternal<T>(instance, attribute, result) || ...);
        }

        template<typename T>
        AZStd::shared_ptr<AZ::Attribute> MakeAttribute(T&& value)
        {
            return AZStd::make_shared<AttributeData<T>>(AZStd::move(value));
        }
    } // namespace LegacyReflectionInternal

    AZStd::shared_ptr<AZ::Attribute> WriteDomValueToGenericAttribute(const AZ::Dom::Value& value)
    {
        switch (value.GetType())
        {
        case AZ::Dom::Type::Bool:
            return LegacyReflectionInternal::MakeAttribute(value.GetBool());
        case AZ::Dom::Type::Int64:
            return LegacyReflectionInternal::MakeAttribute(value.GetInt64());
        case AZ::Dom::Type::Uint64:
            return LegacyReflectionInternal::MakeAttribute(value.GetUint64());
        case AZ::Dom::Type::Double:
            return LegacyReflectionInternal::MakeAttribute(value.GetDouble());
        case AZ::Dom::Type::String:
            return LegacyReflectionInternal::MakeAttribute<AZStd::string>(value.GetString());
        default:
            return {};
        }
    }

    AZStd::optional<AZ::Dom::Value> ReadGenericAttributeToDomValue(void* instance, AZ::Attribute* attribute)
    {
        AZ::Dom::Value result = attribute->GetAsDomValue(instance);
        if (!result.IsNull())
        {
            return result;
        }
        return {};
    }

    void VisitLegacyInMemoryInstance(IRead* visitor, void* instance, const AZ::TypeId& typeId, AZ::SerializeContext* serializeContext)
    {
        IReadWriteToRead helper(visitor);
        VisitLegacyInMemoryInstance(&helper, instance, typeId, serializeContext);
    }

    void VisitLegacyInMemoryInstance(IReadWrite* visitor, void* instance, const AZ::TypeId& typeId, AZ::SerializeContext* serializeContext)
    {
        if (serializeContext == nullptr)
        {
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(serializeContext != nullptr, "Unable to retreive a SerializeContext");
        }

        LegacyReflectionInternal::InstanceVisitor helper(visitor, instance, typeId, serializeContext);
        helper.Visit();
    }
} // namespace AZ::Reflection
