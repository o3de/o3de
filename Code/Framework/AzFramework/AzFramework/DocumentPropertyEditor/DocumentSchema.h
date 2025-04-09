/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomUtils.h>
#include <AzCore/DOM/DomValue.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <AzFramework/DocumentPropertyEditor/Reflection/LegacyReflectionBridge.h>

namespace AZ::DocumentPropertyEditor
{
    //! Defines a schema definition for a DocumentAdapter node.
    //! Nodes have a name and metadata for validating their contents.
    //! To register a Node you may inherit from NodeDefinition and define traits:
    //! struct MyNode : NodeDefinition
    //! {
    //!     static constexpr AZStd::string_view Name = "MyNode";
    //!     static bool CanAddToParentNode(const Dom::Value& parentNode);
    //! };
    struct NodeDefinition
    {
        //! Defines the Name of the node definition.
        //! This field must be defined for all NodeDefinitions.
        static constexpr AZStd::string_view Name = "<undefined node name>";

        //! If defined, specifies if this node may be added to a given parent (a DOM node).
        //! By default all parent nodes are accepted.
        static bool CanAddToParentNode([[maybe_unused]] const Dom::Value& parentNode)
        {
            return true;
        }

        //! If defined, specifies if this node may receive a child (a DOM Value that may be of an arbitrary type).
        //! By default all values are accepted.
        static bool CanBeParentToValue([[maybe_unused]] const Dom::Value& value)
        {
            return true;
        }
    };

    //! Retrieves a node's name from a node definition.
    //! \see NodeDefinition
    template<typename NodeDefinition>
    Name GetNodeName()
    {
        return AZ_NAME_LITERAL(NodeDefinition::Name);
    }

    //! Runtime data describing a NodeDescriptor.
    //! This is used to look up a descriptor from a given name in the PropertyEditorSystem.
    struct NodeMetadata
    {
        //! Helper method, extracts runtime metadata from a NodeDefinition.
        template<typename NodeDefinition>
        static NodeMetadata FromType(const NodeMetadata* parent = nullptr)
        {
            NodeMetadata metadata;
            metadata.m_name = GetNodeName<NodeDefinition>();
            metadata.m_parent = parent;
            metadata.m_canAddToParentNode = &NodeDefinition::CanAddToParentNode;
            metadata.m_canBeParentToValue = &NodeDefinition::CanBeParentToValue;
            return metadata;
        }

        //! Returns true if this node, or any of its parents, have any ancestor with ParentNode's name.
        template<typename ParentNode>
        bool InheritsFrom() const
        {
            const Name targetParentName = GetNodeName<ParentNode>();
            const NodeMetadata* parent = this;
            while (parent != nullptr)
            {
                if (parent->m_name == targetParentName)
                {
                    return true;
                }
                parent = parent->m_parent;
            }
            return false;
        }

        AZ::Name m_name;
        AZStd::function<bool(const Dom::Value&)> m_canAddToParentNode;
        AZStd::function<bool(const Dom::Value&)> m_canBeParentToValue;
        const NodeMetadata* m_parent = nullptr;
    };

    //! PropertyEditors store themselves as nodes, even though they're actually a PropertyEditor node with a type attribute.
    //! All functionality (custom attribute per editor type, etc) works the same.
    using PropertyEditorMetadata = NodeMetadata;
    using PropertyEditorDefinition = NodeDefinition;

    //! Base class for a schema definition for an attribute type on a node.
    //! Attributes are stroed as key/value pairs, and their definitions provide helpers for marshalling
    //! data into and out of the DOM to their associated type.
    //! \see AttributeDefinition
    class AttributeDefinitionInterface
    {
    public:
        //! Retrieves the name of this attribute, as used as a key in the DOM.
        virtual Name GetName() const = 0;
        //! Gets this attribute's type ID.
        virtual AZ::TypeId GetTypeId() const = 0;
        /*! Converts this attribute to an AZ::Attribute usable by the ReflectedPropertyEditor
            @param fallback if false, the Attribute type must match AZ::Dom::Value; if true, it will attempt a fallback on failure */
        virtual AZStd::shared_ptr<AZ::Attribute> DomValueToLegacyAttribute(const AZ::Dom::Value& value, bool fallback = true) const = 0;

