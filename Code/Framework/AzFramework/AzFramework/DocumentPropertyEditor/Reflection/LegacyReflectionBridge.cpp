/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Name/Name.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorSystemInterface.h>
#include <AzFramework/DocumentPropertyEditor/Reflection/LegacyReflectionBridge.h>

namespace AZ::Reflection
{
    namespace LegacyAttributes
    {
        Name Handler = Name::FromStringLiteral("Handler");
        Name Label = Name::FromStringLiteral("Label");
    }

    namespace LegacyReflectionInternal
    {
        struct InstanceVisitor
            : IObjectAccess
            , IAttributes
        {
            IReadWrite* m_visitor;
            void* m_instance;
            AZ::TypeId m_typeId;
            SerializeContext* m_serializeContext;
            const SerializeContext::ClassData* m_classData = nullptr;
            const SerializeContext::ClassElement* m_classElement = nullptr;

            using HandlerCallback = AZStd::function<bool()>;
            AZStd::unordered_map<AZ::TypeId, HandlerCallback> m_handlers;

            struct AttributeData
            {
                Name m_group;
                Name m_name;
                Dom::Value m_value;
            };
            AZStd::vector<AttributeData> m_cachedAttributes;

            virtual ~InstanceVisitor() = default;

            InstanceVisitor(IReadWrite* visitor, void* instance, const AZ::TypeId& typeId, SerializeContext* serializeContext)
                : m_visitor(visitor)
                , m_instance(instance)
                , m_typeId(typeId)
                , m_serializeContext(serializeContext)
            {
                RegisterPrimitiveHandlers<bool, uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t, float, double>();
            }

            template<typename T>
            void RegisterHandler(AZStd::function<bool(T&)> handler)
            {
                m_handlers[azrtti_typeid<T>()] = [this, handler]() -> bool
                {
                    return handler(*reinterpret_cast<T*>(m_instance));
                };
            }

            template<typename T>
            void RegisterPrimitiveHandlers()
            {
                RegisterHandler<T>(
                    [this](T& value) -> bool
                    {
                        m_visitor->Visit(value, *this);
                        return false;
                    });
            }

            template<typename T1, typename T2, typename... Rest>
            void RegisterPrimitiveHandlers()
            {
                RegisterPrimitiveHandlers<T1>();
                RegisterPrimitiveHandlers<T2, Rest...>();
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

                m_serializeContext->EnumerateInstance(&context, m_instance, m_typeId, nullptr, nullptr);
            }

            bool BeginNode(
                void* instance, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* classElement)
            {
                m_instance = instance;
                m_typeId = classData ? classData->m_typeId : Uuid::CreateNull();
                m_classData = classData;
                m_classElement = classElement;

                if (auto handlerIt = m_handlers.find(m_typeId); handlerIt != m_handlers.end())
                {
                    return handlerIt->second();
                }

                CacheAttributes();

                m_visitor->VisitObjectBegin(*this, *this);
                return true;
            }

            bool EndNode()
            {
                if (auto handlerIt = m_handlers.find(m_typeId); handlerIt != m_handlers.end())
                {
                    return true;
                }

                m_visitor->VisitObjectEnd();
                return true;
            }

            const AZ::TypeId& GetType() const override
            {
                return m_typeId;
            }

            AZStd::string_view GetTypeName() const override
            {
                const AZ::SerializeContext::ClassData* classData = m_serializeContext->FindClassData(m_typeId);
                return classData ? classData->m_name : "<unregistered type>";
            }

            void* Get() override
            {
                return m_instance;
            }

            const void* Get() const override
            {
                return m_instance;
            }

            template<typename T>
            bool TryReadAttribute(void* instance, AZ::Attribute* attribute, Dom::Value& result) const
            {
                AZ::AttributeReader reader(instance, attribute);
                T value;
                if (reader.Read<T>(value))
                {
                    result = CreateValue(value);
                    return true;
                }
                return false;
            }

            template<typename First, typename Second, typename... Rest>
            bool TryReadAttribute(void* instance, AZ::Attribute* attribute, Dom::Value& result) const
            {
                if (TryReadAttribute<First>(instance, attribute, result))
                {
                    return true;
                }
                return TryReadAttribute<Second, Rest...>(instance, attribute, result);
            }

            void CacheAttributes()
            {
                m_cachedAttributes.clear();

                // Legacy reflection doesn't have groups, so they're in the root "" group
                const Name group;
                AZStd::unordered_set<Name> visitedAttributes;

                AZStd::string_view labelAttributeValue;

                DocumentPropertyEditor::PropertyEditorSystemInterface* propertyEditorSystem =
                    AZ::Interface<DocumentPropertyEditor::PropertyEditorSystemInterface>::Get();
                AZ_Assert(propertyEditorSystem != nullptr, "LegacyReflectionBridge: Unable to retrieve PropertyEditorSystem");

                auto checkAttribute = [&](const AZ::AttributePair* it)
                {
                    AZ::Name name = propertyEditorSystem->LookupNameFromCrc(it->first);
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
                            bool, uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t, AZStd::string, float, double>(
                            m_instance, it->second, attributeValue);
                        if (readSucceeded)
                        {
                            m_cachedAttributes.push_back({ group, AZStd::move(name), AZStd::move(attributeValue) });
                        }
                    }
                    else
                    {
                        AZ_Warning("DPE", false, "Unable to lookup name for attribute CRC: %" PRId32, it->first);
                    }
                };

                // Read attributes in order, checking:
                // 1) Class element edit data attributes (EditContext from the given row of a class)
                // 2) Class element data attributes (SerializeContext from the given row of a class)
                // 3) Class data attributes (the base attributes of a class)
                if (m_classElement)
                {
                    if (const AZ::Edit::ElementData* elementEditData = m_classElement->m_editData; elementEditData != nullptr)
                    {
                        if (elementEditData->m_name)
                        {
                            labelAttributeValue = elementEditData->m_name;
                        }

                        for (auto it = elementEditData->m_attributes.begin(); it != elementEditData->m_attributes.end(); ++it)
                        {
                            checkAttribute(it);
                        }
                    }

                    if (labelAttributeValue.empty() && m_classElement->m_name)
                    {
                        labelAttributeValue = m_classElement->m_name;
                    }

                    for (auto it = m_classElement->m_attributes.begin(); it != m_classElement->m_attributes.end(); ++it)
                    {
                        AZ::AttributePair pair;
                        pair.first = it->first;
                        pair.second = it->second.get();
                        checkAttribute(&pair);
                    }
                }

                if (m_classData)
                {
                    if (labelAttributeValue.empty() && m_classData->m_name)
                    {
                        labelAttributeValue = m_classData->m_name;
                    }

                    for (auto it = m_classData->m_attributes.begin(); it != m_classData->m_attributes.end(); ++it)
                    {
                        AZ::AttributePair pair;
                        pair.first = it->first;
                        pair.second = it->second.get();
                        checkAttribute(&pair);
                    }
                }

                m_cachedAttributes.push_back({ group, AZ_NAME_LITERAL("Label"), Dom::Value(labelAttributeValue, true) });
            }

            AttributeDataType Find(Name name) const override
            {
                return Find(Name(), AZStd::move(name));
            }

            AttributeDataType Find(Name group, Name name) const override
            {
                for (auto it = m_cachedAttributes.begin(); it != m_cachedAttributes.end(); ++it)
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
                for (const AttributeData& data : m_cachedAttributes)
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
