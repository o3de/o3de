/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/DOM/Backends/JSON/JsonBackend.h>
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
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

        DPEModelNode(QObject* theModel);
        ~DPEModelNode();

        int GetRowChildCount() const;
        int GetColumnChildCount() const;
        DPEModelNode* GetParentNode() const;
        DPEModelNode* GetParentRowNode() const;

        QVariant GetData(int role) const;
        bool SetData(int role, const QVariant& value);
        Qt::ItemFlags GetFlags() const;
        DPEModelNode* GetRowChild(int childIndex);
        DPEModelNode* GetColumnChild(int columnIndex);

        DPEModelNode* GetChildFromDomIndex(size_t domIndex);

        int RowOfChild(DPEModelNode* const childNode) const;
        int ColumnOfChild(DPEModelNode* const childNode) const;

        void SetValue(const AZ::Dom::Value& domVal, bool notifyView);
        AZ::Dom::Value GetValueFromDom() const;

        size_t GetDomValueIndex() const
        {
            return m_domValueIndex;
        }

        bool IsColumn() const
        {
            return !(m_type == NodeType::RootNode || m_type == NodeType::RowNode);
        }

    private:
        DPEDebugModel* GetModel() const;
        void AddChild(DPEModelNode* childNode, size_t domValueIndex);
        void ClearChildren();

        NodeType m_type = NodeType::RootNode;
        DPEModelNode* m_parent = nullptr;
        size_t m_domValueIndex = 0;
        QString m_displayString;

        QVector<DPEModelNode*> m_rowChildren;
        QVector<DPEModelNode*> m_columnChildren;
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

        QModelIndex GetIndexFromNode(DPEModelNode* const theNode) const;

        void SetMaxColumns(int newMax)
        {
            m_maxColumns = newMax;
        }
        int GetMaxColumns() const
        {
            return m_maxColumns;
        }

        using QAbstractItemModel::beginInsertRows;
        using QAbstractItemModel::beginRemoveRows;
        using QAbstractItemModel::endInsertRows;
        using QAbstractItemModel::endRemoveRows;

    protected:
        DPEModelNode* GetNodeFromIndex(const QModelIndex& theIndex) const;
        DPEModelNode* GetNodeFromPath(const AZ::Dom::Path& thePath) const;

        // QAbstractItemModel overrides
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& index) const override;
        int columnCount(const QModelIndex&) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;

        void HandleReset();
        void HandleDomChange(const AZ::Dom::Patch& patch);

    private:
        AZ::DocumentPropertyEditor::DocumentAdapter* m_adapter = nullptr;
        DPEModelNode* m_rootNode = nullptr;
        int m_maxColumns = 1;

        AZ::Dom::JsonBackend<AZ::Dom::Json::ParseFlags::ParseComments, AZ::Dom::Json::OutputFormatting::MinifiedJson> m_jsonBackend;
        AZ::DocumentPropertyEditor::DocumentAdapter::ResetEvent::Handler m_resetHandler;
        AZ::DocumentPropertyEditor::DocumentAdapter::ChangedEvent::Handler m_changedHandler;
    };
} // namespace AzToolsFramework