        /*! Converts this attribute from an AZ::Attribute to a Dom::Value usable in the DocumentPropertyEditor.
            @param fallback if false, a Read<AttributeType> failure will return a null Value; if true, it will attempt a fallback on failure */
        virtual AZ::Dom::Value LegacyAttributeToDomValue(AZ::PointerObject instanceObject, AZ::Attribute* attribute) const = 0;
    };

    //! Defines an attribute applicable to a Node.
    //! Attributes may be defined inline inside of a NodeDefinition.
    //! AttributeDefinitions may be used to marshal types to and from a DOM representation.
    //! This representation is not guaranteed to be serializable - for functions and complex types
    //! they may be stored as non-serializeable opaque types in the DOM.
    template<typename AttributeType>
    class AttributeDefinition : public AttributeDefinitionInterface
    {
    public:
        constexpr AttributeDefinition(AZStd::string_view name)
            : AttributeDefinitionInterface()
            , m_name(name)
        {
        }

        Name GetName() const override
        {
            return AZ::Name(m_name);
        }

        //! Converts a value of this attribute's type to a DOM value.
        virtual Dom::Value ValueToDom(const AttributeType& attribute) const
        {
            return Dom::Utils::ValueFromType(attribute);
        }

        //! Converts a DOM value to an instance of AttributeType.
        virtual AZStd::optional<AttributeType> DomToValue(const Dom::Value& value) const
        {
            return Dom::Utils::ValueToType<AttributeType>(value);
        }

        //! Extracts this value from a given Node, if this attribute is set there.
        AZStd::optional<AttributeType> ExtractFromDomNode(const Dom::Value& node) const
        {
            if (!node.IsNode() && !node.IsObject())
            {
                return {};
            }

            auto memberIt = node.FindMember(GetName());
            if (memberIt == node.MemberEnd())
            {
                return {};
            }

            return DomToValue(memberIt->second);
        }

        AZ::TypeId GetTypeId() const override
        {
            return azrtti_typeid<AttributeType>();
        }

        AZStd::shared_ptr<AZ::Attribute> DomValueToLegacyAttribute(const AZ::Dom::Value& value, bool fallback) const override
        {
            if constexpr (AZStd::is_same_v<AttributeType, AZ::Dom::Value>)
            {
                return AZ::Reflection::WriteDomValueToGenericAttribute(value);
            }
            else
            {
                if (fallback)
                {
                    AZStd::optional<AttributeType> attributeValue = DomToValue(value);
                    if (attributeValue.has_value())
                    {
                        return AZStd::make_shared<AZ::AttributeData<AttributeType>>(AZStd::move(attributeValue.value()));
                    }
                }
                return nullptr;
            }
        }

        AZ::Dom::Value LegacyAttributeToDomValue(AZ::PointerObject instanceObject, AZ::Attribute* attribute) const override
        {
            if (attribute == nullptr)
            {
                return AZ::Dom::Value();
            }

            if constexpr (AZStd::is_same_v<AttributeType, AZ::Dom::Value>)
            {
                return AZ::Reflection::ReadGenericAttributeToDomValue(instanceObject, attribute).value_or(AZ::Dom::Value());
            }
            else
            {
                AZ::AttributeReader reader(instanceObject.m_address, attribute);
                AttributeType value;
                if (!reader.Read<AttributeType>(value))
                {
                    // Handle the attribute providing an invokable function instead of the value directly
                    Dom::Value instanceAndArguments(Dom::Type::Array);
                    instanceAndArguments.ArrayPushBack(AZ::Dom::Utils::ValueFromType(instanceObject.m_address));
                    if (attribute->CanDomInvoke(instanceAndArguments))
                    {
                        return attribute->DomInvoke(instanceAndArguments);
                    }
                    else
                    {
                        return AZ::Dom::Value();
                    }
                }
                return ValueToDom(value);
            }
        }

    protected:
        AZStd::fixed_string<128> m_name;
    };

    //! Represents an attribute that should resolve to an AZ::TypeId with a string representation.
    class TypeIdAttributeDefinition final : public AttributeDefinition<AZ::TypeId>
    {
    public:
        explicit constexpr TypeIdAttributeDefinition(AZStd::string_view name)
            : AttributeDefinition<AZ::TypeId>(name)
        {
        }

