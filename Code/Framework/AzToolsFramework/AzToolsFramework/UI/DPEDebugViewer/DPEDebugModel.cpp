/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzCore/DOM/DomUtils.h>
#include "DPEDebugModel.h"

namespace AzToolsFramework
{
    DPEModelNode::DPEModelNode(DPEModelNode::NodeType nodeType, QObject* theModel)
        : QObject(theModel) // all nodes are owned (and destroyed by the model)
        , m_type(nodeType)
    {
    }

    int DPEModelNode::getNumChildren()
    {
        return m_children.size();
    }

    int DPEModelNode::getNumChildColumns()
    {
        return m_maxChildColumns;
    }

    DPEModelNode* DPEModelNode::getParentNode()
    {
        return m_parent;
    }

    QVariant DPEModelNode::getData(int role)
    {
        QVariant returnedData;
        if (role == Qt::DisplayRole || role == Qt::EditRole)
        {
            returnedData = m_data;
        }
        return returnedData;
    }

    bool DPEModelNode::setData(int role, const QVariant& value)
    {
        bool succeeded = false;
        if (role == Qt::EditRole)
        {
            auto& jsonBackend = getModel()->getBackend();
            AZStd::string stringBuffer = value.toString().toUtf8().constData();
            auto writeOutcome = AZ::Dom::Utils::SerializedStringToValue(jsonBackend, stringBuffer, AZ::Dom::Lifetime::Temporary);
            //AZ::DocumentPropertyEditor::Nodes::PropertyEditor::OnChanged.InvokeOnDomNode(currNodeVal, writeOutcome.GetValue());
        }
        return succeeded;
    }

    Qt::ItemFlags DPEModelNode::getFlags()
    {
        Qt::ItemFlags returnedFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

        if (m_type == kPropertyEditorNode)
        {
            returnedFlags |= Qt::ItemIsEditable;
        }

        return returnedFlags;
    }

    DPEModelNode* DPEModelNode::getChildNode(int childIndex)
    {
        DPEModelNode* childNode = nullptr;
        if (childIndex >= 0 && childIndex < getNumChildren())
        {
            childNode = m_children[childIndex];
        }
        return childNode;
    }

    DPEModelNode* DPEModelNode::getColumnNode(int columnIndex)
    {
        DPEModelNode* returnedNode = nullptr;
        if (columnIndex == 0)
        {
            returnedNode = this;
        }
        else if (columnIndex <= m_columnChildren.count())
        {
            returnedNode = m_columnChildren[columnIndex - 1];
        }
        return returnedNode;
    }

    int DPEModelNode::rowOfChild(DPEModelNode* const childNode)
    {
        return m_children.indexOf(childNode);
    }

    DPEModelNode* DPEModelNode::takeChildNode(int childIndex)
    {
        return m_children.takeAt(childIndex);
    }

    void DPEModelNode::populate(const AZ::Dom::Value& domVal)
    {
        auto& jsonBackend = getModel()->getBackend();
        for (auto iter = domVal.ArrayBegin(); iter != domVal.ArrayEnd(); ++iter)
        {
            auto& currValue = *iter;
            AZStd::string theName = currValue.GetNodeName().GetCStr();

            if (theName == AZ::DocumentPropertyEditor::Nodes::Row::Name)
            {
                DPEModelNode* newRow = addChild(KRowNode, false, QString::number(m_children.count()));
                newRow->populate(currValue);
            }
            else if (theName == AZ::DocumentPropertyEditor::Nodes::Label::Name)
            {
                AZStd::string stringBuffer;
                AZ::Dom::Utils::ValueToSerializedString(jsonBackend, currValue, stringBuffer);
                addChild(kLabelNode, true, QString::fromUtf8(stringBuffer.c_str()));
            }
            else if (theName == AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Name)
            {
                AZStd::string stringBuffer;
                AZ::Dom::Utils::ValueToSerializedString(jsonBackend, currValue.GetNodeValue(), stringBuffer);
                addChild(kPropertyEditorNode, true, QString::fromUtf8(stringBuffer.c_str()));
            }
        }
    }

    DPEDebugModel* DPEModelNode::getModel()
    {
        // the creating model is the owner of all model nodes in the QObject hierarchy
        return static_cast<DPEDebugModel*>(parent());
    }

    DPEModelNode* DPEModelNode::addChild(NodeType childType, bool isColumn, const QString& value)
    {
        DPEModelNode* newChild = new DPEModelNode(childType, parent());
        DPEModelNode* assignedParent = (isColumn ? getParentNode() : this);
        newChild->m_parent = assignedParent;
        newChild->m_data = value;

        if (isColumn)
        {
            m_columnChildren.push_back(newChild);
            const int numCols = m_columnChildren.size() + 1;
            if (assignedParent->getNumChildColumns() < numCols)
            {
                assignedParent->m_maxChildColumns = numCols;
            }
        }
        else
        {
            m_children.push_back(newChild);
        }

        return newChild;
    }

