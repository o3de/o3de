/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <AzCore/DOM/Backends/JSON/JsonBackend.h>
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QAbstractItemModel>
#endif // Q_MOC_RUN

namespace AzToolsFramework
{
    class DPEDebugModel;

    class DPEModelNode : public QObject
    {
        Q_OBJECT;

    public:
        enum class NodeType
        {
            RootNode,
            RowNode,
            LabelNode,
            PropertyEditorNode
        };

        DPEModelNode(NodeType nodeType, size_t domValueIndex, QObject* theModel);
        ~DPEModelNode();

        int GetChildCount() const;
        int GetColumnChildCount() const;
        int GetMaxChildColumns() const;
        DPEModelNode* GetParentNode() const;
        QVariant GetData(int role) const;
        bool SetData(int role, const QVariant& value);
        Qt::ItemFlags GetFlags() const;
        DPEModelNode* GetChildNode(int childIndex);
        DPEModelNode* GetColumnNode(int columnIndex);

        int RowOfChild(DPEModelNode* const childNode) const;
        void Populate(const AZ::Dom::Value& domVal);

    private:
        DPEDebugModel* GetModel() const;
        DPEModelNode* AddChild(NodeType childType, size_t domValueIndex, bool isColumn, const QString& value);
        AZ::Dom::Value GetValue() const;

        NodeType m_type = NodeType::RootNode;
        DPEModelNode* m_parent = nullptr;
        DPEModelNode* m_columnParent = nullptr;
        QVector<DPEModelNode*> m_children;
        QVector<DPEModelNode*> m_columnChildren;
        int m_maxChildColumns = 1;
        size_t m_domValueIndex = 0;

        QString m_displayString;
    }; // DPEModelNode

    class DPEDebugModel : public QAbstractItemModel
    {
        Q_OBJECT;

    public:
        DPEDebugModel(QObject* parent);
        void SetAdapter(AZ::DocumentPropertyEditor::DocumentAdapter* theAdapter);
        auto* GetAdapter()
        {
            return m_adapter;
        }

        auto& GetBackend()
        {
            return m_jsonBackend;
        }

    protected:
        DPEModelNode* GetNodeFromIndex(const QModelIndex& theIndex) const;

        // QAbstractItemModel overrides
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& index) const override;
        int columnCount(const QModelIndex&) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    private:
        AZ::DocumentPropertyEditor::DocumentAdapter* m_adapter = nullptr;
        DPEModelNode* m_rootNode = nullptr;
        bool m_isResetting = false;
        AZ::Dom::JsonBackend<AZ::Dom::Json::ParseFlags::ParseComments, AZ::Dom::Json::OutputFormatting::MinifiedJson> m_jsonBackend;
    };
} // namespace AzToolsFramework
