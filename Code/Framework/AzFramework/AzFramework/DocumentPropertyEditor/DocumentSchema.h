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
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/fixed_string.h>

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
        return AZ::Name(NodeDefinition::Name);
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

    //! Defines an attribute applicable to a Node.
    //! Attributes may be defined inline inside of a NodeDefinition.
    //! AttributeDefinitions may be used to marshal types to and from a DOM representation.
    //! This representation is not guaranteed to be serializable - for functions and complex types
    //! they may be stored as non-serializeable opaque types in the DOM.
    template<typename AttributeType>
    class AttributeDefinition
    {
    public:
        constexpr AttributeDefinition(AZStd::string_view name)
            : m_name(name)
        {
        }

        //! Retrieves the name of this attribute, as used as a key in the DOM.
        Name GetName() const
        {
            return AZ::Name(m_name);
        }

        //! Converts a value of this attribute's type to a DOM value.
        Dom::Value ValueToDom(const AttributeType& attribute) const
        {
            return Dom::Utils::ValueFromType(attribute);
        }

        //! Converts a DOM value to an instance of AttributeType.
        AZStd::optional<AttributeType> DomToValue(const Dom::Value& value) const
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

        //! Gets this attribute's type ID
        const AZ::TypeId& GetTypeId() const
        {
            return azrtti_typeid<AttributeType>();
        }

    protected:
        AZStd::fixed_string<128> m_name;
    };

    //! Runtime data describing an AttributeDefinition tied to a NodeDefinition.
    //! This is used to look up an attribution from a given name and parent node in the PropertyEditorSystem.
    struct AttributeMetadata
    {
        //! Helper method, creates a definition from an AttributeDefinition instance for a given node.
        template<typename AttributeDefinition>
        static AttributeMetadata FromDefinition(const AttributeDefinition& definition, const NodeMetadata* node)
        {
            AttributeMetadata metadata;
            metadata.m_name = definition.GetName();
            metadata.m_node = node;
            return metadata;
        }

        AZ::Name m_name;
        const NodeMetadata* m_node;
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
        };

        using CallbackTraits = Traits<CallbackSignature>;

        //! Attempts to call a function with this attribute's callback signature stored in a DOM value as an opaque type.
        //! Invokes the method with the specified args if our callback signature is found in the value, otherwise returns an error message.
        template<typename... Args>
        typename CallbackTraits::ResultType InvokeOnDomValue(const AZ::Dom::Value& value, Args... args) const
        {
            if (!value.IsOpaqueValue())
            {
                return AZ::Failure<ErrorType>("This property is holding a value and not a callback");
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
            auto attributeIt = value.FindMember(this->GetName());
            if (attributeIt == value.MemberEnd())
            {
                return AZ::Failure<ErrorType>(ErrorType::format("No \"%s\" attribute found", this->m_name.data()));
            }
            return this->InvokeOnDomValue(attributeIt->second, args...);
        }
    };
} // namespace AZ::DocumentPropertyEditor
