/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AZFramework/DocumentPropertyEditor/DocumentAdapter.h>
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
        enum NodeType
        {
            kRootNode = 0,
            KRowNode,
            kLabelNode,
            kPropertyEditorNode
        };

        DPEModelNode(NodeType nodeType, QObject* theModel);

        int getNumChildren();
        int getNumChildColumns();
        DPEModelNode* getParentNode();
        QVariant getData(int role);
        bool setData(int role, const QVariant& value);
        Qt::ItemFlags getFlags();
        DPEModelNode* getChildNode(int childIndex);
        DPEModelNode* getColumnNode(int columnIndex);

        int rowOfChild(DPEModelNode* const childNode);
        DPEModelNode* takeChildNode(int childIndex);
        void populate(const AZ::Dom::Value& domVal);

    private:
        DPEDebugModel* getModel();
        DPEModelNode* addChild(NodeType childType, bool isColumn, const QString& value);

        NodeType m_type = kRootNode;
        DPEModelNode* m_parent = nullptr;
        QVector<DPEModelNode*> m_children;
        QVector<DPEModelNode*> m_columnChildren;
        int m_maxChildColumns = 1;

        QString m_data;
    }; // DPEModelNode

    class DPEDebugModel : public QAbstractItemModel
    {
        Q_OBJECT;

    public:
        DPEDebugModel(QObject* parent);
        void setAdapter(AZ::DocumentPropertyEditor::DocumentAdapter* theAdapter);
        auto& getBackend()
        {
            return m_jsonBackend;
        }

    protected:
        DPEModelNode* getNodeFromIndex(const QModelIndex& theIndex) const;

        // QAbstractItemModel overrides
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& index) const override;
        int columnCount(const QModelIndex&) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    private:
        DPEModelNode* m_rootNode;
        bool m_isResetting = false;
        AZ::Dom::JsonBackend<AZ::Dom::Json::ParseFlags::ParseComments, AZ::Dom::Json::OutputFormatting::MinifiedJson> m_jsonBackend;
    };
} // namespace AzQtComponents
