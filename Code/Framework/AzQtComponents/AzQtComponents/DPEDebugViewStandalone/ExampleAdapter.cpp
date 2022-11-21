/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzQtComponents/DPEDebugViewStandalone/ExampleAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    ExampleAdapter::ExampleAdapter()
    {
        m_rowCount = m_columnCount = 4;
        ResizeMatrix();
    }

    Dom::Value ExampleAdapter::GenerateContents()
    {
        using Nodes::Button;
        using Nodes::IntSpinBox;
        using Nodes::PropertyEditor;

        AdapterBuilder builder;
        builder.BeginAdapter();

        builder.BeginRow();
        builder.Label("Config");
        builder.BeginRow();
        builder.Label("Row Count");
        builder.BeginPropertyEditor<IntSpinBox>(m_rowCount);
        builder.OnEditorChanged(
            [=](const Dom::Path&, const Dom::Value& value, Nodes::ValueChangeType)
            {
                m_rowCount = aznumeric_caster(value.GetInt64());
                ResizeMatrix();
            });
        builder.EndPropertyEditor();
        builder.Label("Column Count");
        builder.BeginPropertyEditor<IntSpinBox>(m_columnCount);
        builder.OnEditorChanged(
            [=](const Dom::Path&, const Dom::Value& value, Nodes::ValueChangeType)
            {
                m_columnCount = aznumeric_caster(value.GetInt64());
                ResizeMatrix();
            });
        builder.EndPropertyEditor();
        builder.EndRow();
        builder.EndRow();

        builder.BeginRow();
        builder.Label(AZStd::string::format("%ix%i Grid", m_rowCount, m_columnCount));
        builder.BeginRow();
        builder.Label("");
        for (int i = 0; i < m_columnCount; ++i)
        {
            builder.Label(AZStd::string::format("%i", i));
        }
        builder.EndRow();
        for (int i = 0; i < m_rowCount; ++i)
        {
            builder.BeginRow();
            builder.Label(AZStd::string::format("%i", i));
            for (int c = 0; c < m_columnCount; ++c)
            {
                builder.BeginPropertyEditor<IntSpinBox>(GetMatrixValue(i, c));
                builder.OnEditorChanged(
                    [=](const Dom::Path& path, const Dom::Value& value, Nodes::ValueChangeType)
                    {
                        SetMatrixValue(i, c, aznumeric_caster(value.GetInt64()));
                        NotifyContentsChanged({ Dom::PatchOperation::ReplaceOperation(path, value) });
                    });
                builder.EndPropertyEditor();
            }
            builder.EndRow();
        }
        builder.EndRow();

        struct EntryToProcess
        {
            ColorTreeNode* m_node = nullptr;
            AZStd::string m_label;
        };
        AZStd::deque<EntryToProcess> nodesToProcess{ EntryToProcess{ &m_colorTreeRoot, "Tree" } };
        while (!nodesToProcess.empty())
        {
            EntryToProcess entry = AZStd::move(nodesToProcess.front());
            nodesToProcess.pop_front();
            if (entry.m_node == nullptr)
            {
                builder.EndRow();
                continue;
            }
            builder.BeginRow();
            builder.Label(entry.m_label);
            builder.BeginPropertyEditor(Nodes::Color::Name, AZ::Dom::Utils::ValueFromType(entry.m_node->m_color));
            builder.OnEditorChanged(
                [=](const Dom::Path& path, const Dom::Value& value, Nodes::ValueChangeType)
                {
                    entry.m_node->m_color = AZ::Dom::Utils::ValueToType<AZ::Color>(value).value();
                    NotifyContentsChanged({ Dom::PatchOperation::ReplaceOperation(path, value) });
                });
            builder.EndPropertyEditor();

            if (entry.m_node->m_parent != nullptr)
            {
                builder.BeginPropertyEditor<Nodes::ContainerActionButton>();
                builder.Attribute(Nodes::PropertyEditor::SharePriorColumn, true);
                builder.Attribute(Nodes::ContainerActionButton::Action, Nodes::ContainerAction::RemoveElement);
                builder.CallbackAttribute(
                    Nodes::ContainerActionButton::OnActivate,
                    [=]()
                    {
                        for (auto it = entry.m_node->m_parent->m_children.begin(); it != entry.m_node->m_parent->m_children.end(); ++it)
                        {
                            if (&(*it) == entry.m_node)
                            {
                                entry.m_node->m_parent->m_children.erase(it);
                                break;
                            }
                        }
                        NotifyResetDocument();
                    });
                builder.EndPropertyEditor();
            }

            builder.BeginPropertyEditor<Nodes::ContainerActionButton>();
            builder.Attribute(Nodes::PropertyEditor::SharePriorColumn, true);
            builder.Attribute(Nodes::ContainerActionButton::Action, Nodes::ContainerAction::AddElement);
            builder.CallbackAttribute(
                Nodes::ContainerActionButton::OnActivate,
                [=]()
                {
                    ColorTreeNode child;
                    child.m_parent = entry.m_node;
                    entry.m_node->m_children.push_back(child);
                    NotifyResetDocument();
                });
            builder.EndPropertyEditor();

            int i = aznumeric_cast<int>(entry.m_node->m_children.size()) - 1;
            nodesToProcess.push_front({});
            for (auto it = entry.m_node->m_children.rbegin(); it != entry.m_node->m_children.rend(); ++it)
            {
                nodesToProcess.push_front({ &(*it), AZStd::string::format("%i", i--) });
            }
        }

        builder.EndAdapter();

        return builder.FinishAndTakeResult();
    }

    int ExampleAdapter::GetMatrixValue(int i, int j) const
    {
        return m_matrix[i][j];
    }

    void ExampleAdapter::SetMatrixValue(int i, int j, int value)
    {
        m_matrix[i][j] = value;
    }

    void ExampleAdapter::ResizeMatrix()
    {
        m_matrix.resize(m_rowCount);
        for (int i = 0; i < m_rowCount; ++i)
        {
            m_matrix[i].resize(m_columnCount);
        }
        NotifyResetDocument();
    }
} // namespace AZ::DocumentPropertyEditor
