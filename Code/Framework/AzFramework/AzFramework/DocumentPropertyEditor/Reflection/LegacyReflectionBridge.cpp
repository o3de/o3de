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
                void* m_parentInstance = nullptr;
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

                // extra data necessary to support Containers composed of pair<> children (like maps!)
                bool m_extractKeyedPair = false;
                AZ::SerializeContext::IDataContainer* m_parentContainerInfo = nullptr;
                void* m_parentContainerOverride = nullptr;
                void* m_containerElementOverride = nullptr;
                int m_insertIndex = 0;
            };
            AZStd::deque<StackEntry> m_stack;
            AZStd::deque<StackEntry> m_UIElements;

            using HandlerCallback = AZStd::function<bool()>;
            AZStd::unordered_map<AZ::TypeId, HandlerCallback> m_handlers;

            virtual ~InstanceVisitor() = default;

            InstanceVisitor(IReadWrite* visitor, void* instance, const AZ::TypeId& typeId, SerializeContext* serializeContext)
                : m_visitor(visitor)
                , m_serializeContext(serializeContext)
            {
                m_stack.push_back({ instance, nullptr, typeId });
                RegisterPrimitiveHandlers<bool, AZ::u8, AZ::u16, AZ::u32, AZ::u64, AZ::s8, AZ::s16, AZ::s32, AZ::s64, float, double>();
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
                SerializeContext::EnumerateInstanceCallContext context(
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
                    SerializeContext::EnumerationAccessFlags::ENUM_ACCESS_FOR_WRITE,
                    nullptr);

                const StackEntry& nodeData = m_stack.back();
                m_serializeContext->EnumerateInstance(&context, nodeData.m_instance, nodeData.m_typeId, nullptr, nullptr);
            }

            bool BeginNode(
                void* instance, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* classElement)
            {
                // Ensure our instance pointer is resolved and safe to bind to member attributes
                if (classElement)
                {
                    instance = AZ::Utils::ResolvePointer(instance, *classElement, *m_serializeContext);
                }

                StackEntry& parentData = m_stack.back();
                AZStd::string path = parentData.m_path;
                AZ::SerializeContext::IDataContainer::IAssociativeDataContainer* parentAssociativeInterface = nullptr;
                if (parentData.m_classData && parentData.m_classData->m_container)
                {
                    path.append("/");
                    path.append(AZStd::string::format("%zu", parentData.m_childElementIndex));

                    auto& containerInfo = parentData.m_classData->m_container;
                    parentAssociativeInterface = containerInfo->GetAssociativeContainerInterface();
                }
                else if (classElement)
                {
                    AZStd::string_view elementName = classElement->m_name;
                    if (!elementName.empty())
                    {
                        path.append("/");
                        path.append(elementName);
                    }
                }

                //! Iterate over all found UIElements, insert a UIElement before this element if this element's parent is also the UIElement's parent,
                //! and this child element's index is the same as the UIElement's insertion index, and this element is going to be visible in the component
                for (auto iter = m_UIElements.begin(); iter != m_UIElements.end(); ++iter)
                {
                    if (iter->m_classData->m_name == parentData.m_classData->m_name && iter->m_insertIndex == parentData.m_childElementIndex && classElement && classElement->m_editData)
                    {
                        m_stack.push_back(m_UIElements.back());
                        CacheAttributes();
                        m_stack.back().m_entryClosed = true;
                        m_visitor->VisitObjectBegin(*this, *this);
                        m_visitor->VisitObjectEnd(*this, *this);
                        m_stack.pop_back();
                    }
                }

                // Search through classData for UIElements, indexing elements so we know where in the hierarchy to insert each UIElement
                if (classData->m_editData)
                {
                    int insertIndex = 0;
                    bool addedElement = false;

                    for (auto eltIt = classData->m_editData->m_elements.begin(); eltIt != classData->m_editData->m_elements.end(); ++eltIt)
                    {
                        if (eltIt->m_serializeClassElement)
                        {
                            insertIndex++;
                        }
                        if (eltIt->m_elementId == AZ::Edit::ClassElements::UIElement)
                        {
                            AZ::SerializeContext::ClassElement* UIElement = new AZ::SerializeContext::ClassElement();
                            UIElement->m_editData = &*eltIt;
                            UIElement->m_flags = SerializeContext::ClassElement::Flags::FLG_UI_ELEMENT;
                            StackEntry entry = { parentData.m_instance, parentData.m_instance, classData->m_typeId, classData, UIElement };
                            entry.m_insertIndex = insertIndex;
                            m_UIElements.push_back(entry);
                            addedElement = true;
                        }
                    }
                    // Special case for classes that only have UI elements inside of them (like Component Mode)
                    if (insertIndex == 0 && addedElement)
                    {
                        for (auto& UIElement : m_UIElements)
                        {
                            m_stack.push_back(UIElement);
                            CacheAttributes();
                            m_stack.back().m_entryClosed = true;
                            m_visitor->VisitObjectBegin(*this, *this);
                            m_visitor->VisitObjectEnd(*this, *this);
                            m_stack.pop_back();
                        }
                    }
                }
                m_stack.push_back(
                    { instance, parentData.m_instance, classData ? classData->m_typeId : Uuid::CreateNull(), classData, classElement });
                StackEntry* nodeData = &m_stack.back();
                nodeData->m_path = AZStd::move(path);

                // If our parent node is disabled then we should inherit its disabled state
                if (parentData.m_disableEditor || parentData.m_isAncestorDisabled)
                {
                    nodeData->m_isAncestorDisabled = true;
                }
                else if (classElement && classElement->m_editData)
                {
                    if (auto readOnlyAttribute = classElement->m_editData->FindAttribute(AZ::Crc32("ReadOnly")); readOnlyAttribute)
                    {
                        Dom::Value readOnlyValue;
                        if (readOnlyAttribute->CanDomInvoke(Dom::Value(Dom::Type::Array)))
                        {
                            readOnlyValue = readOnlyAttribute->DomInvoke(instance, Dom::Value(Dom::Type::Array));
                        }
                        else
                        {
                            readOnlyValue = readOnlyAttribute->GetAsDomValue(instance);
                        }

                        if (readOnlyValue.IsBool())
                        {
                            nodeData->m_disableEditor |= readOnlyValue.GetBool();
                        }
                        else
                        {
                            AZ_Warning("LegacyReflectionBridge", false, "ReadOnly attribute yielded non-bool Value");
                        }
                    }
                }

                if (parentAssociativeInterface)
                {
                    if (nodeData->m_instance ==
                        parentAssociativeInterface->GetValueByKey(parentData.m_instance, nodeData->m_instance))
                    {
                        // the element *is* the key. This is some kind of set and has only one read-only value per entry
                        nodeData->m_skipLabel = true;
                        nodeData->m_disableEditor = true;

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
                        nodeData->m_extractKeyedPair = true;
                        nodeData->m_parentContainerInfo = parentData.m_classData->m_container;
                        nodeData->m_parentContainerOverride = parentData.m_instance;
                        nodeData->m_containerElementOverride = instance;
                        nodeData->m_entryClosed = true;
                        return true;
                    }
                }

                if (parentData.m_extractKeyedPair)
                {
                    // alternate behavoir for the pair children. The first is the key, stringify it
                    if (parentData.m_childElementIndex % 2 == 0)
                    {
                        // store the label override in the parent for the next child to consume
                        GetValueStringHelper(
                            nodeData->m_instance,
                            nodeData->m_classData,
                            nodeData->m_classElement,
                            m_serializeContext,
                            parentData.m_labelOverride);
                        nodeData->m_entryClosed = true;
                        return true;
                    }
                    else // the second is the value, make an editor as normal
                    {
                        // this is the second pair<> child, consume the label override stored above
                        nodeData->m_labelOverride = parentData.m_labelOverride;
                    }
                }

                if (classElement && classElement->m_editData &&
                    (classElement->m_flags & SerializeContext::ClassElement::Flags::FLG_UI_ELEMENT) != 0)
                {
                    // Don't visit EditorData, just poll it for attributes
                    if (classElement->m_editData->m_elementId == AZ::Edit::ClassElements::EditorData)
                    {
                        nodeData->m_entryClosed = true;
                        return true;
                    }
                    else if (classElement->m_editData->m_elementId == AZ::Edit::ClassElements::Group)
                    {
                        AZStd::string_view groupName = classElement->m_editData->m_description;
                        if (!parentData.m_group.empty())
                        {
                            // If we're superceding a previous group, call EndNode to ensure VisitObjectEnd gets called correctly, then put
                            // our group back on top of the stack
                            StackEntry nodeDataCopy = AZStd::move(m_stack.back());
                            m_stack.pop_back();
                            parentData.m_group = {};
                            parentData.m_entryClosed = false;
                            EndNode();
                            m_stack.push_back(AZStd::move(nodeDataCopy));
                            nodeData = &m_stack.back();
                        }

                        if (!groupName.empty())
                        {
                            // If our group has a name, we should actually call VisitObjectEnd, so continue with the group set
                            nodeData->m_group = groupName;
                        }
                        else
                        {
                            // Otherwise, just leave a placeholder entry to be popped off the stack in EndNode and bail, there's no actual
                            // group to visit
                            nodeData->m_entryClosed = true;
                            return true;
                        }
                    }
                }

                CacheAttributes();

                // Inherit the change notify attribute from our parent
                const Name changeNotify = Name("ChangeNotify");
                Dom::Value changeNotifyValue = Find(changeNotify);
                if (changeNotifyValue.IsNull())
                {
                    changeNotifyValue = Find(Name(), changeNotify, parentData);
                    if (!changeNotifyValue.IsNull())
                    {
                        nodeData->m_cachedAttributes.push_back({ Name(), changeNotify, changeNotifyValue });
                    }
                }

                const auto& EnumTypeAttribute = DocumentPropertyEditor::Nodes::PropertyEditor::EnumUnderlyingType;
                Dom::Value enumTypeValue = Find(EnumTypeAttribute.GetName());
                auto enumTypeId = EnumTypeAttribute.DomToValue(enumTypeValue);

                const AZ::TypeId* typeIdForHandler = &nodeData->m_typeId;
                if (enumTypeId.has_value())
                {
                    typeIdForHandler = &enumTypeId.value();
                }
                if (auto handlerIt = m_handlers.find(*typeIdForHandler); handlerIt != m_handlers.end())
                {
                    nodeData->m_entryClosed = true;
                    return handlerIt->second();
                }

                m_visitor->VisitObjectBegin(*this, *this);

                return true;
            }

            bool EndNode()
            {
                StackEntry& nodeData = m_stack.back();
                if (!nodeData.m_group.empty())
                {
                    // For groups, we'll get an EndNode call but the group should remain open
                    // until its parent is actually closed (or another group is specified)
                    // so we just set m_entryClosed the first time, then finish visiting both
                    // the group and its parent when its parent would be closed.
                    if (nodeData.m_entryClosed)
                    {
                        m_visitor->VisitObjectEnd(*this, *this);
                        m_stack.pop_back();
                        EndNode();
                    }
                    else
                    {
                        nodeData.m_entryClosed = true;
                    }
                    return true;
                }
                else if (!nodeData.m_entryClosed)
                {
                    m_visitor->VisitObjectEnd(*this, *this);
                }
                m_stack.pop_back();
                if (!m_stack.empty() && nodeData.m_computedVisibility == DocumentPropertyEditor::Nodes::PropertyVisibility::Show)
                {
                    ++m_stack.back().m_childElementIndex;
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
                PropertyVisibility visibility = PropertyVisibility::Show;

                AZ::Name handlerName;

                auto checkAttribute = [&](const AZ::AttributePair* it, void* instance, bool shouldDescribeChildren)
                {
                    if (it->second->m_describesChildren != shouldDescribeChildren)
                    {
                        return;
                    }

                    AZ::Name name = propertyEditorSystem->LookupNameFromId(it->first);
                    if (!name.IsEmpty())
                    {
                        // If a more specific attribute is already loaded, ignore the new value
                        if (visitedAttributes.find(name) != visitedAttributes.end())
                        {
                            return;
                        }
                        visitedAttributes.insert(name);

                        // Handle visibility calculations internally, as we calculate and emit an aggregate visiblity value.
                        if (name == PropertyEditor::Visibility.GetName())
                        {
                            visibility = PropertyEditor::Visibility
                                             .DomToValue(PropertyEditor::Visibility.LegacyAttributeToDomValue(instance, it->second))
                                             .value_or(visibility);
                        }

                        // See if any registered attributes can read this attribute.
                        Dom::Value attributeValue;
                        propertyEditorSystem->EnumerateRegisteredAttributes(
                            name,
                            [&](const AZ::DocumentPropertyEditor::AttributeDefinitionInterface& attributeReader)
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
                                handlerName = AZ::Name();
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
                                if (elementEditData->m_elementId)
                                {
                                    handlerName = propertyEditorSystem->LookupNameFromId(elementEditData->m_elementId);
                                }

                                if (elementEditData->m_name)
                                {
                                    labelAttributeValue = elementEditData->m_name;
                                }
                                if (elementEditData->m_description)
                                {
                                    descriptionAttributeValue = elementEditData->m_description;
                                }
                            }

                            for (auto it = elementEditData->m_attributes.begin(); it != elementEditData->m_attributes.end(); ++it)
                            {
                                checkAttribute(it, nodeData.m_parentInstance, isParentAttribute);
                            }
                        }

                        if (labelAttributeValue.empty() && nodeData.m_classElement->m_name)
                        {
                            labelAttributeValue = nodeData.m_classElement->m_name;
                        }

                        for (auto it = nodeData.m_classElement->m_attributes.begin(); it != nodeData.m_classElement->m_attributes.end();
                             ++it)
                        {
                            AZ::AttributePair pair;
                            pair.first = it->first;
                            pair.second = it->second.get();
                            checkAttribute(&pair, nodeData.m_parentInstance, isParentAttribute);
                        }
                    }

                    if (nodeData.m_classData)
                    {
                        if (!isParentAttribute && labelAttributeValue.empty() && nodeData.m_classData->m_name)
                        {
                            // Don't inject labels from class data for UI elements
                            if (nodeData.m_classElement == nullptr ||
                                (nodeData.m_classElement->m_flags & SerializeContext::ClassElement::Flags::FLG_UI_ELEMENT) == 0)
                            {
                                labelAttributeValue = nodeData.m_classData->m_name;
                            }
                        }

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

                        if (nodeData.m_labelOverride.empty())
                        {
                            labelAttributeBuffer = decltype(labelAttributeBuffer)::format("[%zu]", parentNode.m_childElementIndex);
                            labelAttributeValue = labelAttributeBuffer;
                        }
                    }
                }

                if (!nodeData.m_labelOverride.empty())
                {
                    labelAttributeValue = nodeData.m_labelOverride;
                }
                else if (!nodeData.m_group.empty())
                {
                    labelAttributeValue = nodeData.m_group;
                }

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
                        { group, AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Disabled.GetName(), Dom::Value(true) });
                }

                if (nodeData.m_isAncestorDisabled)
                {
                    nodeData.m_cachedAttributes.push_back(
                        { group, AZ::DocumentPropertyEditor::Nodes::PropertyEditor::AncestorDisabled.GetName(), Dom::Value(true) });
                }

                if (nodeData.m_classData->m_container)
                {
                    nodeData.m_cachedAttributes.push_back(
                        { group, DescriptorAttributes::Container, Dom::Utils::ValueFromType<void*>(nodeData.m_classData->m_container) });
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
                if (nodeData.m_classElement && !nodeData.m_classElement->m_editData)
                {
                    visibility = PropertyVisibility::ShowChildrenOnly;
                }
                nodeData.m_computedVisibility = visibility;
                nodeData.m_cachedAttributes.push_back(
                    { group, PropertyEditor::Visibility.GetName(), Dom::Utils::ValueFromType(visibility) });
            }

            AttributeDataType Find(Name name) const override
            {
                return Find(Name(), AZStd::move(name));
            }

            AttributeDataType Find(Name group, Name name) const override
            {
                const StackEntry& nodeData = m_stack.back();
                for (auto it = nodeData.m_cachedAttributes.begin(); it != nodeData.m_cachedAttributes.end(); ++it)
                {
                    if (it->m_group == group && it->m_name == name)
                    {
                        return it->m_value;
                    }
                }
                return Dom::Value();
            }

            AttributeDataType Find(Name group, Name name, StackEntry& parentData) const
            {
                for (auto it = parentData.m_cachedAttributes.begin(); it != parentData.m_cachedAttributes.end(); ++it)
                {
                    if (it->m_group == group && it->m_name == name)
                    {
                        return it->m_value;
                    }
                }
                return Dom::Value();
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
