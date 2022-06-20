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
#include <AzCore/Interface/Interface.h>
#include <AzCore/Name/Name.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorSystemInterface.h>
#include <AzFramework/DocumentPropertyEditor/Reflection/LegacyReflectionBridge.h>

namespace AZ::Reflection
{
    namespace DescriptorAttributes
    {
        const Name Handler = Name::FromStringLiteral("Handler");
        const Name Label = Name::FromStringLiteral("Label");
        const Name SerializedPath = Name::FromStringLiteral("SerializedPath");
    } // namespace DescriptorAttributes

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
                AZ::TypeId m_typeId;
                const SerializeContext::ClassData* m_classData = nullptr;
                const SerializeContext::ClassElement* m_classElement = nullptr;
                AZStd::vector<AttributeData> m_cachedAttributes;
                AZStd::string m_path;
                bool m_entryClosed = false;
            };
            AZStd::deque<StackEntry> m_stack;

            using HandlerCallback = AZStd::function<bool()>;
            AZStd::unordered_map<AZ::TypeId, HandlerCallback> m_handlers;

            virtual ~InstanceVisitor() = default;

            InstanceVisitor(IReadWrite* visitor, void* instance, const AZ::TypeId& typeId, SerializeContext* serializeContext)
                : m_visitor(visitor)
                , m_serializeContext(serializeContext)
            {
                m_stack.push_back({ instance, typeId });
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
                EditContext::EnumerateInstanceCallContext context(
                    [this](
                        void* instance, const AZ::SerializeContext::ClassData* classData,
                        const AZ::SerializeContext::ClassElement* classElement)
                    {
                        return BeginNode(instance, classData, classElement);
                    },
                    [this]()
                    {
                        return EndNode();
                    },
                    m_serializeContext->GetEditContext(), SerializeContext::EnumerationAccessFlags::ENUM_ACCESS_FOR_WRITE, nullptr);

                const StackEntry& nodeData = m_stack.back();
                m_serializeContext->GetEditContext()->EnumerateInstance(&context, nodeData.m_instance, nodeData.m_typeId, nullptr, nullptr);
            }

            bool BeginNode(
                void* instance, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* classElement)
            {
                AZStd::string path = m_stack.back().m_path;
                if (classElement)
                {
                    path.append("/");
                    path.append(classElement->m_name);
                }
                m_stack.push_back({ instance, classData ? classData->m_typeId : Uuid::CreateNull(), classData, classElement });
                m_stack.back().m_path = AZStd::move(path);
                CacheAttributes();

                const auto& EnumTypeAttribute = DocumentPropertyEditor::Nodes::PropertyEditor::EnumUnderlyingType;
                Dom::Value enumTypeValue = Find(EnumTypeAttribute.GetName());
                auto enumTypeId = EnumTypeAttribute.DomToValue(enumTypeValue);

                StackEntry& nodeData = m_stack.back();

                const AZ::TypeId* typeIdForHandler = &nodeData.m_typeId;
                if (enumTypeId.has_value())
                {
                    typeIdForHandler = &enumTypeId.value();
                }
                if (auto handlerIt = m_handlers.find(*typeIdForHandler); handlerIt != m_handlers.end())
                {
                    m_stack.back().m_entryClosed = true;
                    return handlerIt->second();
                }
                m_visitor->VisitObjectBegin(*this, *this);

                return true;
            }

            bool EndNode()
            {
                StackEntry nodeData = AZStd::move(m_stack.back());
                m_stack.pop_back();
                if (!nodeData.m_entryClosed)
                {
                    m_visitor->VisitObjectEnd();
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

                DocumentPropertyEditor::PropertyEditorSystemInterface* propertyEditorSystem =
                    AZ::Interface<DocumentPropertyEditor::PropertyEditorSystemInterface>::Get();
                AZ_Assert(propertyEditorSystem != nullptr, "LegacyReflectionBridge: Unable to retrieve PropertyEditorSystem");

                AZ::Name handlerName;

                auto checkAttribute = [&](const AZ::AttributePair* it, StackEntry& nodeData, bool shouldDescribeChildren)
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

                        // See if any registered attributes can read this attribute.
                        Dom::Value attributeValue;
                        propertyEditorSystem->EnumerateRegisteredAttributes(name, [&](const AZ::DocumentPropertyEditor::AttributeDefinitionInterface& attributeReader)
                            {
                                if (attributeValue.IsNull())
                                {
                                    attributeValue = attributeReader.LegacyAttributeToDomValue(nodeData.m_instance, it->second);
                                }
                            });

                        // Fall back on a generic read that handles primitives.
                        if (attributeValue.IsNull())
                        {
                            attributeValue = ReadGenericAttributeToDomValue(nodeData.m_instance, it->second).value_or(Dom::Value());
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
                        AZ_Warning("DPE", false, "Unable to lookup name for attribute CRC: %" PRId32, it->first);
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
                            if (elementEditData->m_elementId)
                            {
                                handlerName = propertyEditorSystem->LookupNameFromId(elementEditData->m_elementId);
                            }

                            if (elementEditData->m_name)
                            {
                                labelAttributeValue = elementEditData->m_name;
                            }

                            for (auto it = elementEditData->m_attributes.begin(); it != elementEditData->m_attributes.end(); ++it)
                            {
                                checkAttribute(it, nodeData, isParentAttribute);
                            }
                        }

                        if (labelAttributeValue.empty() && nodeData.m_classElement->m_name)
                        {
                            labelAttributeValue = nodeData.m_classElement->m_name;
                        }

                        for (auto it = nodeData.m_classElement->m_attributes.begin(); it != nodeData.m_classElement->m_attributes.end(); ++it)
                        {
                            AZ::AttributePair pair;
                            pair.first = it->first;
                            pair.second = it->second.get();
                            checkAttribute(&pair, nodeData, isParentAttribute);
                        }
                    }

                    if (nodeData.m_classData)
                    {
                        for (auto it = nodeData.m_classData->m_attributes.begin(); it != nodeData.m_classData->m_attributes.end(); ++it)
                        {
                            AZ::AttributePair pair;
                            pair.first = it->first;
                            pair.second = it->second.get();
                            checkAttribute(&pair, nodeData, isParentAttribute);
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
                }

                if (!handlerName.IsEmpty())
                {
                    nodeData.m_cachedAttributes.push_back(
                        { group, DescriptorAttributes::Handler, Dom::Value(handlerName.GetCStr(), false) });
                }
                nodeData.m_cachedAttributes.push_back({ group, DescriptorAttributes::SerializedPath, Dom::Value(nodeData.m_path, true) });
                if (!labelAttributeValue.empty())
                {
                    nodeData.m_cachedAttributes.push_back({ group, DescriptorAttributes::Label, Dom::Value(labelAttributeValue, false) });
                }
                nodeData.m_cachedAttributes.push_back({ group, AZ::DocumentPropertyEditor::Nodes::PropertyEditor::ValueType.GetName(),
                                                        AZ::Dom::Utils::TypeIdToDomValue(nodeData.m_typeId) });
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
