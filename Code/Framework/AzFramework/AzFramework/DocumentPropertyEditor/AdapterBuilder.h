/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/stack.h>
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <AzFramework/DocumentPropertyEditor/DocumentSchema.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>

namespace AZ::DocumentPropertyEditor
{
    //! Helper class that builds a DOM suitable for usage in a DocumentAdapter.
    //! Uses a visitor pattern to establish node elements.
    class AdapterBuilder
    {
    public:
        AdapterBuilder() = default;

        //! Begins a new Node, pushing its value to the entry stack.
        //! Nodes may have child nodes, values, and attributes, as dictated by their node definiton.
        //! Calls to Attribute and Value will apply to the last BeginNode call.
        //! Each call to BeginNode must have a corresponding EndNode invocation.
        void BeginNode(AZ::Name name);
        //! Ends the current node, popping its value from the entry stack.
        //! Once the last BeginNode call has had a corresponding EndNode invocation, this builder is in a finished state.
        //! The result may then be retrieved.
        void EndNode(AZ::Name expectedName);

        //! Convenience method, calls BeginNode<Nodes::Adapter>()
        void BeginAdapter();
        //! Convenience method, calls EndNode<Nodes::Adapter>()
        void EndAdapter();
        //! Convenience method, calls BeginNode<Nodes::Row>()
        void BeginRow();
        //! Convenience method, calls EndNode<Nodes::Row>()
        void EndRow();
        //! Convenience method, calls BeginNode<Nodes::Label>()
        void BeginLabel();
        //! Convenience method, calls EndNode<Nodes::Label>()
        void EndLabel();
        //! Convenience method, calls BeginNode<Nodes::PropertyEditor>()
        //! If type and value are specified, the Nodes::PropertyEditor::Type attribute and the
        //! property editor value shall be set.
        void BeginPropertyEditor(AZStd::string_view type = "", Dom::Value value = {});
        //! Convenience method, calls EndNode<Nodes::PropertyEditor>()
        void EndPropertyEditor();

        //! Inserts a Label node with the specified text.
        void Label(AZStd::string_view text);

        //! Sets the value of the last node. Used for setting the current value of a property editor.
        void Value(Dom::Value value);
        //! Sets an attribute of the last node. Rows, labels, and property editors all support different attributes.
        //! \see DocumentPropertyEditor::Nodes
        void Attribute(Name attribute, Dom::Value value);
        //! Adds a Nodes::PropertyEditor::OnChanged attribute that will get called with
        //! the path of the property value and its new value. This path can be used to generate a
        //! correct Replace patch for submitting NotifyContentsChanged.
        void OnEditorChanged(
            AZStd::function<void(const Dom::Path&, const Dom::Value&, Nodes::ValueChangeType)> onChangedCallback);
        //! Adds a message handler bound to the given adapter for a given message name or callback attribute.
        //! \param adapter The adapter to bind this message to.
        //! \param messageName The name of the message.
        //! \param contextData If specified, is provided as additional message data when this message is sent.
        void AddMessageHandler(DocumentAdapter* adapter, AZ::Name messageName, const Dom::Value& contextData = {});

        //! Gets the path to the DOM node currently being built within this builder's DOM.
        Dom::Path GetCurrentPath() const;

        void SetCurrentPath(const Dom::Path&);

        //! Returns true if an error has been encountered during the build process,
        bool IsError() const;
        //! Returns the error information, if any, encountered during the build process.
        const AZStd::string& GetError() const;
        //! Ends the build operation and retrieves the builder result.
        //! Operations are no longer valid on this builder once this is called.
        Dom::Value&& FinishAndTakeResult();

        template<class PropertyEditorDefinition, class ValueType = AZ::Dom::Value>
        void BeginPropertyEditor(ValueType value = {})
        {
            BeginPropertyEditor(PropertyEditorDefinition::Name, AZ::Dom::Utils::ValueFromType(value));
        }

        template<class NodeDefinition>
        void BeginNode()
        {
            BeginNode(GetNodeName<NodeDefinition>());
        }

        template<class NodeDefinition>
        void EndNode()
        {
            EndNode(GetNodeName<NodeDefinition>());
        }

        template<class AttributeType>
        void Attribute(const AttributeDefinition<AttributeType>& definition, Dom::Value value)
        {
            Attribute(definition.GetName(), Dom::Value(AZStd::move(value)));
        }

        template<class AttributeType>
        void Attribute(const AttributeDefinition<AttributeType>& definition, AttributeType value)
        {
            Attribute(definition.GetName(), definition.ValueToDom(AZStd::move(value)));
        }

        template<class CallbackType, class Functor>
        void CallbackAttribute(const CallbackAttributeDefinition<CallbackType>& definition, Functor value)
        {
            Attribute(definition.GetName(), definition.ValueToDom(AZStd::function<CallbackType>(value)));
        }

        void Attribute(const AttributeDefinition<AZStd::string_view>& definition, AZStd::string_view value)
        {
            Attribute(definition.GetName(), Dom::Value(value, true));
        }

        template<class CallbackType>
        void AddMessageHandler(DocumentAdapter* adapter, const CallbackAttributeDefinition<CallbackType>& callback, const Dom::Value& contextData = {})
        {
            AddMessageHandler(adapter, callback.GetName(), contextData);
        }

    private:
        void Error(AZStd::string_view message);
        Dom::Value& CurrentNode();

        Dom::Value m_value;
        Dom::Path m_currentPath;
        AZStd::stack<Dom::Value> m_entries;
        AZStd::string m_error;
    };
} // namespace AZ::DocumentPropertyEditor
