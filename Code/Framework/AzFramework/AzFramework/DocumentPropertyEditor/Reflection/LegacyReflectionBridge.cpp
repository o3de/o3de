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
#include <AzCore/Serialization/DynamicSerializableField.h>
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
        const Name ContainerIndex = Name::FromStringLiteral("ContainerIndex", AZ::Interface<AZ::NameDictionary>::Get());
        const Name ParentInstance = Name::FromStringLiteral("ParentInstance", AZ::Interface<AZ::NameDictionary>::Get());
        const Name ParentClassData = Name::FromStringLiteral("ParentClassData", AZ::Interface<AZ::NameDictionary>::Get());
    } // namespace DescriptorAttributes

    // attempts to stringify the passed instance; useful for things like maps where the key element should not be user editable
    AZStd::optional<AZStd::string> StringifyInstance(
        AZ::PointerObject instanceObject,
        const AZ::SerializeContext::ClassData* classData,
        const AZ::SerializeContext::ClassElement* classElement,
        AZ::SerializeContext* serializeContext)
    {
        void* instance = instanceObject.m_address;
        if (!classData)
        {
            return AZStd::nullopt;
        }
        if (serializeContext == nullptr)
        {
            if (auto componentApplicationRequests = AZ::Interface<AZ::ComponentApplicationRequests>::Get();
                componentApplicationRequests != nullptr)
            {
                serializeContext = componentApplicationRequests->GetSerializeContext();
            }
            // If the serialize context is still nullptr, then return a null optional
            if (serializeContext == nullptr)
            {
                return AZStd::nullopt;
            }
        }

        // First check if the instance can be converted to a string using a ToString attribute
        if (AZ::Attribute* toStringAttribute =
                AZ::Utils::FindEditOrSerializeContextAttribute(AZ::Edit::Attributes::ToString, classData, classElement);
            toStringAttribute != nullptr)
        {
            AZ::AttributeReader toStringReader(instance, toStringAttribute);
            AZStd::string value;
            if (toStringReader.Read<AZStd::string>(value))
            {
                return value;
            }
        }

        // Next check if the instance class element is an Enum type
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
            AZ::Uuid underlyingTypeId;
            extractUuidProperty(classData->m_attributes, Name("EnumUnderlyingType"), underlyingTypeId);
            // got the enumID, now get the enum's underlying type
            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                const AZ::Edit::ElementData* enumElementData = editContext->GetEnumElementData(enumId);
                if (enumElementData)
                {
                    AZStd::string value;
                    // Check all underlying enum storage types for the actual representation type to query
                    if (!underlyingTypeId.IsNull() &&
                        AZ::Utils::GetEnumStringRepresentation<AZ::u8, AZ::u16, AZ::u32, AZ::u64, AZ::s8, AZ::s16, AZ::s32, AZ::s64>(
                            value, enumElementData, instance, underlyingTypeId))
                    {
                        return value;
                    }
                }
            }

            // As a fall back use the Serialize Context Enum reflection option name for the string value

            auto enumClassData = serializeContext->FindClassData(enumId);
            auto underlyingIntClassData = serializeContext->FindClassData(underlyingTypeId);
            if (enumClassData != nullptr && underlyingIntClassData != nullptr && underlyingIntClassData->m_azRtti != nullptr)
            {
                const TypeTraits underlyingTypeTraits = underlyingIntClassData->m_azRtti->GetTypeTraits();
                const bool isSigned = (underlyingTypeTraits & TypeTraits::is_signed) == TypeTraits::is_signed;
                const bool isUnsigned = (underlyingTypeTraits & TypeTraits::is_unsigned) == TypeTraits::is_unsigned;

                AZStd::unordered_map<int64_t, AZStd::string_view> enumConstantsSigned;
                AZStd::unordered_map<uint64_t, AZStd::string_view> enumConstantsUnsigned;
                // Get each Enum Value -> Name pair
                for (const AZ::AttributeSharedPair& enumAttributePair : enumClassData->m_attributes)
                {
                    if (enumAttributePair.first == AZ::Serialize::Attributes::EnumValueKey)
                    {
                        auto enumConstantAttribute = azrtti_cast<AZ::AttributeData<SerializeContextEnumInternal::EnumConstantBasePtr>*>(
                            enumAttributePair.second.get());
                        if (enumConstantAttribute == nullptr)
                        {
                            continue;
                        }

                        const SerializeContextEnumInternal::EnumConstantBasePtr& enumConstantPtr = enumConstantAttribute->Get(nullptr);
                        if (isSigned)
                        {
                            enumConstantsSigned.emplace(enumConstantPtr->GetEnumValueAsInt(), enumConstantPtr->GetEnumValueName());
                        }
                        else if (isUnsigned)
                        {
                            enumConstantsUnsigned.emplace(enumConstantPtr->GetEnumValueAsUInt(), enumConstantPtr->GetEnumValueName());
                        }
                    }
                }

                AZStd::string value;
                auto GetEnumOptionNameSigned = [&value, &enumConstantsSigned](auto signedValue) -> bool
                {
                    auto signedValue64 = static_cast<int64_t>(signedValue);
                    if (auto foundIt = enumConstantsSigned.find(signedValue64); foundIt != enumConstantsSigned.end())
                    {
                        value = foundIt->second;
                        return true;
                    }

                    return false;
                };
                auto GetEnumOptionNameUnsigned = [&value, &enumConstantsUnsigned](auto unsignedValue) -> bool
                {
                    auto unsignedValue64 = static_cast<uint64_t>(unsignedValue);
                    if (auto foundIt = enumConstantsUnsigned.find(unsignedValue64); foundIt != enumConstantsUnsigned.end())
                    {
                        value = foundIt->second;
                        return true;
                    }

                    return false;
                };
                // Start checking the 32-bit int and unsigned int types first since they are the more likely to be the underlying
                // types used for an enum
                // For example a scoped enum `enum class Foo` has a default underlying type of `int`
                // For an unscoped enum such as `enum Foo`, the underlying type is usually `int` or `unsigned int` depending on the values
                // of the enum
                if (underlyingTypeId == azrtti_typeid<AZ::s32>() && GetEnumOptionNameSigned(*static_cast<AZ::s32*>(instance)))
                {
                    return value;
                }
                else if (underlyingTypeId == azrtti_typeid<AZ::u32>() && GetEnumOptionNameUnsigned(*static_cast<AZ::u32*>(instance)))
                {
                    return value;
                }
                // Now check the 8-bit, 16-bit and 64-bit underlying types to see if an Enum uses one of them
                else if (underlyingTypeId == azrtti_typeid<AZ::s8>() && GetEnumOptionNameSigned(*static_cast<AZ::s8*>(instance)))
                {
                    return value;
                }
                else if (underlyingTypeId == azrtti_typeid<AZ::s16>() && GetEnumOptionNameSigned(*static_cast<AZ::s16*>(instance)))
                {
                    return value;
                }
                else if (underlyingTypeId == azrtti_typeid<long>() && GetEnumOptionNameSigned(*static_cast<long*>(instance)))
                {
                    return value;
                }
                else if (underlyingTypeId == azrtti_typeid<AZ::s64>() && GetEnumOptionNameSigned(*static_cast<AZ::s64*>(instance)))
                {
                    return value;
                }
                else if (underlyingTypeId == azrtti_typeid<AZ::u8>() && GetEnumOptionNameUnsigned(*static_cast<AZ::u8*>(instance)))
                {
                    return value;
                }
                else if (underlyingTypeId == azrtti_typeid<AZ::u16>() && GetEnumOptionNameUnsigned(*static_cast<AZ::u16*>(instance)))
                {
                    return value;
                }
                else if (
                    underlyingTypeId == azrtti_typeid<unsigned long>() && GetEnumOptionNameUnsigned(*static_cast<unsigned long*>(instance)))
                {
                    return value;
                }
                else if (underlyingTypeId == azrtti_typeid<AZ::u64>() && GetEnumOptionNameUnsigned(*static_cast<AZ::u64*>(instance)))
                {
                    return value;
                }
                // char is a special case
                // It is a distinct type from signed char(AZ::s8) and unsigned char(AZ::u8) and can be either signed or unsigned
                // so both lambdas are called to check if it is in either set
                if (underlyingTypeId == azrtti_typeid<char>() &&
                    (GetEnumOptionNameSigned(*static_cast<char*>(instance)) || GetEnumOptionNameUnsigned(*static_cast<char*>(instance))))
                {
                    return value;
                }
            }
        }

        // Just use our underlying AZStd::string if we're a string
        if (classData->m_typeId == azrtti_typeid<AZStd::string>())
        {
            AZStd::string value = *reinterpret_cast<AZStd::string*>(instance);
            return value;
        }

        // Fall back on using our serializer's DataToText
        if (classElement)
        {
            if (const auto& serializer = classData->m_serializer; serializer != nullptr)
            {
                AZStd::string value;
                AZ::IO::MemoryStream memStream(instance, 0, classElement->m_dataSize);
                AZ::IO::ByteContainerStream outStream(&value);
                serializer->DataToText(memStream, outStream, false);
                return value;
            }
        }

        return AZStd::nullopt;
    }

    namespace LegacyReflectionInternal
    {
        // Implement the KeyEntry is valid function
        bool KeyEntry::IsValid() const
        {
            return m_keyInstance.IsValid();
        }

        struct InstanceVisitor
            : IObjectAccess
            , IAttributes
        {
            IReadWrite* m_visitor;
            SerializeContext* m_serializeContext;
            SerializeContext::EnumerateInstanceCallContext* m_enumerateContext;

            struct StackEntry
            {
                StackEntry() = default;

                StackEntry(
                    AZ::PointerObject instance,
                    AZ::PointerObject instanceToInvoke,
                    AZ::u32 pointerLevel,
                    const SerializeContext::ClassData* classData,
                    const SerializeContext::ClassElement* classElement,
                    const AZ::Edit::ElementData* elementEditData)
                    : m_instance(instance)
                    , m_instanceToInvoke(instanceToInvoke)
                    , m_pointerLevel(pointerLevel)
                    , m_classData(classData)
                    , m_classElement(classElement)
                    , m_elementEditData(elementEditData)
                {
                }

                AZ::PointerObject m_instance;
                // Instance that is used to retrieve attribute values that are tied to invokable functions.
                // Commonly, it is the parent instance for property nodes, and the instance for UI element nodes.
                AZ::PointerObject m_instanceToInvoke;

                //! Keeps tracks of how many pointers are attached to the instance type
                //! For example if the actual instance being referenced is an AZ::Component, then m_pointerLevel is 0
                //! But if the instance being referenced is an AZ::Component*, then the m_pointerLevel is 1
                //! Finally if the instance being referenced is an AZ::Component**, then the m_pointerLevel is 2
                AZ::u32 m_pointerLevel = 0;

                //! Forces this node to be hidden and to show just the children.
                bool m_showChildrenOnly = false;

                const SerializeContext::ClassData* m_classData = nullptr;
                const SerializeContext::ClassElement* m_classElement = nullptr;
                const AZ::Edit::ElementData* m_elementEditData = nullptr;
                AZStd::vector<AttributeData> m_cachedAttributes;
                AZStd::string m_path;
                DocumentPropertyEditor::Nodes::PropertyVisibility m_computedVisibility =
                    DocumentPropertyEditor::Nodes::PropertyVisibility::Show;
                bool m_entryClosed = false;
                size_t m_childElementIndex = 0;
                using AssociativeContainerType = Serialize::IDataContainer::IAssociativeDataContainer::AssociativeType;
                AssociativeContainerType m_associativeContainerType = AssociativeContainerType::Unknown;

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
                AZ::PointerObject m_parentContainerOverride;
                AZ::PointerObject m_containerElementOverride;
                //! For the pair<> element of an associative container
                //! this points to key instance and it's attributes
                KeyEntry m_childKeyEntry;
                //! Override for the mapped value of a pair<> element of an associative container
                AZStd::string m_childMappedValueLabelOverride;

                //! For the mapped_type of the pair<> element of an associative container,
                //! stores the key instance address and type id
                KeyEntry m_keyEntry;

                //! Returns an address that can be casted to the Type of the instance via a "reinterpret_cast<Type*>" cast
                //! If the instance is storing a pointer to a Type*, that is actually a Type**
                //! So this code make sure that dereferences occurs before returning the address
                const void* GetRawInstance() const
                {
                    const void* directInstance = m_instance.m_address;
                    for (AZ::u32 pointersToDereference = m_pointerLevel; pointersToDereference > 0; --pointersToDereference)
                    {
                        directInstance = *reinterpret_cast<const void* const*>(directInstance);
                    }
                    return directInstance;
                }

                void* GetRawInstance()
                {
                    return const_cast<void*>(AZStd::as_const(*this).GetRawInstance());
                }

                AZ::TypeId GetTypeId() const
                {
                    return m_instance.m_typeId;
                }

                AZStd::vector<AZStd::pair<AZStd::string, AZStd::optional<StackEntry>>> m_groups;
                AZStd::map<AZStd::string, AZStd::vector<StackEntry>> m_groupEntries;
                AZStd::map<AZStd::string, AZStd::string> m_propertyToGroupMap;
                AZStd::vector<AZStd::pair<AZStd::string, StackEntry>> m_nonSerializedChildren;
            };
            AZStd::deque<StackEntry> m_stack;

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
                m_stack.emplace_back();
                m_stack.back().m_instance = AZ::PointerObject{ instance, typeId };

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
                    return handler(*reinterpret_cast<T*>(Get()));
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
                m_serializeContext->EnumerateInstanceConst(
                    m_enumerateContext, nodeData.GetRawInstance(), nodeData.m_instance.m_typeId, nullptr, nullptr);
            }

            void GenerateNodePath(const StackEntry& parentData, StackEntry& nodeData)
            {
                AZStd::string path = parentData.m_path;
                if (parentData.m_classData && parentData.m_classData->m_container)
                {
                    path.append("/");
                    path.append(AZStd::string::format("%zu", parentData.m_childElementIndex));
                }
                // If we have a class element, we skip appending to the SerializedPath if its a base class
                // because base class information is ignored by the JSON serializer in JsonSerializer::StoreWithClassElement.
                // The name for these look like "BaseClass1", "BaseClass2", etc... are defined in c_serializeBaseClassStrings
                // and won't be present once serialized so if we don't ignore them then certain logic that attempts to
                // match against the SerializedPath won't be accurate.
                else if (
                    nodeData.m_classElement &&
                    ((nodeData.m_classElement->m_flags & AZ::SerializeContext::ClassElement::FLG_BASE_CLASS) == 0))
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
                                    const AZ::Serialize::ClassElement* serializeClassElement = currElement.m_serializeClassElement;
                                    void* boolAddress = reinterpret_cast<void*>(
                                        reinterpret_cast<size_t>(nodeData.GetRawInstance()) + serializeClassElement->m_offset);
                                    boolAddress = AZ::Utils::ResolvePointer(boolAddress, *serializeClassElement, *m_serializeContext);

                                    constexpr AZ::u32 pointerLevel = 0;

                                    StackEntry entry(
                                        AZ::PointerObject{ boolAddress, serializeClassElement->m_typeId },
                                        nodeData.m_instance,
                                        pointerLevel,
                                        nullptr,
                                        currElement.m_serializeClassElement,
                                        currElement.m_serializeClassElement->m_editData);

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
                                    // A UI node isn't backed with a real C++ type, so its pointer level is 0
                                    constexpr AZ::u32 pointerLevel = 0;
                                    StackEntry entry(
                                        nodeData.m_instance,
                                        nodeData.m_instance,
                                        pointerLevel,
                                        nodeData.m_classData,
                                        UIElement,
                                        UIElement->m_editData);

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

                                    // Create a dummy group entry that will be used to correctly display all children properties and UI
                                    // elements.
                                    AZ::SerializeContext::ClassElement* UIElement = new AZ::SerializeContext::ClassElement();
                                    UIElement->m_editData = &currElement;
                                    UIElement->m_flags = SerializeContext::ClassElement::Flags::FLG_UI_ELEMENT;
                                    // A group is a UI node that isn't backed with a real C++ type, so its pointer level is 0.
                                    constexpr AZ::u32 pointerLevel = 0;
                                    StackEntry entry(
                                        nodeData.m_instance,
                                        nodeData.m_instance,
                                        pointerLevel,
                                        nodeData.m_classData,
                                        UIElement,
                                        UIElement->m_editData);
                                    // Hide this dummy group and only show the children.
                                    entry.m_showChildrenOnly = true;

                                    nodeData.m_groups.emplace_back(AZStd::make_pair(groupName, AZStd::move(entry)));
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

            void HandleNodeUiElementsRetrieval(StackEntry& nodeData)
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
                            lastValidElementName = "";
                        }
                        else if (iter->m_elementId == AZ::Edit::ClassElements::UIElement)
                        {
                            AZ::SerializeContext::ClassElement* UIElement = new AZ::SerializeContext::ClassElement();
                            UIElement->m_editData = &*iter;
                            UIElement->m_flags = SerializeContext::ClassElement::Flags::FLG_UI_ELEMENT;
                            // Use the instance itself to retrieve parameter values that invoke functions on the UI Element.
                            constexpr AZ::u32 pointerLevel = 0;
                            StackEntry entry(
                                nodeData.m_instance,
                                nodeData.m_instance,
                                pointerLevel,
                                nodeData.m_classData,
                                UIElement,
                                UIElement->m_editData);

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

                            nodeData.m_nonSerializedChildren.emplace_back(AZStd::move(pathString), AZStd::move(entry));
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
                // find the last non-ui element in the stack -- it may hold UI elements to create at this path
                StackEntry* nonUIAncestor = nullptr;
                for (auto stackIter = m_stack.rbegin(), endIter = m_stack.rend(); !nonUIAncestor && stackIter != endIter; ++stackIter)
                {
                    if (!stackIter->m_classElement ||
                        !(stackIter->m_classElement->m_flags & SerializeContext::ClassElement::Flags::FLG_UI_ELEMENT))
                    {
                        nonUIAncestor = &*stackIter;
                    }
                }
                if (!nonUIAncestor)
                {
                    return;
                }

                // Iterate over non serialized elements to see if any of them should be added
                auto iter = nonUIAncestor->m_nonSerializedChildren.begin();
                while (iter != nonUIAncestor->m_nonSerializedChildren.end())
                {
                    // If the parent of the element that was just created has the same name as the parent of any non serialized
                    // elements, and the element that was just created is the element immediately before any non serialized element,
                    // create that serialized element
                    if (path == iter->first)
                    {
                        m_stack.push_back(iter->second);
                        CacheAttributes();
                        m_stack.back().m_entryClosed = true;
                        m_visitor->VisitObjectBegin(*this, *this);
                        m_visitor->VisitObjectEnd(*this, *this);
                        m_stack.pop_back();

                        iter = nonUIAncestor->m_nonSerializedChildren.erase(iter);
                    }
                    else
                    {
                        ++iter;
                    }
                }
            }

            enum class VisitEntry
            {
                VisitChildrenCallVisitObjectBegin,
                VisitChildrenSkipVisitObjectBegin,
                VisitNextSiblingSkipVisitObjectBegin

            };
            VisitEntry HandleNodeAssociativeInterface(StackEntry& parentData, StackEntry& nodeData)
            {
                using AssociativeType = Serialize::IDataContainer::IAssociativeDataContainer::AssociativeType;
                if (parentData.m_classData && parentData.m_classData->m_container)
                {
                    auto& containerInfo = parentData.m_classData->m_container;
                    AZ::SerializeContext::IDataContainer::IAssociativeDataContainer* parentAssociativeInterface =
                        containerInfo->GetAssociativeContainerInterface();

                    if (parentAssociativeInterface != nullptr)
                    {
                        auto associativeType = parentAssociativeInterface->GetAssociativeType();
                        // Store the type of associative container
                        parentData.m_associativeContainerType = associativeType;
                        switch (associativeType)
                        {
                        case AssociativeType::Set:
                        case AssociativeType::UnorderedSet:
                            {
                                // the element *is* the key. This is some kind of set and has only one read-only value per entry
                                nodeData.m_skipLabel = true;
                                nodeData.m_disableEditor = true;
                                break;
                            }
                        case AssociativeType::Map:
                        case AssociativeType::UnorderedMap:
                            {
                                // parent is a map associative container with pair<> children,
                                // extract the info from the parent and just show label/editor pairs
                                nodeData.m_extractKeyedPair = true;
                                nodeData.m_parentContainerInfo = parentData.m_classData->m_container;
                                nodeData.m_parentContainerOverride = parentData.m_instance;
                                nodeData.m_containerElementOverride = nodeData.m_instance;
                            }
                        }
                    }
                }

                if (parentData.m_extractKeyedPair)
                {
                    // alternate behavior for the pair children. The first is the key, stringify it
                    if (parentData.m_childElementIndex % 2 == 0)
                    {
                        nodeData.m_entryClosed = true;
                        // Update the parent to store a pointer to the key instance data
                        parentData.m_childKeyEntry.m_keyInstance = nodeData.m_instance;

                        // Populate the attributes for the key instance into it's m_cachedAttribute
                        // field and then move it over into the parentData KeyEntry to cache off the value
                        // until the mapped type is processed in the else block below

                        // Make sure to have the key editor be disabled
                        nodeData.m_disableEditor = true;
                        // Store of the stringified value of the key into label override for the mapped value
                        if (auto stringKey =
                                StringifyInstance(nodeData.m_instance, nodeData.m_classData, nodeData.m_classElement, m_serializeContext);
                            stringKey)
                        {
                            parentData.m_childMappedValueLabelOverride = AZStd::move(*stringKey);
                        }

                        // Do not enumerate the key elements itself
                        // Instead let the mapped type in the else block add a DOM Editor for the key
                        return VisitEntry::VisitNextSiblingSkipVisitObjectBegin;
                    }
                    else // the second is the value, make an editor as normal
                    {
                        // swap the current child key instance stored in the parent with the node key instance field
                        // and then clear the parent current child key instance field
                        nodeData.m_keyEntry = AZStd::move(parentData.m_childKeyEntry);
                        parentData.m_childKeyEntry = {};

                        // Apply the stringified key as the label for the value
                        nodeData.m_labelOverride = AZStd::move(parentData.m_childMappedValueLabelOverride);
                        parentData.m_childMappedValueLabelOverride = AZStd::string{};
                    }
                }

                return VisitEntry::VisitChildrenCallVisitObjectBegin;
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
                InheritChangeNotify(parentData, nodeData);

                const auto& EnumTypeAttribute = DocumentPropertyEditor::Nodes::PropertyEditor::EnumUnderlyingType;
                AZStd::optional<AZ::TypeId> enumTypeId;
                if (auto enumTypeValue = Find(EnumTypeAttribute.GetName()); enumTypeValue)
                {
                    enumTypeId = EnumTypeAttribute.DomToValue(*enumTypeValue);
                }

                const AZ::TypeId* typeIdForHandler = &nodeData.m_instance.m_typeId;
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
                // Prepare the node references for the handlers.
                StackEntry& parentData = m_stack.back();

                const AZ::Edit::ElementData* elementEditData = (classElement ? classElement->m_editData : nullptr);

                if (classElement)
                {
                    // Ensure our instance pointer is resolved and safe to bind to member attributes.
                    instance = AZ::Utils::ResolvePointer(instance, *classElement, *m_serializeContext);

                    // Since we iterate over the SerializeContext, we end up processing all elements even if they
                    // weren't exposed to the EditContext, so we need to skip any nodes that are missing m_editData and don't have an
                    // editDataProvider. There are caveats:
                    //   - We still need to process FLG_BASE_CLASS nodes because there could be sub-classes that are exposed to the
                    //   EditContext.
                    //   - Elements in a container will have empty m_editData, so we need to still include nodes who are members of
                    //   containers.

                    if (classData && !m_stack.empty())
                    {
                        for (auto rIter = m_stack.rbegin(), rEnd = m_stack.rend(); (rIter != rEnd && !elementEditData); ++rIter)
                        {
                            auto& ancestorData = *rIter;

                            if (ancestorData.m_classData && ancestorData.m_classData->m_editData &&
                                ancestorData.m_classData->m_editData->m_editDataProvider)
                            {
                                const AZ::Edit::ElementData* overrideData = ancestorData.m_classData->m_editData->m_editDataProvider(
                                    ancestorData.m_instance.m_address, instance, classData->m_typeId);
                                if (overrideData)
                                {
                                    elementEditData = overrideData;
                                }
                            }
                        }
                    }

                    if (!elementEditData && ((classElement->m_flags & AZ::SerializeContext::ClassElement::FLG_BASE_CLASS) == 0) &&
                        ((classElement->m_flags & AZ::SerializeContext::ClassElement::FLG_DYNAMIC_FIELD) == 0) &&
                        !(parentData.m_classData && parentData.m_classData->m_container))
                    {
                        m_nodeWasSkipped = true;
                        return false;
                    }
                }

                // Set the pointerLevel to 0 as the pointer is resolved by the AZ::Utils::ResolvePointer
                constexpr AZ::u32 pointerLevel = 0;

                // search up the stack for the "true parent", which is the last entry created by the serialize enumerate itself
                AZ::PointerObject instanceToInvokeObject{ instance, classData ? classData->m_typeId : Uuid::CreateNull() };
                for (auto rIter = m_stack.rbegin(), rEnd = m_stack.rend(); rIter != rEnd; ++rIter)
                {
                    auto* currInstance = rIter->GetRawInstance();
                    if (rIter->m_createdByEnumerate && currInstance)
                    {
                        instanceToInvokeObject = AZ::PointerObject{ currInstance, rIter->GetTypeId() };
                        break;
                    }
                }

                AZ::PointerObject instanceObject{ instance, classData ? classData->m_typeId : Uuid::CreateNull() };

                StackEntry& nodeData =
                    m_stack.emplace_back(instanceObject, instanceToInvokeObject, pointerLevel, classData, classElement, elementEditData);
                nodeData.m_createdByEnumerate = true;

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

                if (auto result = HandleNodeAssociativeInterface(parentData, nodeData);
                    result != VisitEntry::VisitChildrenCallVisitObjectBegin)
                {
                    return result == VisitEntry::VisitChildrenSkipVisitObjectBegin;
                }

                if (auto result = HandleNodeAttributes(parentData, nodeData); result.has_value())
                {
                    return result.value();
                }

                // handle direct descendant UI element creation
                HandleNodeUiElementsCreation(nodeData.m_path);
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

                    // Handle groups
                    if (nodeData.m_groups.size() > 0)
                    {
                        for (auto& groupPair : nodeData.m_groups)
                        {
                            if (groupPair.second.has_value())
                            {
                                auto& groupStackEntry = groupPair.second.value();
                                groupStackEntry.m_group = groupPair.first;

                                if (groupStackEntry.m_elementEditData->m_serializeClassElement)
                                {
                                    groupStackEntry.m_skipHandler = true;
                                }
                                auto& parentEntry = m_stack.back();
                                m_stack.push_back(groupStackEntry);
                                CacheAttributes();
                                InheritChangeNotify(parentEntry, m_stack.back());
                                m_visitor->VisitObjectBegin(*this, *this);
                            }

                            AZStd::string path = AZStd::string::format("%s/%s", nodeData.m_path.c_str(), groupPair.first.c_str());
                            HandleNodeUiElementsCreation(path);

                            for (const auto& groupEntry : nodeData.m_groupEntries[groupPair.first])
                            {
                                if (groupPair.second.has_value() &&
                                    groupPair.second.value().m_elementEditData->m_serializeClassElement == groupEntry.m_classElement)
                                {
                                    // skip the bool that represented the group toggle, it's already in-line with the group
                                    continue;
                                }
                                m_serializeContext->EnumerateInstance(
                                    m_enumerateContext,
                                    groupEntry.m_instance.m_address,
                                    groupEntry.m_instance.m_typeId,
                                    groupEntry.m_classData,
                                    groupEntry.m_classElement);
                            }

                            if (groupPair.second.has_value())
                            {
                                m_visitor->VisitObjectEnd(*this, *this);
                                m_stack.pop_back();
                            }
                        }

                        nodeData.m_propertyToGroupMap.clear();
                        nodeData.m_groupEntries.clear();
                    }

                    if (!nodeData.m_entryClosed)
                    {
                        m_visitor->VisitObjectEnd(*this, *this);
                    }

                    auto nodePath = nodeData.m_path;
                    m_stack.pop_back();

                    // handle creation of any UI elements that were slated to come directly after this element
                    HandleNodeUiElementsCreation(nodePath);
                }

                // The back of the stack now holds the parent.
                {
                    StackEntry& parentData = m_stack.back();

                    if (!m_stack.empty() && parentData.m_computedVisibility == DocumentPropertyEditor::Nodes::PropertyVisibility::Show)
                    {
                        ++m_stack.back().m_childElementIndex;
                    }
                }

                return true;
            }

            AZ::TypeId GetType() const override
            {
                return m_stack.back().GetTypeId();
            }

            AZStd::string_view GetTypeName() const override
            {
                const AZ::SerializeContext::ClassData* classData = m_serializeContext->FindClassData(m_stack.back().GetTypeId());
                return classData ? classData->m_name : "<unregistered type>";
            }

            // This returns a pointer to an instance that can be cast to a <TypeId>* for that instance
            // Even if the StackEntry::m_instance is referencing a <TypeId>**, a pointer is returned that can be casted to <TypeId>*
            void* Get() override
            {
                StackEntry& topEntry = m_stack.back();
                return topEntry.GetRawInstance();
            }

            const void* Get() const override
            {
                const StackEntry& topEntry = m_stack.back();
                return topEntry.GetRawInstance();
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
                    AZ::Serialize::IDataContainer* dataContainer = parentNode.m_classData ? parentNode.m_classData->m_container : nullptr;
                    if (dataContainer)
                    {
                        if (auto indexedChildOverride = Find(
                                AZ::Name(nodeData.m_group),
                                DocumentPropertyEditor::Nodes::Container::IndexedChildNameLabelOverride.GetName(),
                                parentNode);
                            indexedChildOverride)
                        {
                            auto retrievedName = DocumentPropertyEditor::Nodes::Container::IndexedChildNameLabelOverride.InvokeOnDomValue(
                                *indexedChildOverride, parentNode.m_childElementIndex);

                            if (retrievedName.IsSuccess())
                            {
                                labelAttributeBuffer = AZStd::fixed_string<128>::format("%s", retrievedName.GetValue().c_str());
                                return labelAttributeBuffer;
                            }
                        }
                        if (dataContainer->IsSequenceContainer())
                        {
                            // Only add a numeric label for sequence containers
                            labelAttributeBuffer = AZStd::fixed_string<128>::format("[%zu]", parentNode.m_childElementIndex);
                            return labelAttributeBuffer;
                        }
                    }
                }

                // No overrides, so check the element edit data, class data, and class element
                if (nodeData.m_elementEditData && nodeData.m_elementEditData->m_name)
                {
                    return nodeData.m_elementEditData->m_name;
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
                // Alternatively, hide if forced by the node's property.
                if ((m_stack.size() == 2 && m_visitFromRoot) || nodeData.m_showChildrenOnly)
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

                auto checkAttribute = [&](const AttributePair* it, AZ::PointerObject instance, bool shouldDescribeChildren)
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

                        // Handle visibility calculations internally, as we calculate and emit an aggregate visibility value.
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
                            else if (auto genericVisibility = ReadGenericAttributeToDomValue(instance, it->second))
                            {
                                // Fallback to generic read if LegacyAttributeToDomValue fails
                                visibility = PropertyEditor::Visibility.DomToValue(genericVisibility.value()).value();
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

                        auto readValue = [&](const DocumentPropertyEditor::AttributeDefinitionInterface& attributeReader)
                        {
                            if (attributeValue.IsNull())
                            {
                                attributeValue = attributeReader.LegacyAttributeToDomValue(instance, it->second);
                            }
                        };
                        propertyEditorSystem->EnumerateRegisteredAttributes(name, readValue);

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
                                nodeData.m_instance.m_typeId = AzTypeInfo<u64>::Uuid();
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
                    if (nodeData.m_elementEditData)
                    {
                        if (!isParentAttribute)
                        {
                            if (!nodeData.m_skipHandler && nodeData.m_elementEditData->m_elementId)
                            {
                                handlerName = propertyEditorSystem->LookupNameFromId(nodeData.m_elementEditData->m_elementId);
                            }

                            if (nodeData.m_elementEditData->m_description)
                            {
                                descriptionAttributeValue = nodeData.m_elementEditData->m_description;
                            }
                        }

                        for (auto it = nodeData.m_elementEditData->m_attributes.begin();
                             it != nodeData.m_elementEditData->m_attributes.end();
                             ++it)
                        {
                            checkAttribute(it, nodeData.m_instanceToInvoke, isParentAttribute);
                        }
                    }
                    if (nodeData.m_classElement)
                    {
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
                                        checkAttribute(it, nodeData.m_instance, isParentAttribute);
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
                        AZ::PointerObject parentDataContainerObject = { parentContainerInfo,
                                                                        azrtti_typeid<AZ::Serialize::IDataContainer>() };

                        nodeData.m_cachedAttributes.push_back(
                            { group, DescriptorAttributes::ParentContainer, Dom::Utils::ValueFromType(parentDataContainerObject) });

                        AZ::PointerObject parentContainerObject =
                            parentNode.m_parentContainerOverride.IsValid() ? parentNode.m_parentContainerOverride : parentNode.m_instance;

                        nodeData.m_cachedAttributes.push_back(
                            { group, DescriptorAttributes::ParentContainerInstance, Dom::Utils::ValueFromType(parentContainerObject) });

                        if (parentContainerInfo->IsSequenceContainer())
                        {
                            nodeData.m_cachedAttributes.push_back(
                                { group, DescriptorAttributes::ContainerIndex, AZ::Dom::Value(parentNode.m_childElementIndex) });
                        }

                        if (parentNode.m_containerElementOverride.IsValid())
                        {
                            nodeData.m_cachedAttributes.push_back({ group,
                                                                    DescriptorAttributes::ContainerElementOverride,
                                                                    Dom::Utils::ValueFromType(parentNode.m_containerElementOverride) });
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

                    // Check for edge case where the parent node has a parent container, but is set to ShowChildrenOnly
                    // which would result in the container element missing the 'Remove' button. This replicates the parent container
                    // info to its child node instead so that the 'Remove' button can still be shown.
                    auto parentContainer = Find(group, DescriptorAttributes::ParentContainer, parentNode);
                    if ((parentNode.m_computedVisibility == PropertyVisibility::ShowChildrenOnly) && parentContainer)
                    {
                        nodeData.m_cachedAttributes.push_back({ group, DescriptorAttributes::ParentContainer, *parentContainer });

                        const auto inheritedAttributes = { DescriptorAttributes::ParentContainerInstance,
                                                           DescriptorAttributes::ParentContainerCanBeModified,
                                                           PropertyEditor::NameLabelOverride.GetName(),
                                                           AZ::Reflection::DescriptorAttributes::ContainerIndex };

                        for (const auto& attributeName : inheritedAttributes)
                        {
                            auto inheritedAttribute = Find(group, attributeName, parentNode);
                            if (inheritedAttribute)
                            {
                                auto existingAttribute = Find(group, attributeName, nodeData);
                                if (existingAttribute)
                                {
                                    if (existingAttribute->IsNull() ||
                                        (existingAttribute->IsObject() && existingAttribute->ObjectEmpty()) ||
                                        (existingAttribute->IsString() && !existingAttribute->GetStringLength()))
                                    {
                                        // overwrite existing empty value
                                        *existingAttribute = *inheritedAttribute;
                                    }
                                    else
                                    {
                                        // do nothing! Do not overwrite existing non-empty values
                                    }
                                }
                                else
                                {
                                    nodeData.m_cachedAttributes.push_back({ group, attributeName, *inheritedAttribute });
                                }
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

                if (m_stack.size() >= 2)
                {
                    StackEntry& parentNode = m_stack[m_stack.size() - 2];
                    nodeData.m_cachedAttributes.push_back(
                        { group, DescriptorAttributes::ParentInstance, Dom::Utils::ValueFromType(parentNode.m_instance) });

                    AZ::PointerObject parentClassDataObject;
                    parentClassDataObject.m_address = const_cast<SerializeContext::ClassData*>(parentNode.m_classData);
                    parentClassDataObject.m_typeId = azrtti_typeid<const SerializeContext::ClassData*>();
                    nodeData.m_cachedAttributes.push_back(
                        { group, DescriptorAttributes::ParentClassData, Dom::Utils::ValueFromType(parentClassDataObject) });
                }

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
                                                        AZ::Dom::Utils::TypeIdToDomValue(nodeData.m_instance.m_typeId) });

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
                    AZ::PointerObject nodeContainerObject;
                    nodeContainerObject.m_address = nodeData.m_classData->m_container;
                    nodeContainerObject.m_typeId = azrtti_typeid<AZ::Serialize::IDataContainer>();
                    nodeData.m_cachedAttributes.push_back(
                        { group, DescriptorAttributes::Container, Dom::Utils::ValueFromType(nodeContainerObject) });
                }

                // RpePropertyHandlerWrapper would cache the parent info from which a wrapped handler may retrieve the parent instance.
                if (nodeData.m_instanceToInvoke.IsValid())
                {
                    nodeData.m_cachedAttributes.push_back(
                        { group, PropertyEditor::ParentValue.GetName(), Dom::Utils::ValueFromType(nodeData.m_instanceToInvoke) });
                }

                // Create an Attribute for storing the Key Value associated with this value if it is the mapped value of an associative
                // container https://en.cppreference.com/w/cpp/container/map
                if (nodeData.m_keyEntry.IsValid())
                {
                    nodeData.m_cachedAttributes.push_back(
                        { group, PropertyEditor::KeyValue.GetName(), Dom::Utils::ValueFromType(nodeData.m_keyEntry) });
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
                if (nodeData.m_classElement && !nodeData.m_elementEditData &&
                    (parentData->m_classData && !parentData->m_classData->m_container))
                {
                    visibility = DocumentPropertyEditor::Nodes::PropertyVisibility::ShowChildrenOnly;
                }

                nodeData.m_computedVisibility = visibility;
                nodeData.m_cachedAttributes.push_back(
                    { group, PropertyEditor::Visibility.GetName(), Dom::Utils::ValueFromType(nodeData.m_computedVisibility) });
            }

            void InheritChangeNotify(StackEntry& parentData, StackEntry& nodeData)
            {
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
                        it->m_value = newChangeNotifyValue;
                    }
                    else
                    {
                        // no existing changeNotify, so let's just inherit the parent's one
                        nodeData.m_cachedAttributes.push_back({ Name(), changeNotifyName, *parentValue });
                    }
                }
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

            const AttributeDataType* Find(Name group, Name name, const StackEntry& parentData) const
            {
                for (auto it = parentData.m_cachedAttributes.cbegin(); it != parentData.m_cachedAttributes.cend(); ++it)
                {
                    if (it->m_group == group && it->m_name == name)
                    {
                        return &(it->m_value);
                    }
                }
                return nullptr;
            }

            // non-const Find that can be used to overwrite existing attributes
            AttributeDataType* Find(Name group, Name name, StackEntry& parentData)
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
            return AZStd::make_shared<AZ::AttributeData<T>>(AZStd::move(value));
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

    AZStd::optional<AZ::Dom::Value> ReadGenericAttributeToDomValue(AZ::PointerObject instanceObject, AZ::Attribute* attribute)
    {
        AZ::Dom::Value result = attribute->GetAsDomValue(instanceObject);
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
            AZ_Assert(serializeContext != nullptr, "Unable to retrieve a SerializeContext");
        }

        LegacyReflectionInternal::InstanceVisitor helper(visitor, instance, typeId, serializeContext);
        helper.Visit();
    }
} // namespace AZ::Reflection
