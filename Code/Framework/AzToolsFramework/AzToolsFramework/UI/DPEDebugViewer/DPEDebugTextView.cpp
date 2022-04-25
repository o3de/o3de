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
                return Write(QStringLiteral("null"));
            }
            Result Bool(bool value)
            {
                return Write(value ? QStringLiteral("true") : QStringLiteral("false"));
            }
            Result Int64(AZ::s64 value)
            {
                return Write(QString::number(value));
            }
            Result Uint64(AZ::u64 value)
            {
                return Write(QString::number(value));
            }
            Result Double(double value)
            {
                return Write(QString::number(value));
            }
            Result String(AZStd::string_view value, AZ::Dom::Lifetime)
            {
                Write(QStringLiteral("\""));
                Write(QString::fromUtf8(value.data(), aznumeric_cast<int>(value.size())), false);
                return Write(QStringLiteral("\""), false);
            }
            Result RefCountedString(AZStd::shared_ptr<const AZStd::vector<char>> value, AZ::Dom::Lifetime lifetime)
            {
                return String({ value->data(), value->size() }, lifetime);
            }
            Result OpaqueValue(AZ::Dom::OpaqueType&)
            {
                return Write(QStringLiteral("{...C++ type...}"));
            }
            Result RawValue(AZStd::string_view value, AZ::Dom::Lifetime)
            {
                return Write(QString::fromUtf8(value.data(), aznumeric_cast<int>(value.size())));
            }

            Result StartObject()
            {
                Result result = Write(QStringLiteral("["));
                m_stack.emplace();
                return result;
            }
            Result EndObject(AZ::u64)
            {
                m_stack.pop();
                return Write(QStringLiteral("]"), false);
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
                Write(QStringLiteral(" "), false);
                Write(QString::fromUtf8(key.data(), aznumeric_cast<int>(key.size())), false);
                return Write(QStringLiteral("="), false);
            }

            Result StartArray()
            {
                Result result = Write(QStringLiteral("["));
                m_stack.emplace();
                m_stack.top().m_separator = QStringLiteral(", ");
                return result;
            }
            Result EndArray(AZ::u64)
            {
                m_stack.pop();
                return Write(QStringLiteral("]"), false);
            }

            Result StartNode(AZ::Name name)
            {
                return RawStartNode(name.GetStringView(), AZ::Dom::Lifetime::Temporary);
            }
            Result RawStartNode(AZStd::string_view name, AZ::Dom::Lifetime)
            {
                int indentLevel = GetIndent();
                if (auto currentNode = CurrentNode(); currentNode != nullptr && !currentNode->m_firstValue)
                {
                    Write(QStringLiteral("\n"), true);
                    WriteIndent(indentLevel);
                }
                else
                {
                    Write(QString());
                }
                Write(QStringLiteral("<"), false);
                QString nodeName = QString::fromUtf8(name.data(), aznumeric_cast<int>(name.size()));
                Write(nodeName, false);
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
                    Write(QStringLiteral(" />"), false);
                }
                else
                {
                    Write(QStringLiteral("\n"));
                    WriteIndent(GetIndent() - 1);
                    Write(QStringLiteral("</"), false);
                    Write(m_stack.top().m_nodeName, false);
                    Write(QStringLiteral(">"), false);
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
                QString m_separator = "";
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
                    Write(QStringLiteral("  "), false);
                }
            }

            Result Write(QString text, bool checkNode = true)
            {
                NodeEntry* node = CurrentNode();
                if (checkNode && node != nullptr)
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
                m_contents.append(text);
                return VisitorSuccess();
            }
        };
    } // namespace

    void DPEDebugTextView::UpdateContents()
    {
        setPlainText(AdapterToTextConverter::DumpText(m_adapter));
    }
} // namespace AzToolsFramework