        Dom::Value ValueToDom(const AZ::TypeId& attribute) const override;
        AZStd::optional<AZ::TypeId> DomToValue(const Dom::Value& value) const override;
        AZStd::shared_ptr<AZ::Attribute> DomValueToLegacyAttribute(const AZ::Dom::Value& value, bool fallback = true) const override;
        AZ::Dom::Value LegacyAttributeToDomValue(AZ::PointerObject instanceObject, AZ::Attribute* attribute) const override;
    };

    //! Represents an attribute that should be stored as an AZ::Name, but legacy attribute instances (AZ::Attribute*)
    //! will marshal the attribute as a CRC32 that needs to be translated back into a Name.
    class NamedCrcAttributeDefinition final : public AttributeDefinition<AZ::Name>
    {
    public:
        explicit constexpr NamedCrcAttributeDefinition(AZStd::string_view name)
            : AttributeDefinition<AZ::Name>(name)
        {
        }

        Dom::Value ValueToDom(const AZ::Name& attribute) const override;
        AZStd::optional<AZ::Name> DomToValue(const Dom::Value& value) const override;
        AZStd::shared_ptr<AZ::Attribute> DomValueToLegacyAttribute(const AZ::Dom::Value& value, bool fallback = true) const override;
        AZ::Dom::Value LegacyAttributeToDomValue(AZ::PointerObject instanceObject, AZ::Attribute* attribute) const override;
    };

    template<typename GenericValueType>
    class GenericValueAttributeDefinition final : public AttributeDefinition<AZStd::pair<GenericValueType, AZStd::string>>
    {
    public:
        using GenericValuePair = AZStd::pair<GenericValueType, AZStd::string>;

        static constexpr const char* EntryDescriptionKey = "description";
        static constexpr const char* EntryValueKey = "value";

        explicit constexpr GenericValueAttributeDefinition(AZStd::string_view name)
            : AttributeDefinition<GenericValuePair>(name)
        {
        }

        Dom::Value ValueToDom(const GenericValuePair& attribute) const override
        {
            Dom::Value result(Dom::Type::Object);
            if constexpr (AZStd::is_constructible_v<Dom::Value, GenericValueType>)
            {
                // this type is already constructible as a normal Dom::Value, create it as-is
                result[EntryValueKey] = Dom::Value(attribute.first);
            }
            else
            {
                // type doesn't fit directly into a Dom::Value, construct it as an opaque value
                result[EntryValueKey] = Dom::Value::FromOpaqueValue(AZStd::make_any<GenericValueType>(attribute.first));
            }
            result[EntryDescriptionKey] = Dom::Value(attribute.second, true);
            return result;
        }

        AZ::Dom::Value LegacyAttributeToDomValue(AZ::PointerObject instanceObject, AZ::Attribute* attribute) const override
        {
            if (attribute == nullptr)
            {
                return {};
            }

            // This case is for handling legacy EnumValueKey attributes
            if constexpr (AZStd::is_same_v<GenericValueType, AZ::u64>)
            {
                using EnumConstantBaseType = AZ::SerializeContextEnumInternal::EnumConstantBase;
                if (auto data = azdynamic_cast<AttributeData<AZStd::unique_ptr<EnumConstantBaseType>>*>(attribute); data != nullptr)
                {
                    EnumConstantBaseType* value = static_cast<EnumConstantBaseType*>(data->Get(instanceObject.m_address).get());
                    return ValueToDom(AZStd::make_pair(value->GetEnumValueAsUInt(), value->GetEnumValueName()));
                }
            }

            AZ::AttributeReader reader(instanceObject.m_address, attribute);
            if (GenericValuePair value; reader.Read<GenericValuePair>(value))
            {
                return ValueToDom(value);
            }

            return {};
        }

        AZStd::optional<GenericValuePair> DomToValue(const Dom::Value& value) const override
        {
            if (!value.IsObject() || !value.HasMember(EntryDescriptionKey) || !value.HasMember(EntryValueKey))
            {
                return {};
            }

            if (auto genericValueOpt = Dom::Utils::ValueToType<GenericValueType>(value[EntryValueKey]); genericValueOpt.has_value())
            {
                return GenericValuePair{ genericValueOpt.value(), AZStd::string(value[EntryDescriptionKey].GetString()) };
            }

            return {};
        }
    };

