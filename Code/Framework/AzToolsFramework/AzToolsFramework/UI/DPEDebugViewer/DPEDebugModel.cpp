/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DPEDebugModel.h"
#include <AzCore/DOM/DomUtils.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>

namespace AzToolsFramework
{
    DPEModelNode::DPEModelNode(QObject* theModel)
        : QObject(theModel) // all nodes are owned (and cannot outlive)
    {
    }

    DPEModelNode::~DPEModelNode()
    {
        ClearChildren();
    }

    int DPEModelNode::GetRowChildCount() const
    {
        return m_rowChildren.size();
    }

    int DPEModelNode::GetColumnChildCount() const
    {
        return m_columnChildren.size();
    }

    DPEModelNode* DPEModelNode::GetParentNode() const
    {
        return m_parent;
    }

    DPEModelNode* DPEModelNode::GetParentRowNode() const
    {
        // returns the parent row. If this is a column,
        // its direct parent is its row, and its grandparent is its parent row
        DPEModelNode* returnedNode = nullptr;
        if (IsColumn())
        {
            AZ_Assert(m_parent != nullptr, "A column node must have a parent set!");
            if (m_parent)
            {
                returnedNode = m_parent->m_parent;
            }
        }
        else
        {
            returnedNode = m_parent;
        }
        return returnedNode;
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
            AZ::DocumentPropertyEditor::Nodes::PropertyEditor::OnChanged.InvokeOnDomNode(
                GetValueFromDom(),
                writeOutcome.GetValue(),
                AZ::DocumentPropertyEditor::Nodes::ValueChangeType::FinishedEdit);
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

    DPEModelNode* DPEModelNode::GetRowChild(int childIndex)
    {
        DPEModelNode* childNode = nullptr;
        if (childIndex >= 0 && childIndex < GetRowChildCount())
        {
            childNode = m_rowChildren[childIndex];
        }
        return childNode;
    }

    DPEModelNode* DPEModelNode::GetColumnChild(int columnIndex)
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

    DPEModelNode* DPEModelNode::GetChildFromDomIndex(size_t domIndex)
    {
        // search children for node that has the given dom index
        // These are almost always tiny lists, so using a Map would be overkill and almost certainly slower in practice
        DPEModelNode* returnedNode = nullptr;

        // search row and column children for the node with the given DOM index
        for (auto rowIterator = m_rowChildren.begin(), endIterator = m_rowChildren.end();
             returnedNode == nullptr && rowIterator != endIterator; ++rowIterator)
        {
            if ((*rowIterator)->GetDomValueIndex() == domIndex)
            {
                returnedNode = *rowIterator;
            }
        }
        for (auto columnIterator = m_columnChildren.begin(), endIterator = m_columnChildren.end();
             returnedNode == nullptr && columnIterator != endIterator; ++columnIterator)
        {
            if ((*columnIterator)->GetDomValueIndex() == domIndex)
            {
                returnedNode = *columnIterator;
            }
        }

        return returnedNode;
    }

    int DPEModelNode::RowOfChild(DPEModelNode* const childNode) const
    {
        return m_rowChildren.indexOf(childNode);
    }

    int DPEModelNode::ColumnOfChild(DPEModelNode* const childNode) const
    {
        return m_columnChildren.indexOf(childNode);
    }

    void DPEModelNode::SetValue(const AZ::Dom::Value& domVal, bool notifyView)
    {
        if (!domVal.IsNode())
        {
            AZ_Warning("DPE", false, "Received a non-node value");
            return;
        }
        auto domName = domVal.GetNodeName().GetStringView();

        // these will only be used if we're notifying the view
        DPEDebugModel* theModel = nullptr;
        QModelIndex myIndex;
        if (notifyView)
        {
            theModel = GetModel();
            myIndex = theModel->GetIndexFromNode(this);
        }

        // clear any children before repopulating
        if (!m_rowChildren.isEmpty() || !m_columnChildren.isEmpty())
        {
            if (notifyView && !m_rowChildren.isEmpty())
            {
                theModel->beginRemoveRows(myIndex, 0, GetRowChildCount() - 1);
                ClearChildren();
                theModel->endRemoveRows();
            }
            else
            {
                ClearChildren();
            }
        }

        // populate type and string, if applicable
        if (domName == AZ::DocumentPropertyEditor::Nodes::Adapter::Name)
        {
            m_type = NodeType::RootNode;
        }
        else if (domName == AZ::DocumentPropertyEditor::Nodes::Row::Name)
        {
            m_type = NodeType::RowNode;
        }
        else if (domName == AZ::DocumentPropertyEditor::Nodes::Label::Name)
        {
            m_type = NodeType::LabelNode;
            auto labelString = domVal[AZ::DocumentPropertyEditor::Nodes::Label::Value.GetName()].GetString();
            m_displayString = QString::fromUtf8(labelString.data(), static_cast<int>(labelString.size()));
        }
        else if (domName == AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Name)
        {
            m_type = NodeType::PropertyEditorNode;
            auto& jsonBackend = GetModel()->GetBackend();
            AZStd::string stringBuffer;
            auto editorValue = AZ::DocumentPropertyEditor::Nodes::PropertyEditor::Value.ExtractFromDomNode(domVal);
            if (editorValue.has_value())
            {
                AZ::Dom::Utils::ValueToSerializedString(
                    jsonBackend, editorValue.value(), stringBuffer);
            }
            m_displayString = QString::fromUtf8(stringBuffer.data(), static_cast<int>(stringBuffer.size()));
        }

        if (m_type == NodeType::RootNode || m_type == NodeType::RowNode)
        {
            // populate all direct children of adapters or rows
            for (size_t arrayIndex = 0, numIndices = domVal.ArraySize(); arrayIndex < numIndices; ++arrayIndex)
            {
                auto& childValue = domVal[arrayIndex];
                auto* newNode = new DPEModelNode(parent());
                newNode->SetValue(childValue, false);
                AddChild(newNode, arrayIndex);
            }

            if (notifyView)
            {
                // notify the model that all this row's data may have changed, and that new children have been added
                theModel->dataChanged(myIndex, myIndex.sibling(myIndex.row(), theModel->GetMaxColumns() - 1));
                const int childrenAdded = GetRowChildCount();
                if (childrenAdded != 0)
                {
                    theModel->beginInsertRows(myIndex, 0, childrenAdded - 1);
                    theModel->endInsertRows();
                }
            }
        }
    }

    DPEDebugModel* DPEModelNode::GetModel() const
    {
        // the creating model is the owner of all model nodes in the QObject hierarchy
        return static_cast<DPEDebugModel*>(parent());
    }

    void DPEModelNode::AddChild(DPEModelNode* childNode, size_t domValueIndex)
    {
        childNode->m_domValueIndex = domValueIndex;
        childNode->m_parent = this;

        if (childNode->IsColumn())
        {
            m_columnChildren.push_back(childNode);
            const int numCols = m_columnChildren.size() + 1;
            auto* theModel = GetModel();
            if (theModel->GetMaxColumns() < numCols)
            {
                theModel->SetMaxColumns(numCols);
            }
        }
        else
        {
            // for now, a row's string is just its row index
            childNode->m_displayString = QString::number(m_rowChildren.count());
            m_rowChildren.push_back(childNode);
        }
    }

    AZ::Dom::Value DPEModelNode::GetValueFromDom() const
    {
        QVector<size_t> reversePath;
        const DPEModelNode* currNode = this;

        // traverse upwards to the root determine the path to the current node
        bool traversingUpwards = true;
        while (traversingUpwards)
        {
            if (currNode->m_parent)
            {
                reversePath << currNode->GetDomValueIndex();
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

    void DPEModelNode::ClearChildren()
    {
        for (auto iter = m_rowChildren.begin(); iter != m_rowChildren.end(); ++iter)
        {
            (*iter)->deleteLater();
        }
        for (auto iter = m_columnChildren.begin(); iter != m_columnChildren.end(); ++iter)
        {
            (*iter)->deleteLater();
        }
        m_rowChildren.clear();
        m_columnChildren.clear();
    }

    DPEDebugModel::DPEDebugModel(QObject* parent)
        : QAbstractItemModel(parent)
    {
    }

    void DPEDebugModel::SetAdapter(AZ::DocumentPropertyEditor::DocumentAdapterPtr theAdapter)
    {
        m_adapter = theAdapter;
        if (m_adapter == nullptr)
        {
            // Clear out the event handlers when a nullptr DocumentAdapter is suppleid
            m_resetHandler = {};
            m_changedHandler = {};
            return;
        };
        m_resetHandler = AZ::DocumentPropertyEditor::DocumentAdapter::ResetEvent::Handler(
            [this]()
            {
                this->HandleReset();
            });
        m_adapter->ConnectResetHandler(m_resetHandler);

        m_changedHandler = AZ::DocumentPropertyEditor::DocumentAdapter::ChangedEvent::Handler(
            [this](const AZ::Dom::Patch& patch)
            {
                this->HandleDomChange(patch);
            });
        m_adapter->ConnectChangedHandler(m_changedHandler);

        // populate the model, just like a reset
        HandleReset();
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
            if (addressedNode->GetParentRowNode()->GetRowChild(theIndex.row())->GetColumnChild(theIndex.column()) == addressedNode)
            {
                return addressedNode;
            }
        }
        return returnedNode;
    }

    QModelIndex DPEDebugModel::GetIndexFromNode(DPEModelNode* const theNode) const
    {
        if (theNode && theNode != m_rootNode)
        {
            int column = 0;
            DPEModelNode* rowNode = nullptr;
            if (theNode->IsColumn())
            {
                rowNode = theNode->GetParentNode();
                column = rowNode->ColumnOfChild(theNode);
            }
            else
            {
                rowNode = theNode;
            }

            DPEModelNode* rowParent = rowNode->GetParentRowNode();
            const int row = rowParent->RowOfChild(rowNode);
            return createIndex(row, column, static_cast<void*>(theNode));
        }
        else
        {
            // the root is by definition indexed by a default constructed QModelIndex
            return QModelIndex();
        }
    }

    DPEModelNode* DPEDebugModel::GetNodeFromPath(const AZ::Dom::Path& thePath) const
    {
        DPEModelNode* returnedNode = m_rootNode;
        for (auto pathIter = thePath.begin(), endIter = thePath.end();
             pathIter != endIter && returnedNode != nullptr && pathIter->IsIndex();
             ++pathIter)
        {
            // non-index sub-paths are for properties not nodes, so only handle the index paths
            returnedNode = returnedNode->GetChildFromDomIndex(pathIter->GetIndex());
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

        if (parentNode && row < parentNode->GetRowChildCount())
        {
            auto* rowNode = parentNode->GetRowChild(row);
            auto* columnNode = rowNode->GetColumnChild(column);

            // if there's no actual node for the given column, still give the index an internal pointer of the row node,
            // so that the QModelIndex has enough info for functions like parent() to function properly. This does complicate
            // GetNodeFromIndex though, which now needs to verify that the internal pointer is the actual node at that column
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
            auto* parentNode = theNode->GetParentRowNode();

            if (parentNode != m_rootNode) // if the parent is root, parentIndex is already correctly set as invalid
            {
                auto* grandparent = parentNode->GetParentRowNode();
                if (grandparent)
                {
                    parentIndex = createIndex(grandparent->RowOfChild(parentNode), 0, parentNode);
                }
            }
        }

        return parentIndex;
    }

    int DPEDebugModel::columnCount([[maybe_unused]] const QModelIndex& parent) const
    {
        return m_maxColumns;
    }

    int DPEDebugModel::rowCount(const QModelIndex& parent) const
    {
        int rowCount = 0;

        DPEModelNode* theNode = (parent.isValid() ? GetNodeFromIndex(parent) : m_rootNode);
        if (theNode)
        {
            rowCount = theNode->GetRowChildCount();
        }
        return rowCount;
    }

    void DPEDebugModel::HandleReset()
    {
        beginResetModel();

        if (!m_rootNode)
        {
            m_rootNode = new DPEModelNode(this);
        }

        // recursively populate the node tree with the new adapter information
        m_rootNode->SetValue(m_adapter->GetContents(), false);

        endResetModel();
    }

    void DPEDebugModel::HandleDomChange(const AZ::Dom::Patch& patch)
    {
        auto fullDom = m_adapter->GetContents();
        for (auto operationIterator = patch.begin(), endIterator = patch.end(); operationIterator != endIterator; ++operationIterator)
        {
            if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Replace)
            {
                // replace operations on a DOM can be surprisingly complicated, like if a column is replaced by a row, or vice versa
                // the safest method is to get the full row of the node, remove it, update it, and put it back into place
                auto* destinationNode = GetNodeFromPath(operationIterator->GetDestinationPath());
                AZ_Assert(destinationNode, "received patch for non-existent node!");
                auto* owningRow = destinationNode->GetParentNode();
                if (owningRow && owningRow != m_rootNode)
                {
                    auto rowIndex = GetIndexFromNode(owningRow);
                    auto parentIndex = rowIndex.parent();
                    auto rowNumber = rowIndex.row();
                    beginRemoveRows(parentIndex, rowNumber, rowNumber);
                    owningRow->SetValue(owningRow->GetValueFromDom(), false);
                    endRemoveRows();
                    beginInsertRows(parentIndex, rowNumber, rowNumber);
                    endInsertRows();
                }
                else // update is at the top level, reset the whole thing
                {
                    HandleReset();
                    break;
                }
            }
        }
    }

} // namespace AzToolsFramework
