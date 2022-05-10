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
            };
            AZStd::stack<StackEntry> m_stack;

            using HandlerCallback = AZStd::function<bool()>;
            AZStd::unordered_map<AZ::TypeId, HandlerCallback> m_handlers;

            virtual ~InstanceVisitor() = default;

            InstanceVisitor(IReadWrite* visitor, void* instance, const AZ::TypeId& typeId, SerializeContext* serializeContext)
                : m_visitor(visitor)
                , m_serializeContext(serializeContext)
            {
                m_stack.push({ instance, typeId });
                RegisterPrimitiveHandlers<bool, AZ::u8, AZ::u16, AZ::u32, AZ::u64, AZ::s8, AZ::s16, AZ::s32, AZ::s64, float, double>();
            }

            template<typename T>
            void RegisterHandler(AZStd::function<bool(T&)> handler)
            {
                m_handlers[azrtti_typeid<T>()] = [this, handler = AZStd::move(handler)]() -> bool
                {
                    return handler(*reinterpret_cast<T*>(m_stack.top().m_instance));
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
                SerializeContext::EnumerateInstanceCallContext context(
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
                    m_serializeContext, SerializeContext::EnumerationAccessFlags::ENUM_ACCESS_FOR_WRITE, nullptr);

                const StackEntry& nodeData = m_stack.top();
                m_serializeContext->EnumerateInstance(&context, nodeData.m_instance, nodeData.m_typeId, nullptr, nullptr);
            }

            bool BeginNode(
                void* instance, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* classElement)
            {
                AZStd::string path = m_stack.top().m_path;
                if (classElement)
                {
                    path.append("/");
                    path.append(classElement->m_name);
                }
                m_stack.push({ instance, classData ? classData->m_typeId : Uuid::CreateNull(), classData, classElement });
                m_stack.top().m_path = AZStd::move(path);
                CacheAttributes();

                StackEntry& nodeData = m_stack.top();

                if (auto handlerIt = m_handlers.find(nodeData.m_typeId); handlerIt != m_handlers.end())
                {
                    return handlerIt->second();
                }
                m_visitor->VisitObjectBegin(*this, *this);

                return true;
            }

            bool EndNode()
            {
                StackEntry nodeData = AZStd::move(m_stack.top());
                m_stack.pop();
                if (auto handlerIt = m_handlers.find(nodeData.m_typeId); handlerIt != m_handlers.end())
                {
                    return true;
                }

                m_visitor->VisitObjectEnd();
                return true;
            }

            const AZ::TypeId& GetType() const override
            {
                return m_stack.top().m_typeId;
            }

            AZStd::string_view GetTypeName() const override
            {
                const AZ::SerializeContext::ClassData* classData = m_serializeContext->FindClassData(m_stack.top().m_typeId);
                return classData ? classData->m_name : "<unregistered type>";
            }

            void* Get() override
            {
                return m_stack.top().m_instance;
            }

            const void* Get() const override
            {
                return m_stack.top().m_instance;
            }

            template<typename T>
            bool TryReadAttributeInternal(void* instance, AZ::Attribute* attribute, Dom::Value& result) const
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
            bool TryReadAttribute(void* instance, AZ::Attribute* attribute, Dom::Value& result) const
            {
                return (TryReadAttributeInternal<T>(instance, attribute, result) || ...);
            }

            void CacheAttributes()
            {
                StackEntry& nodeData = m_stack.top();
                AZStd::vector<AttributeData>& cachedAttributes = nodeData.m_cachedAttributes;
                cachedAttributes.clear();

                // Legacy reflection doesn't have groups, so they're in the root "" group
                const Name group;
                AZStd::unordered_set<Name> visitedAttributes;

                AZStd::string_view labelAttributeValue;

                DocumentPropertyEditor::PropertyEditorSystemInterface* propertyEditorSystem =
                    AZ::Interface<DocumentPropertyEditor::PropertyEditorSystemInterface>::Get();
                AZ_Assert(propertyEditorSystem != nullptr, "LegacyReflectionBridge: Unable to retrieve PropertyEditorSystem");

                auto checkAttribute = [&](const AZ::AttributePair* it)
                {
                    AZ::Name name = propertyEditorSystem->LookupNameFromId(it->first);
                    if (!name.IsEmpty())
                    {
                        // If a more specific attribute is already loaded, ignore the new value
                        if (visitedAttributes.find(name) != visitedAttributes.end())
                        {
                            return;
                        }
                        visitedAttributes.insert(name);
                        Dom::Value attributeValue;
                        const bool readSucceeded = TryReadAttribute<
                            bool, AZ::u8, AZ::u16, AZ::u32, AZ::u64, AZ::s8, AZ::s16, AZ::s32, AZ::s64, AZStd::string, float, double>(
                            nodeData.m_instance, it->second, attributeValue);
                        if (readSucceeded)
                        {
                            cachedAttributes.push_back({ group, AZStd::move(name), AZStd::move(attributeValue) });
                        }
                    }
                    else
                    {
                        AZ_Warning("DPE", false, "Unable to lookup name for attribute CRC: %" PRId32, it->first);
                    }
                };

                nodeData.m_cachedAttributes.push_back({ group, DescriptorAttributes::SerializedPath, Dom::Value(nodeData.m_path, true) });

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
                            AZ::Name handlerName = propertyEditorSystem->LookupNameFromId(elementEditData->m_elementId);
                            if (!handlerName.IsEmpty())
                            {
                                nodeData.m_cachedAttributes.push_back(
                                    { group, DescriptorAttributes::Handler, Dom::Value(handlerName.GetCStr(), false) });
                            }
                        }

                        if (elementEditData->m_name)
                        {
                            labelAttributeValue = elementEditData->m_name;
                        }

                        for (auto it = elementEditData->m_attributes.begin(); it != elementEditData->m_attributes.end(); ++it)
                        {
                            checkAttribute(it);
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
                        checkAttribute(&pair);
                    }
                }

                if (nodeData.m_classData)
                {
                    if (labelAttributeValue.empty() && nodeData.m_classData->m_name)
                    {
                        labelAttributeValue = nodeData.m_classData->m_name;
                    }

                    for (auto it = nodeData.m_classData->m_attributes.begin(); it != nodeData.m_classData->m_attributes.end(); ++it)
                    {
                        AZ::AttributePair pair;
                        pair.first = it->first;
                        pair.second = it->second.get();
                        checkAttribute(&pair);
                    }
                }

                nodeData.m_cachedAttributes.push_back({ group, AZ_NAME_LITERAL("Label"), Dom::Value(labelAttributeValue, true) });
            }

            AttributeDataType Find(Name name) const override
            {
                return Find(Name(), AZStd::move(name));
            }

            AttributeDataType Find(Name group, Name name) const override
            {
                const StackEntry& nodeData = m_stack.top();
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
                const StackEntry& nodeData = m_stack.top();
                for (const AttributeData& data : nodeData.m_cachedAttributes)
                {
                    callback(data.m_group, data.m_name, data.m_value);
                }
            }
        };
    } // namespace LegacyReflectionInternal

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