    template<typename T>
    using GenericValueContainer = AZStd::vector<AZStd::pair<T, AZStd::string>>;

    template<typename GenericValueType>
    class GenericValueListAttributeDefinition final : public AttributeDefinition<GenericValueContainer<GenericValueType>>
    {
    public:
        using GenericValueList = GenericValueContainer<GenericValueType>;

        static constexpr const char* EntryDescriptionKey = "description";
        static constexpr const char* EntryValueKey = "value";

        explicit constexpr GenericValueListAttributeDefinition(AZStd::string_view name)
            : AttributeDefinition<GenericValueList>(name)
        {
        }

        Dom::Value ValueToDom(const GenericValueList& attribute) const override
        {
            Dom::Value result(Dom::Type::Array);
            for (const auto& entry : attribute)
            {
                Dom::Value entryDom(Dom::Type::Object);
                if constexpr (AZStd::is_constructible_v<Dom::Value, GenericValueType>)
                {
                    // this type is already constructible as a normal Dom::Value, create it as-is
                    entryDom[EntryValueKey] = Dom::Value(entry.first);
                }
                else
                {
                    // type doesn't fit directly into a Dom::Value, construct it as an opaque value
                    entryDom[EntryValueKey] = Dom::Value::FromOpaqueValue(AZStd::make_any<GenericValueType>(entry.first));
                }
                entryDom[EntryDescriptionKey] = Dom::Value(entry.second, true);
                result.ArrayPushBack(AZStd::move(entryDom));
            }
            return result;
        }

        AZ::Dom::Value LegacyAttributeToDomValue(AZ::PointerObject instanceObject, AZ::Attribute* attribute) const override
        {
            if (attribute == nullptr)
            {
                return {};
            }

            auto attributeInvocable = attribute->GetVoidInstanceAttributeInvocable();
            AttributeReader reader = AttributeReader(instanceObject.m_address, attributeInvocable.get());
            if (GenericValueList value; reader.Read<GenericValueList>(value))
            {
                return ValueToDom(value);
            }

            return {};
        }

        AZStd::optional<GenericValueList> DomToValue(const Dom::Value& value) const override
        {
            if (!value.IsArray())
            {
                return {};
            }

            GenericValueList result;
            for (const Dom::Value& entryDom : value.GetArray())
            {
                if (!entryDom.IsObject() || !entryDom.HasMember(EntryValueKey) || !entryDom.HasMember(EntryDescriptionKey))
                {
                    continue;
                }

                if (auto genericValueOpt = Dom::Utils::ValueToType<GenericValueType>(entryDom[EntryValueKey]); genericValueOpt.has_value())
                {
                    result.emplace_back(genericValueOpt.value(), entryDom[EntryDescriptionKey].GetString());
                }
            }

            return !result.empty() ? AZStd::make_optional(AZStd::move(result)) : AZStd::nullopt;
        }
    };

    //! Defines a callback applicable to a Node.
    //! Callbacks are stored as attributes and accept an AZStd::function<CallbackSignature> stored as an
    //! opaque value. Callbacks can be validated and invokved from DOM values using InvokeOnDomValue
    //! and InvokeOnDomNode.
    template<typename CallbackSignature>
    class CallbackAttributeDefinition : public AttributeDefinition<AZStd::function<CallbackSignature>>
    {
    public:
        using ErrorType = AZStd::fixed_string<128>;
        using FunctionType = AZStd::function<CallbackSignature>;

        constexpr CallbackAttributeDefinition(AZStd::string_view name)
            : AttributeDefinition<FunctionType>(name)
        {
        }

        template<typename T>
        struct Traits;

        template<typename Result, typename... Args>
        struct Traits<Result(Args...)>
        {
            using ResultType = AZ::Outcome<Result, ErrorType>;

            static ResultType Invoke(const AZStd::function<Result(Args...)>& fn, Args... args)
            {
                return AZ::Success(fn(args...));
            }

