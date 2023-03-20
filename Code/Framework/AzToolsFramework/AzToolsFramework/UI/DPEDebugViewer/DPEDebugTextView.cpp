/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/DPEDebugViewer/DPEDebugTextView.h>

namespace AzToolsFramework
{
    DPEDebugTextView::DPEDebugTextView(QWidget* parent)
        : QTextEdit(parent)
    {
    }

    void DPEDebugTextView::SetAdapter(AZ::DocumentPropertyEditor::DocumentAdapterPtr adapter)
    {
        m_adapter = AZStd::move(adapter);
        if (m_adapter == nullptr)
        {
            // Clear out the event handlers when a nullptr DocumentAdapter is suppleid
            m_resetHandler = {};
            m_changeHandler = {};
            return;
        };

        m_resetHandler = AZ::DocumentPropertyEditor::DocumentAdapter::ResetEvent::Handler(
            [this]()
            {
                UpdateContents();
            });
        m_changeHandler = AZ::DocumentPropertyEditor::DocumentAdapter::ChangedEvent::Handler(
            [this](const AZ::Dom::Patch&)
            {
                // For now, just fully recreate our textual contents
                // It might be useful later to highlight what actually changed
                UpdateContents();
            });
        m_adapter->ConnectResetHandler(m_resetHandler);
        m_adapter->ConnectChangedHandler(m_changeHandler);
        UpdateContents();
    }

    namespace
    {
        class AdapterToTextConverter : public AZ::Dom::Visitor
        {
        public:
            static QString DumpText(AZ::DocumentPropertyEditor::DocumentAdapterPtr adapter)
            {
                AdapterToTextConverter converter;
                adapter->GetContents().Accept(converter, true);
                return AZStd::move(converter.m_contents);
            }

            virtual AZ::Dom::VisitorFlags GetVisitorFlags() const
            {
                return AZ::Dom::VisitorFlags::SupportsArrays | AZ::Dom::VisitorFlags::SupportsObjects |
                    AZ::Dom::VisitorFlags::SupportsNodes | AZ::Dom::VisitorFlags::SupportsOpaqueValues |
                    AZ::Dom::VisitorFlags::SupportsRawKeys | AZ::Dom::VisitorFlags::SupportsRawValues;
            }

            Result Null()
            {
                return WriteQuotedValue(QStringLiteral("null"));
            }
            Result Bool(bool value)
            {
                return WriteQuotedValue(value ? QStringLiteral("true") : QStringLiteral("false"));
            }
            Result Int64(AZ::s64 value)
            {
                return WriteQuotedValue(QString::number(value));
            }
            Result Uint64(AZ::u64 value)
            {
                return WriteQuotedValue(QString::number(value));
            }
            Result Double(double value)
            {
                return WriteQuotedValue(QString::number(value));
            }
            Result String(AZStd::string_view value, AZ::Dom::Lifetime)
            {
                return WriteQuotedValue(QString::fromUtf8(value.data(), aznumeric_cast<int>(value.size())));
            }
            Result RefCountedString(AZStd::shared_ptr<const AZStd::vector<char>> value, AZ::Dom::Lifetime lifetime)
            {
                return String({ value->data(), value->size() }, lifetime);
            }
            Result OpaqueValue(AZ::Dom::OpaqueType&)
            {
                return WriteQuotedValue(QStringLiteral("{...C++ type...}"));
            }
            Result RawValue(AZStd::string_view value, AZ::Dom::Lifetime)
            {
                return WriteValue(QString::fromUtf8(value.data(), aznumeric_cast<int>(value.size())));
            }

            Result StartObject()
            {
                StartValue();
                Write(QStringLiteral("{"));
                m_stack.emplace();
                // don't specify m_separator for object stack entries, our Key implementation will handle writing out
                // needed spaces
                return VisitorSuccess();
            }
            Result EndObject(AZ::u64)
            {
                m_stack.pop();
                Write(QStringLiteral("}"));
                return VisitorSuccess();
            }

            Result Key(AZ::Name key)
            {
                return RawKey(key.GetStringView(), AZ::Dom::Lifetime::Temporary);
            }
            Result RawKey(AZStd::string_view key, AZ::Dom::Lifetime)
            {
                NodeEntry* entry = CurrentNode();
                if (entry != nullptr)
                {
                    entry->m_key = true;
                }
                Write(QStringLiteral(" "));
                Write(QString::fromUtf8(key.data(), aznumeric_cast<int>(key.size())));
                Write(QStringLiteral("="));
                return VisitorSuccess();
            }