    DPEDebugModel::DPEDebugModel(QObject* parent)
        : QAbstractItemModel(parent)
    {
        m_rootNode = new DPEModelNode(DPEModelNode::kRootNode, this);
    }

    void DPEDebugModel::setAdapter(AZ::DocumentPropertyEditor::DocumentAdapter* theAdapter)
    {
        const int numExistingChildren = m_rootNode->getNumChildren();
        m_isResetting = (numExistingChildren > 0);
        if (m_isResetting)
        {
            beginResetModel();
            for (int index = numExistingChildren - 1; index >= 0; ++index)
            {
                m_rootNode->takeChildNode(index)->deleteLater();
            }
        }

        auto theContents = theAdapter->GetContents();
        m_rootNode->populate(theAdapter->GetContents());

        if (m_isResetting)
        {
            endResetModel();
            m_isResetting = false;
        }
        else
        {
            const int numRootChildren = m_rootNode->getNumChildren();
            if (numRootChildren)
            {
                beginInsertRows(QModelIndex(), 0, numRootChildren - 1);
                endInsertRows();
            }
        }
    }

    DPEModelNode* DPEDebugModel::getNodeFromIndex(const QModelIndex& theIndex) const
    {
        DPEModelNode* returnedNode = nullptr;
        if (!theIndex.isValid())
        {
            returnedNode = m_rootNode;
        }
        else
        {
            // only return the internally addressed node if it's the correct node for the column,
            // otherwise fall through to returning null
            DPEModelNode* addressedNode = static_cast<DPEModelNode*>(theIndex.internalPointer());
            if (addressedNode->getParentNode()->getChildNode(theIndex.row())->getColumnNode(theIndex.column()) == addressedNode)
            {
                return addressedNode;
            }
        }
        return returnedNode;
    }

    QVariant DPEDebugModel::data(const QModelIndex& index, int role) const
    {
        QVariant returnedData;
        auto theNode = getNodeFromIndex(index);

        if (theNode)
        {
            returnedData = theNode->getData(role);
        }
        return returnedData;
    }

    bool DPEDebugModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        bool succeeded = false;
        auto theNode = getNodeFromIndex(index);
        if (theNode)
        {
            succeeded = theNode->setData(role, value);
        }
        return succeeded;
    }

    Qt::ItemFlags DPEDebugModel::flags(const QModelIndex& passedIndex) const
    {
        Qt::ItemFlags returnedFlags = Qt::NoItemFlags;
        auto* theNode = getNodeFromIndex(passedIndex);
        if (theNode)
        {
            returnedFlags = theNode->getFlags();
        }
        return returnedFlags;
    }

    QModelIndex DPEDebugModel::index(int row, int column, const QModelIndex& parentIndex) const
    {
        QModelIndex returnedIndex;
        auto* parentNode = getNodeFromIndex(parentIndex);

        if (parentNode && row < parentNode->getNumChildren())
        {
            auto* rowNode = parentNode->getChildNode(row);
            auto* colNode = rowNode->getColumnNode(column);

            // if there's no actual node for the given column, still give the index an internal pointer of the row node,
            // so that the QModelIndex has enough info for functions like parent() to function properly. This does complicate
            // getNodeFromIndex though, which now needs to verify that the internal pointer is the actual node at that column
            returnedIndex = createIndex(row, column, static_cast<void*>(colNode ? colNode : rowNode));
        }
        return returnedIndex;
    }

    QModelIndex DPEDebugModel::parent(const QModelIndex& passedIndex) const
    {
        QModelIndex parentIndex;

        if (passedIndex.isValid())
        {
            auto* theNode = static_cast<DPEModelNode*>(passedIndex.internalPointer());
            auto* parentNode = theNode->getParentNode();

            if (parentNode != m_rootNode) // if the parent is root, parentIndex is already correctly set as invalid
            {
                auto* grandParent = parentNode->getParentNode();
                if (grandParent)
                {
                    parentIndex = createIndex(grandParent->rowOfChild(parentNode), 0, parentNode);
                }
            }
        }

        return parentIndex;
    }

    int DPEDebugModel::columnCount(const QModelIndex& parent) const
    {
        int numColumns = 0;

        DPEModelNode* theNode = (parent.isValid() ? getNodeFromIndex(parent) : m_rootNode);
        if (theNode)
        {
            numColumns = theNode->getNumChildColumns();
        }
        return numColumns;
    }

    int DPEDebugModel::rowCount(const QModelIndex& parent) const
    {
        int numRows = 0;

        DPEModelNode* theNode = (parent.isValid() ? getNodeFromIndex(parent) : m_rootNode);
        if (theNode)
        {
            numRows = theNode->getNumChildren();
        }
        return numRows;
    }
} // namespace AzToolsFramework

// #include "UI/moc_DPEDebugModel.cpp"