            static ResultType InvokeOnAttribute(AZ::Attribute* attribute, const Dom::Value& instanceAndArgs)
            {
                return AZ::Success(AZ::Dom::Utils::ValueToTypeUnsafe<Result>(attribute->DomInvoke(instanceAndArgs)));
            }

            static FunctionType InvokeOnAttribute(void* instance, AZ::Attribute* attribute)
            {
                return [instance, attribute](Args... args)
                {
                    AttributeReader reader(instance, attribute);
                    Result result;
                    reader.Read<Result>(result, args...);
                    return result;
                };
            }

            static ResultType InvokeOnBoundMessage(BoundAdapterMessage& boundMessage, const Dom::Value& args)
            {
                return AZ::Success(AZ::Dom::Utils::ValueToTypeUnsafe<Result>(boundMessage(args)));
            }

            static ResultType ValueToResult(const Dom::Value& value)
            {
                auto result = AZ::Dom::Utils::ValueToType<Result>(value);
                if (!result.has_value())
                {
                    return AZ::Failure<ErrorType>("Failed to convert return value type");
                }
                return AZ::Success(AZStd::move(result.value()));
            }
        };

        template<typename... Args>
        struct Traits<void(Args...)>
        {
            using ResultType = AZ::Outcome<void, ErrorType>;

            static ResultType Invoke(const AZStd::function<void(Args...)>& fn, Args... args)
            {
                fn(args...);
                return AZ::Success();
            }

            static ResultType InvokeOnAttribute(AZ::Attribute* attribute, const Dom::Value& instanceAndArgs)
            {
                attribute->DomInvoke(instanceAndArgs);
                return AZ::Success();
            }

            static FunctionType InvokeOnAttribute(void* instance, AZ::Attribute* attribute)
            {
                return [instance, attribute](Args... args)
                {
                    AttributeReader reader(instance, attribute);
                    reader.Invoke<void>(args...);
                };
            }

            static ResultType InvokeOnBoundMessage(BoundAdapterMessage& boundMessage, const Dom::Value& args)
            {
                boundMessage(args);
                return AZ::Success();
            }

            static ResultType ValueToResult(const Dom::Value&)
            {
                return AZ::Success();
            }
        };

        using CallbackTraits = Traits<CallbackSignature>;
        using MessageCallbackType = AZStd::function<CallbackSignature>;

        //! Attempts to call a function with this attribute's callback signature stored in a DOM value as an opaque type.
        //! Invokes the method with the specified args if our callback signature is found in the value, otherwise returns an error message.
        template<typename... Args>
        typename CallbackTraits::ResultType InvokeOnDomValue(const AZ::Dom::Value& value, Args... args) const
        {
            if (value.IsObject())
            {
                auto typeField = value.FindMember(AZ::Attribute::GetTypeField());
                if (typeField != value.MemberEnd() && typeField->second.IsString())
                {
                    // For RPE callbacks, we may store an AZ::Attribute and its instance in a DOM value
                    if (typeField->second.GetString() == Attribute::GetTypeName())
                    {
                        AZ::Dom::Value marshalledArguments(AZ::Dom::Type::Array);
                        auto foundInstance = value.FindMember(AZ::Attribute::GetInstanceField());
                        if (foundInstance != value.MemberEnd())
                        {
                            // Push the instance arguments as the first marshelled argument for the attribute functor
                            marshalledArguments.ArrayPushBack(foundInstance->second);
                        }

                        AZ::Attribute* attribute =
                            AZ::Dom::Utils::ValueToTypeUnsafe<AZ::Attribute*>(value[AZ::Attribute::GetAttributeField()]);

                        if (!attribute->IsInvokable())
                        {
                            return AZ::Failure<ErrorType>("Attempted to invoke a non-invokable attribute");
                        }

                        // Push the remaining non-class type arguments to the Dom array containing the arguments
                        // of the attribute functor that would be inovked
                        (marshalledArguments.ArrayPushBack(AZ::Dom::Utils::ValueFromType(args)), ...);

                        if (!attribute->CanDomInvoke(marshalledArguments))
                        {
                            return AZ::Failure<ErrorType>("Attempted to invoke an AZ::Attribute with invalid parameters");
                        }

                        return CallbackTraits::InvokeOnAttribute(attribute, marshalledArguments);
                    }
                    // For messages handled by the adapter, we store a marshalled BoundAdapterMessage
                    else if (typeField->second.GetString() == BoundAdapterMessage::s_typeName)
                    {
                        auto boundMessage = BoundAdapterMessage::TryMarshalFromDom(value);
                        if (boundMessage.has_value())
                        {
                            AZ::Dom::Value marshalledArguments(AZ::Dom::Type::Array);
                            (marshalledArguments.ArrayPushBack(AZ::Dom::Utils::ValueFromType(args)), ...);

                            return CallbackTraits::InvokeOnBoundMessage(boundMessage.value(), marshalledArguments);
                        }
                    }
                }
            }
            else if (value.IsArray())
            {
                typename CallbackTraits::ResultType overallResult;
                for (size_t valueIndex = 0, numValues = value.ArraySize(); valueIndex < numValues; ++valueIndex)
                {
                    // Note: Currently, last result wins. If different behavior is desirable in the future,
                    // we can parameterize this function
                    overallResult = InvokeOnDomValue(value[valueIndex], args...);
                }
                return overallResult;
            }
            else if (!value.IsOpaqueValue())
            {
                // CallbackAttributes that return a value may be bound to a simple value of that type
                // In that case, ignore our parameters and simply return the value
                return CallbackTraits::ValueToResult(value);
            }

            const AZStd::any& wrapper = value.GetOpaqueValue();
            if (!wrapper.is<FunctionType>())
            {
                return AZ::Failure<ErrorType>("This property does not contain a callback with the correct signature");
            }

            return CallbackTraits::Invoke(AZStd::any_cast<FunctionType>(wrapper), args...);
        }

