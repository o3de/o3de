/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DPEDebugModel.h"
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzCore/DOM/DomUtils.h>

namespace AzToolsFramework
{
    DPEModelNode::DPEModelNode(DPEModelNode::NodeType nodeType, size_t domValueIndex, QObject* theModel)
        : QObject(theModel) // all nodes are owned (and cannot outlive)
        , m_type(nodeType)
        , m_domValueIndex(domValueIndex)
    {
    }

    DPEModelNode::~DPEModelNode()
    {
        for (auto iter = m_children.begin(); iter != m_children.end(); ++iter)
        {
            (*iter)->deleteLater();
        }
        for (auto iter = m_columnChildren.begin(); iter != m_columnChildren.end(); ++iter)
        {
            (*iter)->deleteLater();
        }
    }

    int DPEModelNode::GetChildCount() const
    {
        return m_children.size();
    }

    int DPEModelNode::GetColumnChildCount() const
    {
        return m_columnChildren.size();
    }

    int DPEModelNode::GetMaxChildColumns() const
    {
        return m_maxChildColumns;
    }

    DPEModelNode* DPEModelNode::GetParentNode() const
    {
        return m_parent;
    }

    QVariant DPEModelNode::GetData(int role) const
    {
        QVariant returnedData;
        if (role == Qt::DisplayRole || role == Qt::EditRole)
        {
            returnedData = m_displayString;
        }
        return returnedData;
    }

    bool DPEModelNode::SetData(int role, const QVariant& value)
    {
        bool succeeded = false;
        if (role == Qt::EditRole)
        {
            auto& jsonBackend = GetModel()->GetBackend();
            AZStd::string stringBuffer = value.toString().toUtf8().constData();
            auto writeOutcome = AZ::Dom::Utils::SerializedStringToValue(jsonBackend, stringBuffer, AZ::Dom::Lifetime::Temporary);
            
            // invoke the actual change on the Dom, it will come back to us as an update
            AZ::DocumentPropertyEditor::Nodes::PropertyEditor::OnChanged.InvokeOnDomNode(GetValue(), writeOutcome.GetValue());
        }
        return succeeded;
    }

    Qt::ItemFlags DPEModelNode::GetFlags() const
    {
        Qt::ItemFlags returnedFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

        if (m_type == NodeType::PropertyEditorNode)
        {
            returnedFlags |= Qt::ItemIsEditable;
        }

        return returnedFlags;
    }

    DPEModelNode* DPEModelNode::GetChildNode(int childIndex)
    {
        DPEModelNode* childNode = nullptr;
        if (childIndex >= 0 && childIndex < GetChildCount())
        {
            childNode = m_children[childIndex];
        }
        return childNode;
    }

