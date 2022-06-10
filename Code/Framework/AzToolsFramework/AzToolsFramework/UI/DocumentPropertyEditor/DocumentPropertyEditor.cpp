/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "DocumentPropertyEditor.h"

#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <QLineEdit>
#include <QVBoxLayout>

#include <AzCore/DOM/DomUtils.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyEditorToolsSystemInterface.h>

namespace AzToolsFramework
{
    // helper method used by both the view and row widgets to empty layouts
    static void DestroyLayoutContents(QLayout* layout)
    {
        while (auto layoutItem = layout->takeAt(0))
        {
            auto subWidget = layoutItem->widget();
            if (subWidget)
            {
                delete subWidget;
            }
            delete layoutItem;
        }
    }

    DPELayout::DPELayout(int depth, QWidget* parentWidget)
        : QHBoxLayout(parentWidget)
        , m_depth(depth)
    {
    }

    QSize DPELayout::sizeHint() const
    {
        int cumulativeWidth = 0;
        int preferredHeight = 0;

        // sizeHint for this horizontal layout is the sum of the preferred widths,
        // and the maximum of the preferred heights
        for (int layoutIndex = 0; layoutIndex < count(); ++layoutIndex)
        {
            auto widgetSizeHint = itemAt(layoutIndex)->sizeHint();
            cumulativeWidth += widgetSizeHint.width();
            preferredHeight = AZStd::max(widgetSizeHint.height(), preferredHeight);
        }
        return { cumulativeWidth, preferredHeight };
    }

    QSize DPELayout::minimumSize() const
    {
        int cumulativeWidth = 0;
        int minimumHeight = 0;

        // minimumSize for this horizontal layout is the sum of the min widths,
        // and the maximum of the preferred heights
        for (int layoutIndex = 0; layoutIndex < count(); ++layoutIndex)
        {
            QWidget* widgetChild = itemAt(layoutIndex)->widget();
            if (widgetChild)
            {
                const auto minWidth = widgetChild->minimumSizeHint().width();
                if (minWidth > 0)
                {
                    cumulativeWidth += minWidth;
                }
                minimumHeight = AZStd::max(widgetChild->sizeHint().height(), minimumHeight);
            }
        }
        return { cumulativeWidth, minimumHeight };
    }

    void DPELayout::setGeometry(const QRect& rect)
    {
        QLayout::setGeometry(rect);

        // todo: implement QSplitter-like functionality to allow the user to resize columns within a DPE

        const int itemCount = count();
        if (itemCount > 0)
        {
            // divide evenly, unless there are 2 columns, in which case follow the 2/5ths rule here:
            // https://www.o3de.org/docs/tools-ui/ux-patterns/component-card/overview/
            int perItemWidth = (itemCount == 2 ? (rect.width() * 3) / 5 : rect.width() / itemCount);

            // special case the first item to handle indent and the 2/5ths rule
            constexpr int indentSize = 15; // child indent of first item, in pixels
            QRect itemGeometry(rect);
            itemGeometry.setRight(itemCount == 2 ? itemGeometry.width() - perItemWidth : perItemWidth);
            itemGeometry.setLeft(itemGeometry.left() + (m_depth * indentSize));
            itemAt(0)->setGeometry(itemGeometry);

            // iterate over the remaining items, laying them left to right
            for (int layoutIndex = 1; layoutIndex < itemCount; ++layoutIndex)
            {
                itemGeometry.setLeft(itemGeometry.right() + 1);
                itemGeometry.setRight(itemGeometry.left() + perItemWidth);
                itemAt(layoutIndex)->setGeometry(itemGeometry);
            }
        }
    }

    Qt::Orientations DPELayout::expandingDirections() const
    {
        return Qt::Vertical | Qt::Horizontal;
    }

    DocumentPropertyEditor* DPELayout::GetDPE() const
    {
        DocumentPropertyEditor* dpe = nullptr;
        const auto parent = parentWidget();
        if (parent)
        {
            dpe = qobject_cast<DocumentPropertyEditor*>(parent->parentWidget());
            AZ_Assert(dpe, "A DPELayout must be the child of a DPERowWidget, which must be the child of a DocumentPropertyEditor!");
        }
        return dpe;
    }

