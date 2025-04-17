/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzFramework/DocumentPropertyEditor/DocumentSchema.h>

namespace AZ::DocumentPropertyEditor
{
    void AdapterBuilder::Value(Dom::Value value)
    {
        CurrentNode().SetNodeValue(AZStd::move(value));
    }

    void AdapterBuilder::Attribute(Name attribute, Dom::Value value)
    {
        CurrentNode()[attribute] = AZStd::move(value);
    }

    void AdapterBuilder::OnEditorChanged(AZStd::function<void(const Dom::Path&, const Dom::Value&, Nodes::ValueChangeType)> onChangedCallback)
    {
        // The value path is the first child of the PropertyEditor node
        Dom::Path changedPath = GetCurrentPath() / Nodes::Label::Value.GetName();
        CallbackAttribute(
            Nodes::PropertyEditor::OnChanged,
            [changedPath, onChangedCallback](const Dom::Value& value, Nodes::ValueChangeType changeType)
            {
                onChangedCallback(changedPath, value, changeType);
            });
    }

    void AdapterBuilder::AddMessageHandler(DocumentAdapter* adapter, AZ::Name messageName, const Dom::Value& contextData)
    {
        BoundAdapterMessage boundMessage;
        boundMessage.m_adapter = adapter;
        boundMessage.m_messageName = messageName;
        boundMessage.m_messageOrigin = GetCurrentPath();
        boundMessage.m_contextData = contextData;
        Attribute(messageName, boundMessage.MarshalToDom());
    }

    Dom::Path AdapterBuilder::GetCurrentPath() const
    {
        return m_currentPath;
    }

    void AdapterBuilder::SetCurrentPath(const Dom::Path& newPath)
    {
        m_currentPath = newPath;
    }

    bool AdapterBuilder::IsError() const
    {
        return !m_error.empty();
    }

    const AZStd::string& AdapterBuilder::GetError() const
    {
        return m_error;
    }

    Dom::Value&& AdapterBuilder::FinishAndTakeResult()
    {
        AZ_Assert(m_entries.empty(), "AdapterBuilder::FinishAndTakeResult called before the builder finished");
        return AZStd::move(m_value);
    }

    void AdapterBuilder::Error(AZStd::string_view message)
    {
        m_error.append(message);
        m_error.append(1, '\n');
    }

    Dom::Value& AdapterBuilder::CurrentNode()
    {
        AZ_Assert(!m_entries.empty(), "AdapterBuilder::CurrentNode called without a node on the entry stack");
        return m_entries.top();
    }

    void AdapterBuilder::BeginNode(Name name)
    {
        // For everything except our root node, add the current index to the path
        if (!m_entries.empty())
        {
            m_currentPath.Push(CurrentNode().ArraySize());
        }
        m_entries.push(Dom::Value::CreateNode(name));
    }

    void AdapterBuilder::EndNode([[maybe_unused]] AZ::Name expectedName)
    {
        AZ_Assert(!m_entries.empty(), "AdapterBuilder::EndNode called with an empty entry stack");
        AZ_Assert(
            expectedName.IsEmpty() || CurrentNode().GetNodeName() == expectedName,
            "AdapterBuilder::EndNode called for %s when %s was expected", CurrentNode().GetNodeName().GetCStr(), expectedName.GetCStr());
        if (!m_currentPath.IsEmpty())
        {
            m_currentPath.Pop();
        }
        Dom::Value value = AZStd::move(m_entries.top());
        m_entries.pop();
        if (!m_entries.empty())
        {
            CurrentNode().ArrayPushBack(AZStd::move(value));
        }
        else
        {
            m_value = AZStd::move(value);
        }
    }

    void AdapterBuilder::BeginAdapter()
    {
        BeginNode<Nodes::Adapter>();
    }

    void AdapterBuilder::EndAdapter()
    {
        EndNode<Nodes::Adapter>();
    }

    void AdapterBuilder::BeginRow()
    {
        BeginNode<Nodes::Row>();
    }

    void AdapterBuilder::EndRow()
    {
        EndNode<Nodes::Row>();
    }

    void AdapterBuilder::BeginLabel()
    {
        BeginNode<Nodes::Label>();
    }

    void AdapterBuilder::EndLabel()
    {
        EndNode<Nodes::Label>();
    }

    void AdapterBuilder::BeginPropertyEditor(AZStd::string_view type, Dom::Value value)
    {
        BeginNode<Nodes::PropertyEditor>();
        if (!type.empty())
        {
            Attribute(Nodes::PropertyEditor::Type, type);
        }
        if (!value.IsNull())
        {
            Attribute(Nodes::PropertyEditor::Value.GetName(), AZStd::move(value));
        }
    }

    void AdapterBuilder::EndPropertyEditor()
    {
        EndNode<Nodes::PropertyEditor>();
    }

    void AdapterBuilder::Label(AZStd::string_view text)
    {
        BeginLabel();
        Attribute(Nodes::Label::Value, text);
        EndLabel();
    }
} // namespace AZ::DocumentPropertyEditor