    DPEModelNode* DPEModelNode::GetColumnNode(int columnIndex)
    {
        // column 0 is this node itself, subsequent columns are kept in m_columnChildren
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

    int DPEModelNode::RowOfChild(DPEModelNode* const childNode) const
    {
        return m_children.indexOf(childNode);
    }

    void DPEModelNode::Populate(const AZ::Dom::Value& domVal)
    {
        auto& jsonBackend = GetModel()->GetBackend();
        for (size_t arrayIndex = 0, numIndices = domVal.ArraySize(); arrayIndex < numIndices; ++arrayIndex)
        {
            auto& currentValue = domVal[arrayIndex];
            AZStd::string currentName = currentValue.GetNodeName().GetCStr();

            // "Row" children start new rows, and are themselves populated, other node types are additional colu
            if (currentName == AZ::DocumentPropertyEditor::Nodes::Row::Name)
            {
                DPEModelNode* newRow = AddChild(NodeType::RowNode, arrayIndex, false, QString::number(m_children.count()));
                newRow->Populate(currentValue);
            }
            else if (currentName == AZ::DocumentPropertyEditor::Nodes::Label::Name)
            {
                AZStd::string stringBuffer;
                AZ::Dom::Utils::ValueToSerializedString(jsonBackend, currentValue, stringBuffer);
                AddChild(NodeType::LabelNode, arrayIndex, true, QString::fromUtf8(stringBuffer.c_str()));
            }
            else if (currentName == AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Name)
            {
                AZStd::string stringBuffer;
                AZ::Dom::Utils::ValueToSerializedString(jsonBackend, currentValue.GetNodeValue(), stringBuffer);
                AddChild(NodeType::PropertyEditorNode, arrayIndex, true, QString::fromUtf8(stringBuffer.c_str()));
            }
        }
    }

    DPEDebugModel* DPEModelNode::GetModel() const
    {
        // the creating model is the owner of all model nodes in the QObject hierarchy
        return static_cast<DPEDebugModel*>(parent());
    }

    DPEModelNode* DPEModelNode::AddChild(NodeType childType, size_t domValueIndex, bool isColumn, const QString& value)
    {
        DPEModelNode* newChild = new DPEModelNode(childType, domValueIndex, parent());
        
        // new actual children have this node as a parent, new columns share the same parent as this node
        DPEModelNode* assignedParent = (isColumn ? GetParentNode() : this);
        newChild->m_parent = assignedParent;
        newChild->m_displayString = value;

        if (isColumn)
        {
            newChild->m_columnParent = this;
            m_columnChildren.push_back(newChild);
            const int numCols = m_columnChildren.size() + 1;
            if (assignedParent->GetMaxChildColumns() < numCols)
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

    AZ::Dom::Value DPEModelNode::GetValue() const
    {
        QVector<size_t> reversePath;
        const DPEModelNode* currNode = this;

        // traverse upwards to the root determine the path to the current node
        bool traversingUpwards = true;
        while (traversingUpwards)
        {
            if (currNode->m_columnParent)
            {
                reversePath << currNode->m_domValueIndex;
                currNode = currNode->m_columnParent;
            }
            else if (currNode->m_parent)
            {
                reversePath << currNode->m_domValueIndex;
                currNode = currNode->m_parent;
            }
            else
            {
                traversingUpwards = false;
            }
        }
        // start at the top of the adapter and move down the inverse of the path we took traversing upwards
        auto returnValue = GetModel()->GetAdapter()->GetContents();
        for (auto reverseIter = reversePath.rbegin(); reverseIter != reversePath.rend(); ++reverseIter)
        {
            returnValue = returnValue[*reverseIter];
        }

        return returnValue;
    }

    DPEDebugModel::DPEDebugModel(QObject* parent)
        : QAbstractItemModel(parent)
    {
    }

    void DPEDebugModel::SetAdapter(AZ::DocumentPropertyEditor::DocumentAdapter* theAdapter)
    {
        m_adapter = theAdapter;
        m_isResetting = (m_rootNode != nullptr);
        if (m_isResetting)
        {
            beginResetModel();
            m_rootNode->deleteLater(); // recursively destroys the existing tree
        }

        // recursively populate the node tree with the new adapter information
        m_rootNode = new DPEModelNode(DPEModelNode::NodeType::RootNode, 0, this);
        m_rootNode->Populate(theAdapter->GetContents());

        if (m_isResetting)
        {
            endResetModel();
            m_isResetting = false;
        }
        else
        {
            // inform any listening views that top-level rows have been added,
            // they will automatically traverse the child nodes
            const int numRootChildren = m_rootNode->GetChildCount();
            if (numRootChildren)
            {
                beginInsertRows(QModelIndex(), 0, numRootChildren - 1);
                endInsertRows();
            }
        }
    }

    DPEModelNode* DPEDebugModel::GetNodeFromIndex(const QModelIndex& theIndex) const
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
            if (addressedNode->GetParentNode()->GetChildNode(theIndex.row())->GetColumnNode(theIndex.column()) == addressedNode)
            {
                return addressedNode;
            }
        }
        return returnedNode;
    }

    QVariant DPEDebugModel::data(const QModelIndex& index, int role) const
    {
        QVariant returnedData;
        auto theNode = GetNodeFromIndex(index);

        if (theNode)
        {
            returnedData = theNode->GetData(role);
        }
        return returnedData;
    }

    bool DPEDebugModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        bool succeeded = false;
        auto theNode = GetNodeFromIndex(index);
        if (theNode)
        {
            succeeded = theNode->SetData(role, value);
        }
        return succeeded;
    }

    Qt::ItemFlags DPEDebugModel::flags(const QModelIndex& passedIndex) const
    {
        Qt::ItemFlags returnedFlags = Qt::NoItemFlags;
        auto* theNode = GetNodeFromIndex(passedIndex);
        if (theNode)
        {
            returnedFlags = theNode->GetFlags();
        }
        return returnedFlags;
    }

    QModelIndex DPEDebugModel::index(int row, int column, const QModelIndex& parentIndex) const
    {
        QModelIndex returnedIndex;
        auto* parentNode = GetNodeFromIndex(parentIndex);

        if (parentNode && row < parentNode->GetChildCount())
        {
            auto* rowNode = parentNode->GetChildNode(row);
            auto* columnNode = rowNode->GetColumnNode(column);

            // if there's no actual node for the given column, still give the index an internal pointer of the row node,
            // so that the QModelIndex has enough info for functions like parent() to function properly. This does complicate
            // getNodeFromIndex though, which now needs to verify that the internal pointer is the actual node at that column
            returnedIndex = createIndex(row, column, static_cast<void*>(columnNode ? columnNode : rowNode));
        }
        return returnedIndex;
    }

    QModelIndex DPEDebugModel::parent(const QModelIndex& passedIndex) const
    {
        QModelIndex parentIndex;

        if (passedIndex.isValid())
        {
            auto* theNode = static_cast<DPEModelNode*>(passedIndex.internalPointer());
            auto* parentNode = theNode->GetParentNode();

            if (parentNode != m_rootNode) // if the parent is root, parentIndex is already correctly set as invalid
            {
                auto* grandparent = parentNode->GetParentNode();
                if (grandparent)
                {
                    parentIndex = createIndex(grandparent->RowOfChild(parentNode), 0, parentNode);
                }
            }
        }

        return parentIndex;
    }

    int DPEDebugModel::columnCount(const QModelIndex& parent) const
    {
        int columnCount = 0;

        DPEModelNode* theNode = (parent.isValid() ? GetNodeFromIndex(parent) : m_rootNode);
        if (theNode)
        {
            columnCount = theNode->GetMaxChildColumns();
        }
        return columnCount;
    }

    int DPEDebugModel::rowCount(const QModelIndex& parent) const
    {
        int rowCount = 0;

        DPEModelNode* theNode = (parent.isValid() ? GetNodeFromIndex(parent) : m_rootNode);
        if (theNode)
        {
            rowCount = theNode->GetChildCount();
        }
        return rowCount;
    }
} // namespace AzToolsFramework

// #include "UI/moc_DPEDebugModel.cpp"