        //! Attemps to read this attribute from the specified DOM value, which must be a Node.
        //! Invokes the method with the specified args if this attribute is specified, otherwise returns an error message.
        template<typename... Args>
        typename CallbackTraits::ResultType InvokeOnDomNode(const AZ::Dom::Value& value, Args... args) const
        {
            if (!value.IsNode())
            {
                return AZ::Failure<ErrorType>("Value is not a Node");
            }
            auto attributeIt = value.FindMember(this->GetName());
            if (attributeIt == value.MemberEnd())
            {
                return AZ::Failure<ErrorType>(ErrorType::format("No \"%s\" attribute found", this->m_name.data()));
            }
            return this->InvokeOnDomValue(attributeIt->second, args...);
        }

        //! Checks if a given AdapterMessage matches this attribute and invokes a provided callback if so.
        //! Returns true if the callback was invoked.
        bool MatchMessage(const AdapterMessage& message, Dom::Value& result, const MessageCallbackType& callback) const
        {
            if (message.m_messageName == this->GetName())
            {
                if (AZ::DomInvokeHelper<MessageCallbackType>::CanInvoke(message.m_messageParameters))
                {
                    result = AZ::DomInvokeHelper<MessageCallbackType>::Invoke(callback, message.m_messageParameters);
                    return true;
                }
            }
            return false;
        }

        AZStd::shared_ptr<AZ::Attribute> DomValueToLegacyAttribute(const AZ::Dom::Value& value, bool) const override
        {
            // If we're already an attribute, return a non-owning shared_ptr
            if (value.IsOpaqueValue() && value.GetOpaqueValue().is<AZ::Attribute*>())
            {
                AZ::Attribute* attribute = AZStd::any_cast<AZ::Attribute*>(value.GetOpaqueValue());
                return AZStd::shared_ptr<AZ::Attribute>(
                    attribute,
                    [](AZ::Attribute*)
                    {
                    });
            }

            // Otherwise, try to store an AZStd::function with our signature
            auto function = Dom::Utils::ValueToType<FunctionType>(value);
            if (!function.has_value())
            {
                return {};
            }
            return AZStd::make_shared<AZ::AttributeInvocable<CallbackSignature>>(function.value());
        }

        AZ::Dom::Value LegacyAttributeToDomValue(AZ::PointerObject instanceObject, AZ::Attribute* attribute) const override
        {
            return attribute->GetAsDomValue(instanceObject);
        }
    };
} // namespace AZ::DocumentPropertyEditor