            Result StartArray()
            {
                StartValue();
                Write(QStringLiteral("["));
                m_stack.emplace();
                m_stack.top().m_separator = QStringLiteral(", ");
                return VisitorSuccess();
            }
            Result EndArray(AZ::u64)
            {
                m_stack.pop();
                Write(QStringLiteral("]"));
                return VisitorSuccess();
            }

            Result StartNode(AZ::Name name)
            {
                return RawStartNode(name.GetStringView(), AZ::Dom::Lifetime::Temporary);
            }
            Result RawStartNode(AZStd::string_view name, AZ::Dom::Lifetime)
            {
                int indentLevel = GetIndent();
                if (auto currentNode = CurrentNode(); currentNode != nullptr && !currentNode->m_firstValue && !currentNode->m_nodeOpen)
                {
                    StartValue();
                    Write(QStringLiteral("\n"));
                    WriteIndent(indentLevel);
                }
                else
                {
                    StartValue();
                }
                Write(QStringLiteral("<"));
                QString nodeName = QString::fromUtf8(name.data(), aznumeric_cast<int>(name.size()));
                Write(nodeName);
                m_stack.emplace();
                m_stack.top().m_nodeOpen = true;
                m_stack.top().m_nodeName = nodeName;
                m_stack.top().m_indent = indentLevel + 1;
                return VisitorSuccess();
            }
            Result EndNode(AZ::u64, AZ::u64)
            {
                if (m_stack.top().m_nodeOpen)
                {
                    Write(QStringLiteral(" />"));
                }
                else
                {
                    Write(QStringLiteral("\n"));
                    WriteIndent(GetIndent() - 1);
                    Write(QStringLiteral("</"));
                    Write(m_stack.top().m_nodeName);
                    Write(QStringLiteral(">"));
                }
                m_stack.pop();
                return VisitorSuccess();
            }

        private:
            struct NodeEntry
            {
                bool m_key = false;
                bool m_nodeOpen = false;
                bool m_firstValue = true;
                QString m_separator = QStringLiteral("");
                QString m_nodeName;
                int m_indent = 0;
            };
            AZStd::stack<NodeEntry> m_stack;
            QString m_contents;

            NodeEntry* CurrentNode()
            {
                return m_stack.empty() ? nullptr : &m_stack.top();
            }

            void CloseNode()
            {
                CurrentNode()->m_nodeOpen = false;
            }

            int GetIndent()
            {
                if (auto node = CurrentNode(); node != nullptr)
                {
                    return node->m_indent;
                }
                return 0;
            }

            void WriteIndent(int indent)
            {
                for (int i = 0; i < indent; ++i)
                {
                    // We're using two-space indents for now, to give more horizontal real estate to deeply nested Adapters
                    Write(QStringLiteral("  "));
                }
            }

            void StartValue()
            {
                if (auto node = CurrentNode(); node != nullptr)
                {
                    if (node->m_nodeOpen && !node->m_key)
                    {
                        m_contents.append(QStringLiteral(">\n"));
                        WriteIndent(GetIndent());
                        node->m_nodeOpen = false;
                    }
                    else if (node->m_key)
                    {
                        node->m_key = false;
                    }

                    if (node->m_firstValue)
                    {
                        node->m_firstValue = false;
                    }
                    else if (!node->m_separator.isEmpty())
                    {
                        m_contents.append(node->m_separator);
                    }
                }
            }

            void Write(QString text)
            {
                m_contents.append(text);
            }

            Result WriteValue(QString value)
            {
                StartValue();
                Write(value);
                return VisitorSuccess();
            }

            Result WriteQuotedValue(QString value)
            {
                StartValue();
                Write(QStringLiteral("\""));
                Write(value);
                Write(QStringLiteral("\""));
                return VisitorSuccess();
            }
        };
    } // namespace

    void DPEDebugTextView::UpdateContents()
    {
        setPlainText(AdapterToTextConverter::DumpText(m_adapter));
    }
} // namespace AzToolsFramework