    DPERowWidget::DPERowWidget(int depth, QWidget* parentWidget)
        : QWidget(parentWidget)
        , m_depth(depth)
        , m_columnLayout(new DPELayout(depth, this))
    {
        // allow horizontal stretching, but use the vertical size hint exactly
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    DPERowWidget::~DPERowWidget()
    {
        Clear();
    }

    void DPERowWidget::Clear()
    {
        // propertyHandlers own their widgets, so don't destroy them here. Set them free!
        for (auto propertyWidgetIter = m_widgetToPropertyHandler.begin(), endIter = m_widgetToPropertyHandler.end();
             propertyWidgetIter != endIter; ++propertyWidgetIter)
        {
            m_columnLayout->removeWidget(propertyWidgetIter->first);
        }
        m_widgetToPropertyHandler.clear();

        DestroyLayoutContents(m_columnLayout);

        // delete all child rows, this will also remove them from the layout
        for (auto entry : m_domOrderedChildren)
        {
            DPERowWidget* rowChild = qobject_cast<DPERowWidget*>(entry.data());
            if (rowChild)
            {
                rowChild->deleteLater();
            }
        }
    }

    void DPERowWidget::AddChildFromDomValue(const AZ::Dom::Value& childValue, int domIndex)
    {
        // create a child widget from the given DOM value and add it to the correct layout
        auto childType = childValue.GetNodeName();

        if (childType == AZ::Dpe::GetNodeName<AZ::Dpe::Nodes::Row>())
        {
            // determine where to put this new row in the main DPE layout
            auto newRow = new DPERowWidget(m_depth + 1);
            DPERowWidget* priorWidgetInLayout = nullptr;

            // search for an existing row sibling with a lower dom index
            for (int priorWidgetIndex = domIndex - 1; priorWidgetInLayout == nullptr && priorWidgetIndex >= 0; --priorWidgetIndex)
            {
                priorWidgetInLayout = qobject_cast<DPERowWidget*>(m_domOrderedChildren[priorWidgetIndex]);
            }

            // if we found a prior DPERowWidget, put this one after the last of its children,
            // if not, put this new row immediately after its parent -- this
            if (priorWidgetInLayout)
            {
                priorWidgetInLayout = priorWidgetInLayout->GetLastDescendantInLayout();
            }
            else
            {
                priorWidgetInLayout = this;
            }
            m_domOrderedChildren.insert(m_domOrderedChildren.begin() + domIndex, newRow);
            GetDPE()->AddAfterWidget(priorWidgetInLayout, newRow);

            // if it's a row, recursively populate the children from the DOM array in the passed value
            newRow->SetValueFromDom(childValue);
        }
        else
        {
            QWidget* addedWidget = nullptr;
            if (childType == AZ::Dpe::GetNodeName<AZ::Dpe::Nodes::Label>())
            {
                auto labelString = AZ::Dpe::Nodes::Label::Value.ExtractFromDomNode(childValue).value_or("");
                addedWidget =
                    new AzQtComponents::ElidingLabel(QString::fromUtf8(labelString.data(), aznumeric_cast<int>(labelString.size())), this);
            }
            else if (childType == AZ::Dpe::GetNodeName<AZ::Dpe::Nodes::PropertyEditor>())
            {
                auto dpeSystem = AZ::Interface<AzToolsFramework::PropertyEditorToolsSystemInterface>::Get();
                auto handlerId = dpeSystem->GetPropertyHandlerForNode(childValue);

                // if we found a valid handler, grab its widget to add to the column layout
                if (handlerId)
                {
                    // store, then reference the unique_ptr that will manage the handler's lifetime
                    auto handler = dpeSystem->CreateHandlerInstance(handlerId);
                    handler->SetValueFromDom(childValue);
                    addedWidget = handler->GetWidget();
                    m_widgetToPropertyHandler[addedWidget] = AZStd::move(handler);
                }
                else
                {
                    addedWidget = new QLabel("Missing handler for dom node!");
                }
            }
            else
            {
                AZ_Assert(0, "unknown node type for DPE");
                return;
            }

            addedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

            // search for an existing column sibling with a lower dom index
            int priorColumnIndex = -1;
            for (int searchIndex = domIndex - 1; (priorColumnIndex == -1 && searchIndex >= 0); --searchIndex)
            {
                priorColumnIndex = m_columnLayout->indexOf(m_domOrderedChildren[searchIndex]);
            }
            // insert after the found index; even if nothing were found and priorIndex is still -1,
            // still insert one after it, at position 0
            m_columnLayout->insertWidget(priorColumnIndex + 1, addedWidget);
            m_domOrderedChildren.insert(m_domOrderedChildren.begin() + domIndex, addedWidget);
        }
    }

    void DPERowWidget::SetValueFromDom(const AZ::Dom::Value& domArray)
    {
        Clear();

        // populate all direct children of this row
        for (size_t arrayIndex = 0, numIndices = domArray.ArraySize(); arrayIndex < numIndices; ++arrayIndex)
        {
            auto& childValue = domArray[arrayIndex];
            AddChildFromDomValue(childValue, aznumeric_cast<int>(arrayIndex));
        }
    }

    void DPERowWidget::HandleOperationAtPath(const AZ::Dom::PatchOperation& domOperation, size_t pathIndex)
    {
        const auto& fullPath = domOperation.GetDestinationPath();
        auto pathEntry = fullPath[pathIndex];
        AZ_Assert(pathEntry.IsIndex() || pathEntry.IsEndOfArray(), "the direct children of a row must be referenced by index");
        auto childCount = m_domOrderedChildren.size();

        // if we're on the last entry in the path, this row widget is the direct owner
        if (pathIndex == fullPath.Size() - 1)
        {
            size_t childIndex = 0;
            if (pathEntry.IsIndex())
            {
                // remove and replace operations must match an existing index. Add operations can be one past the current end.
                AZ_Assert(
                    (domOperation.GetType() == AZ::Dom::PatchOperation::Type::Add ? childIndex <= childCount : childIndex < childCount),
                    "patch index is beyond the array bounds!");

                childIndex = aznumeric_cast<int>(pathEntry.GetIndex());
            }
            else if (domOperation.GetType() == AZ::Dom::PatchOperation::Type::Add)
            {
                childIndex = childCount;
            }
            else // must be IsEndOfArray and a replace or remove, use the last existing index
            {
                childIndex = childCount - 1;
            }

            // if this is a remove or replace, remove the existing entry first,
            // then, if this is a replace or add, add the new entry
            if (domOperation.GetType() == AZ::Dom::PatchOperation::Type::Remove ||
                domOperation.GetType() == AZ::Dom::PatchOperation::Type::Replace)
            {
                const auto childIterator = m_domOrderedChildren.begin() + childIndex;
                delete *childIterator; // deleting the widget also automatically removes it from the layout
                m_domOrderedChildren.erase(childIterator);
            }

            if (domOperation.GetType() == AZ::Dom::PatchOperation::Type::Replace ||
                domOperation.GetType() == AZ::Dom::PatchOperation::Type::Add)
            {
                AddChildFromDomValue(domOperation.GetValue(), aznumeric_cast<int>(childIndex));
            }
        }
        else // not the direct owner of the entry to patch
        {
            // find the next widget in the path and delegate the operation to them
            auto childIndex = (pathEntry.IsIndex() ? pathEntry.GetIndex() : childCount - 1);
            AZ_Assert(childIndex <= childCount, "DPE: Patch failed to apply, invalid child index specified");
            if (childIndex > childCount)
            {
                return;
            }

            QWidget* childWidget = m_domOrderedChildren[childIndex];
            DPERowWidget* widgetAsDpeRow = qobject_cast<DPERowWidget*>(childWidget);
            if (widgetAsDpeRow)
            {
                // child is a DPERowWidget, pass patch processing to it
                widgetAsDpeRow->HandleOperationAtPath(domOperation, pathIndex + 1);
            }
            else // child must be a label or a PropertyEditor
            {
                // pare down the path to this node, then look up and set the value from the DOM
                auto subPath = fullPath;
                for (size_t pathEntryIndex = fullPath.size() - 1; pathEntryIndex > pathIndex; --pathEntryIndex)
                {
                    subPath.Pop();
                }
                const auto valueAtSubPath = GetDPE()->GetAdapter()->GetContents()[subPath];

                // check if it's a PropertyHandler; if it is, just set it from the DOM directly
                auto foundEntry = m_widgetToPropertyHandler.find(childWidget);
                if (foundEntry != m_widgetToPropertyHandler.end())
                {
                    foundEntry->second->SetValueFromDom(valueAtSubPath);
                }
                else
                {
                    QLabel* changedLabel = qobject_cast<QLabel*>(childWidget);
                    AZ_Assert(changedLabel, "not a label, unknown widget discovered!");
                    if (changedLabel)
                    {
                        auto labelString = AZ::Dpe::Nodes::Label::Value.ExtractFromDomNode(valueAtSubPath).value_or("");
                        changedLabel->setText(QString::fromUtf8(labelString.data()));
                    }
                }
            }
        }
    }

    DocumentPropertyEditor* DPERowWidget::GetDPE()
    {
        DocumentPropertyEditor* theDPE = nullptr;
        QWidget* ancestorWidget = parentWidget();
        while (ancestorWidget && !theDPE)
        {
            theDPE = qobject_cast<DocumentPropertyEditor*>(ancestorWidget);
            ancestorWidget = ancestorWidget->parentWidget();
        }
        AZ_Assert(theDPE, "the top level widget in any DPE hierarchy must be the DocumentPropertyEditor itself!");
        return theDPE;
    }

    DPERowWidget* DPERowWidget::GetLastDescendantInLayout()
    {
        DPERowWidget* lastDescendant = nullptr;
        for (auto childIter = m_domOrderedChildren.rbegin(); (lastDescendant == nullptr && childIter != m_domOrderedChildren.rend());
             ++childIter)
        {
            lastDescendant = qobject_cast<DPERowWidget*>(childIter->data());
        }
        if (lastDescendant)
        {
            lastDescendant = lastDescendant->GetLastDescendantInLayout();
        }
        else
        {
            // didn't find any relevant children, this row widget is the last descendant
            lastDescendant = this;
        }
        return lastDescendant;
    }

    DocumentPropertyEditor::DocumentPropertyEditor(QWidget* parentWidget)
        : QFrame(parentWidget)
    {
        m_layout = new QVBoxLayout(this);
    }

    DocumentPropertyEditor::~DocumentPropertyEditor()
    {
        DestroyLayoutContents(GetVerticalLayout());
    }

    void DocumentPropertyEditor::SetAdapter(AZ::DocumentPropertyEditor::DocumentAdapter* theAdapter)
    {
        m_adapter = theAdapter;
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

        // populate the view from the full adapter contents, just like a reset
        HandleReset();
    }

    void DocumentPropertyEditor::AddAfterWidget(QWidget* precursor, QWidget* widgetToAdd)
    {
        int foundIndex = m_layout->indexOf(precursor);
        if (foundIndex >= 0)
        {
            m_layout->insertWidget(foundIndex + 1, widgetToAdd);
        }
    }

    QVBoxLayout* DocumentPropertyEditor::GetVerticalLayout()
    {
        return m_layout;
    }

    void DocumentPropertyEditor::AddRowFromValue(const AZ::Dom::Value& domValue, int rowIndex)
    {
        const bool indexInRange = (rowIndex <= m_domOrderedRows.size());
        AZ_Assert(indexInRange, "rowIndex cannot be more than one past the existing end!")

        if (indexInRange)
        {
            auto newRow = new DPERowWidget(0, this);

            if (rowIndex == 0)
            {
                m_domOrderedRows.push_front(newRow);
                m_layout->insertWidget(0, newRow);
            }
            else
            {
                auto priorRowPosition = m_domOrderedRows.begin() + (rowIndex - 1);
                AddAfterWidget((*priorRowPosition)->GetLastDescendantInLayout(), newRow);
                m_domOrderedRows.insert(priorRowPosition + 1, newRow);
            }
            newRow->SetValueFromDom(domValue);
        }
    }

    void DocumentPropertyEditor::HandleReset()
    {
        // clear any pre-existing DPERowWidgets
        DestroyLayoutContents(m_layout);

        auto topContents = m_adapter->GetContents();

        for (size_t arrayIndex = 0, numIndices = topContents.ArraySize(); arrayIndex < numIndices; ++arrayIndex)
        {
            auto& rowValue = topContents[arrayIndex];
            auto domName = rowValue.GetNodeName().GetStringView();
            const bool isRow = (domName == AZ::DocumentPropertyEditor::Nodes::Row::Name);
            AZ_Assert(isRow, "adapters must only have rows as direct children!");

            if (isRow)
            {
                AddRowFromValue(rowValue, aznumeric_cast<int>(arrayIndex));
            }
        }
        m_layout->addStretch();
    }
    void DocumentPropertyEditor::HandleDomChange(const AZ::Dom::Patch& patch)
    {
        for (auto operationIterator = patch.begin(), endIterator = patch.end(); operationIterator != endIterator; ++operationIterator)
        {
            const auto& patchPath = operationIterator->GetDestinationPath();
            auto firstAddressEntry = patchPath[0];

            AZ_Assert(
                firstAddressEntry.IsIndex() || firstAddressEntry.IsEndOfArray(),
                "first entry in a DPE patch must be the index of the first row");
            auto rowIndex = (firstAddressEntry.IsIndex() ? firstAddressEntry.GetIndex() : m_domOrderedRows.size());
            AZ_Assert(
                rowIndex < m_domOrderedRows.size() ||
                    (rowIndex <= m_domOrderedRows.size() && operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Add),
                "received a patch for a row that doesn't exist");

            // if the patch points at our root, this operation is for the top level layout
            if (patchPath.IsEmpty())
            {
                if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Add)
                {
                    AddRowFromValue(operationIterator->GetValue(), aznumeric_cast<int>(rowIndex));
                }
                else
                {
                    auto rowWidget = m_domOrderedRows[aznumeric_cast<int>(firstAddressEntry.GetIndex())];
                    if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Replace)
                    {
                        rowWidget->SetValueFromDom(operationIterator->GetValue());
                    }
                    else if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Remove)
                    {
                        delete rowWidget;
                    }
                }
            }
            else
            {
                // delegate the action to the rowWidget, which will, in turn, delegate to the next row in the path, if available
                auto rowWidget = m_domOrderedRows[aznumeric_cast<int>(firstAddressEntry.GetIndex())];;
                constexpr size_t pathDepth = 1; // top level has been handled, start the next operation at path depth 1
                rowWidget->HandleOperationAtPath(*operationIterator, pathDepth);
            }
        }
    }

} // namespace AzToolsFramework
