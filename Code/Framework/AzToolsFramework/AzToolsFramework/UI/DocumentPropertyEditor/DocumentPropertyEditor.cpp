/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "DocumentPropertyEditor.h"

#include <QCheckBox>
#include <QDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>

#include <AzCore/Console/IConsole.h>
#include <AzCore/DOM/DomUtils.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorSystem.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzToolsFramework/UI/DPEDebugViewer/DPEDebugModel.h>
#include <AzToolsFramework/UI/DPEDebugViewer/DPEDebugWindow.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/KeyQueryDPE.h>

AZ_CVAR(
    bool,
    ed_enableCVarDPE,
    true,
    nullptr,
    AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
    "If set, enables experimental DPE-based CVar Editor");

static constexpr const char* GetHandlerPropertyName()
{
    return "handlerId";
}

template<class T>
void DetachAndHide(T* widget)
{
    if (widget)
    {
        QSignalBlocker widgetBlocker(widget);
        widget->hide();
        widget->setParent(nullptr);
    }
}

namespace AzToolsFramework
{
    DPELayout::DPELayout(QWidget* parent)
        : QHBoxLayout(parent)
        , m_depth(-1)
    {
        setContentsMargins(0, 0, 0, 0);
        setSpacing(0);
    }

    void DPELayout::Init(int depth, bool enforceMinWidth, [[maybe_unused]] QWidget* parentWidget)
    {
        m_depth = depth;
        m_enforceMinWidth = enforceMinWidth;
    }

    void DPELayout::Clear()
    {
        m_showExpander = false;
        m_expanded = true;
        delete m_expanderWidget;
        m_expanderWidget = nullptr;
        m_columnStarts.clear();
        m_cachedLayoutSize = QSize();
        m_cachedMinLayoutSize = QSize();
        for (int index = count(); index > 0; --index)
        {
            auto theItem = takeAt(index - 1);
            auto theWidget = theItem->widget();
            if (theWidget)
            {
                theWidget->deleteLater();
            }
        }
    }

    DPELayout::~DPELayout()
    {
        if (m_expanderWidget)
        {
            delete m_expanderWidget;
            m_expanderWidget = nullptr;
        }
    }

    void DPELayout::SetExpanderShown(bool shouldShow)
    {
        if (m_showExpander != shouldShow)
        {
            m_showExpander = shouldShow;
            if (m_expanderWidget && !shouldShow)
            {
                delete m_expanderWidget;
                m_expanderWidget = nullptr;
            }
            update();
        }
    }

    void DPELayout::SetExpanded(bool expanded)
    {
        if (m_expanded != expanded)
        {
            m_expanded = expanded;
            if (m_expanderWidget)
            {
                Qt::CheckState newCheckState = (expanded ? Qt::Checked : Qt::Unchecked);
                if (m_expanderWidget->checkState() != newCheckState)
                {
                    m_expanderWidget->setCheckState(newCheckState);
                }
            }

            emit expanderChanged(expanded);
        }
    }

    bool DPELayout::IsExpanded() const
    {
        return m_expanded;
    }

    void DPELayout::invalidate()
    {
        QHBoxLayout::invalidate();
        m_cachedLayoutSize = QSize();
        m_cachedMinLayoutSize = QSize();
    }

    QSize DPELayout::sizeHint() const
    {
        if (m_cachedLayoutSize.isValid())
        {
            return m_cachedLayoutSize;
        }

        int cumulativeWidth = 0;
        int preferredHeight = 0;

        // sizeHint for this horizontal layout is the sum of the preferred widths,
        // and the maximum of the preferred heights
        for (int layoutIndex = 0, numItems = count(); layoutIndex < numItems; ++layoutIndex)
        {
            auto widgetSizeHint = itemAt(layoutIndex)->sizeHint();
            cumulativeWidth += widgetSizeHint.width();

            if (widgetSizeHint.height() > preferredHeight)
            {
                preferredHeight = widgetSizeHint.height();
            }
        }

        m_cachedLayoutSize.setWidth(cumulativeWidth);
        m_cachedLayoutSize.setHeight(preferredHeight);
        return m_cachedLayoutSize;
    }

    QSize DPELayout::minimumSize() const
    {
        if (m_cachedMinLayoutSize.isValid())
        {
            return m_cachedMinLayoutSize;
        }

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

        m_cachedMinLayoutSize = QSize(cumulativeWidth, minimumHeight);

        return { (m_enforceMinWidth ? cumulativeWidth : 0), minimumHeight };
    }

    void DPELayout::CloseColumn(
        QHBoxLayout* currentColumnLayout,
        QRect& itemGeometry,
        int& currentColumnCount,
        const int columnWidth,
        bool allWidgetsUnstretched,
        bool startSpacer,
        bool endSpacer
    )
    {
        // if all widgets in this shared column take up only their minimum width, set the appropriate alignment with spacers
        if (allWidgetsUnstretched)
        {
            if (startSpacer)
            {
                currentColumnLayout->insertSpacerItem(0, new QSpacerItem(columnWidth, 1, QSizePolicy::Expanding, QSizePolicy::Fixed));
            }
            if (endSpacer)
            {
                currentColumnLayout->addSpacerItem(new QSpacerItem(columnWidth, 1, QSizePolicy::Expanding, QSizePolicy::Fixed));
            }
        }

        // Correctly set the geometry based on the column.
        if (currentColumnCount > 0)
        {
            itemGeometry.setLeft(itemGeometry.right() + 1);
            itemGeometry.setRight(itemGeometry.left() + columnWidth);
        }
        currentColumnLayout->setGeometry(itemGeometry);

        // Count completed columns.
        ++currentColumnCount;
    }

    void DPELayout::setGeometry(const QRect& rect)
    {
        QLayout::setGeometry(rect);

        // todo: implement QSplitter-like functionality to allow the user to resize columns within a DPE
        
        // Determine the number of columns.
        const int columnCount = aznumeric_cast<int>(m_columnStarts.size());

        // Early out if no columns are detected.
        if (columnCount == 0)
        {
            return;
        }

        // Divide horizontal space evenly, unless there are 2 columns, in which case follow the 2/5ths rule here:
        // https://www.o3de.org/docs/tools-ui/ux-patterns/component-card/overview/
        const int columnWidth = (columnCount == 2 ? (rect.width() * 3) / 5 : rect.width() / columnCount);

        // special case the first item to handle indent and the 2/5ths rule
        constexpr int indentSize = 15; // child indent of first item, in pixels
        QRect itemGeometry(rect);
        itemGeometry.setRight(columnCount == 2 ? itemGeometry.width() - columnWidth : columnWidth);
        itemGeometry.setLeft(itemGeometry.left() + (m_depth * indentSize));

        // Show Expander if needed.
        if (m_showExpander)
        {
            if (!m_expanderWidget)
            {
                CreateExpanderWidget();
            }

            m_expanderWidget->move(itemGeometry.topLeft());

            if (auto* widgetParent = parentWidget(); widgetParent && widgetParent->isVisible())
            {
                m_expanderWidget->show();
            }
        }

        // Leave space for Expander even if it's not there.
        constexpr int expanderSpace = 16;
        itemGeometry.setLeft(itemGeometry.left() + expanderSpace);

        // Helper variables for column handling
        QHBoxLayout* currentColumnLayout = nullptr;
        int currentColumnCount = 0;
        int currentColumnWidgetsCount = 0;
        int currentColumnNonStretchedWidgetsCount = 0;
        bool isFirstColumn = true;

        // Store spacing info from last element of each column.
        bool startSpacer = false;
        bool endSpacer = false;

        // Loop through all items one by one.
        auto* myRow = GetRow();
        const int itemCount = count();
        for (int itemIndex = 0; itemIndex < itemCount; ++itemIndex)
        {
            auto* currentItem = itemAt(itemIndex);
            auto* currentWidget = currentItem->widget();

            // Only handle widgets here.
            if (currentWidget == nullptr)
            {
                if (currentColumnLayout)
                {
                    currentColumnLayout->addItem(currentItem);
                }
                continue;
            }

            // Retrieve the dom index for this widget, which is what is used in m_columnStarts.
            auto domIndex = myRow->GetDomIndexOfChild(currentWidget);
            AZ_Assert(domIndex != -1, "widget in layout was not found in row's dom list!");

            // Retrieve attributes.
            AzToolsFramework::DPERowWidget::AttributeInfo* domAttributes = myRow->GetCachedAttributes(domIndex);

            // Save the alignment of the last widget in the shared column with an alignment attribute
            if (domAttributes)
            {
                switch (domAttributes->m_alignment)
                {
                case AZ::Dpe::Nodes::PropertyEditor::Align::AlignLeft:
                    {
                        endSpacer = true;
                        break;
                    }
                case AZ::Dpe::Nodes::PropertyEditor::Align::AlignCenter:
                    {
                        startSpacer = true;
                        endSpacer = true;
                        break;
                    }
                case AZ::Dpe::Nodes::PropertyEditor::Align::AlignRight:
                    {
                        startSpacer = true;
                        break;
                    }
                default:
                    break;
                }
            }

            // If the current widget is the first widget of a column, create the shared column layout and add widgets to it.
            // First widget always creates a column.
            if (m_columnStarts.contains(domIndex) || isFirstColumn)
            {
                // Close previous column.
                if (!isFirstColumn)
                {
                    CloseColumn(
                        currentColumnLayout,
                        itemGeometry,
                        currentColumnCount,
                        columnWidth,
                        (currentColumnNonStretchedWidgetsCount == currentColumnWidgetsCount),
                        startSpacer,
                        endSpacer
                    );
                }

                // Create new column.
                currentColumnLayout = new QHBoxLayout;
                currentColumnWidgetsCount = 0;
                currentColumnNonStretchedWidgetsCount = 0;
                startSpacer = false;
                endSpacer = false;

                if (isFirstColumn)
                {
                    isFirstColumn = false;
                }
            }

            // Add widget to column
            currentColumnLayout->addItem(currentItem);

            // If a widget should only take up its minimum width, do not stretch it
            if (domAttributes && domAttributes->m_minimumWidth)
            {
                ++currentColumnNonStretchedWidgetsCount;
            }
            else
            {
                currentColumnLayout->setStretch(currentColumnLayout->count() - 1, 1);
            }

            ++currentColumnWidgetsCount;
        }

        // Close the last column
        CloseColumn(
            currentColumnLayout,
            itemGeometry,
            currentColumnCount,
            columnWidth,
            (currentColumnNonStretchedWidgetsCount == currentColumnWidgetsCount),
            startSpacer,
            endSpacer
        );
    }

    Qt::Orientations DPELayout::expandingDirections() const
    {
        return Qt::Vertical | Qt::Horizontal;
    }

    void DPELayout::onCheckstateChanged(int expanderState)
    {
        SetExpanded(expanderState == Qt::Checked);
    }

    AzToolsFramework::DPERowWidget* DPELayout::GetRow() const
    {
        return static_cast<DPERowWidget*>(parentWidget());
    }

    void DPELayout::CreateExpanderWidget()
    {
        m_expanderWidget = new QCheckBox(parentWidget());
        m_expanderWidget->setCheckState(m_expanded ? Qt::Checked : Qt::Unchecked);
        AzQtComponents::CheckBox::applyExpanderStyle(m_expanderWidget);
        connect(m_expanderWidget, &QCheckBox::stateChanged, this, &DPELayout::onCheckstateChanged);
    }

    void DPELayout::SetAsStartOfNewColumn(size_t widgetIndex)
    {
        m_columnStarts.insert(widgetIndex);
    }

    DPERowWidget::DPERowWidget()
        : QFrame(nullptr) // parent will be set when the row is added to its layout
        , m_columnLayout(new DPELayout(this))
    {
        m_columnLayout->Init(-1, m_enforceMinWidth, this);
        // allow horizontal stretching, but use the vertical size hint exactly
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QObject::connect(m_columnLayout, &DPELayout::expanderChanged, this, &DPERowWidget::onExpanderChanged);
    }

    void DPERowWidget::Clear()
    {
        for (auto childWidget : m_domOrderedChildren)
        {
            if (childWidget)
            {
                auto handlerInfo = DocumentPropertyEditor::GetInfoFromWidget(childWidget);
                if (!handlerInfo.IsNull())
                {
                    childWidget->hide();
                    m_columnLayout->removeWidget(childWidget);
                    DocumentPropertyEditor::ReleaseHandler(handlerInfo);
                }
                else if (auto rowWidget = qobject_cast<DPERowWidget*>(childWidget))
                {
                    DocumentPropertyEditor::GetRowPool()->RecycleInstance(rowWidget);
                }
                else // if it's not a row or a PropertyHandler, it must be a label
                {
                    auto* label = qobject_cast<AzQtComponents::ElidingLabel*>(childWidget);
                    AZ_Assert(label, "unknown widget in DPERowWidget!");
                    if (label)
                    {
                        DocumentPropertyEditor::GetLabelPool()->RecycleInstance(label);
                    }
                }
            }
        }
        ClearCachedAttributes();
        m_domOrderedChildren.clear();
        m_columnLayout->Clear();

        m_parentRow = nullptr;
        m_depth = -1;
        m_enforceMinWidth = true;
        m_expandingProgrammatically = false;
        m_forceAutoExpand.reset();
        m_expandByDefault.reset();
    }

    DPERowWidget::~DPERowWidget()
    {
        Clear();
    }

    DPERowWidget* DPERowWidget::GetPriorRowInLayout(size_t domIndex)
    {
        DPERowWidget* priorRowInLayout = nullptr;

        // search for an existing row sibling with a lower dom index
        for (int priorWidgetIndex = static_cast<int>(domIndex) - 1; priorRowInLayout == nullptr && priorWidgetIndex >= 0;
             --priorWidgetIndex)
        {
            priorRowInLayout = qobject_cast<DPERowWidget*>(m_domOrderedChildren[priorWidgetIndex]);
        }

        // if we found a prior DPERowWidget, put this one after the last of its children,
        // if not, put this new row immediately after its parent -- this
        if (priorRowInLayout)
        {
            priorRowInLayout = priorRowInLayout->GetLastDescendantInLayout();
        }
        else
        {
            priorRowInLayout = this;
        }
        return priorRowInLayout;
    }

    int DPERowWidget::GetDomIndexOfChild(const QWidget* childWidget) const
    {
        for (int searchIndex = 0, numEntries = aznumeric_cast<int>(m_domOrderedChildren.size()); searchIndex < numEntries; ++searchIndex)
        {
            if (m_domOrderedChildren[searchIndex] == childWidget)
            {
                return searchIndex;
            }
        }
        return -1;
    }

    QWidget* DPERowWidget::GetChild(size_t domIndex)
    {
        AZ_Assert(domIndex < m_domOrderedChildren.size(), "DOM index out of bounds!");
        return m_domOrderedChildren[domIndex];
    }

    void DPERowWidget::AddChildFromDomValue(const AZ::Dom::Value& childValue, size_t domIndex)
    {
        // create a child widget from the given DOM value and add it to the correct layout
        auto childType = childValue.GetNodeName();

        if (childType == AZ::Dpe::GetNodeName<AZ::Dpe::Nodes::Row>())
        {
            if (IsExpanded())
            {
                // create and add the row child to m_domOrderedChildren
                auto newRow = DocumentPropertyEditor::GetRowPool()->GetInstance();
                AddRowChild(newRow, domIndex);

                // if it's a row, recursively populate the children from the DOM array in the passed value
                newRow->SetValueFromDom(childValue);
            }
            else
            {
                // this row isn't expanded, don't create any row children, just log that there's a null widget at
                // the given DOM index
                AddRowChild(nullptr, domIndex);
            }
        }
        else // not a row, so it's a column widget
        {
            QWidget* addedWidget = nullptr;
            if (childType == AZ::Dpe::GetNodeName<AZ::Dpe::Nodes::Label>())
            {
                auto labelString = AZ::Dpe::Nodes::Label::Value.ExtractFromDomNode(childValue).value_or("");
                auto label = DocumentPropertyEditor::GetLabelPool()->GetInstance();
                label->SetText(QString::fromUtf8(labelString.data(), aznumeric_cast<int>(labelString.size())));
                label->setParent(this);
                addedWidget = label;
            }
            else if (childType == AZ::Dpe::GetNodeName<AZ::Dpe::Nodes::PropertyEditor>())
            {
                auto handlerId = AZ::Interface<PropertyEditorToolsSystemInterface>::Get()->GetPropertyHandlerForNode(childValue);

                addedWidget = GetDPE()->CreateWidgetForHandler(handlerId, childValue);
            }
            else
            {
                AZ_Assert(0, "unknown node type for DPE");
                return;
            }

            AddDomChildWidget(domIndex, addedWidget);
            AddColumnWidget(addedWidget, domIndex, childValue);
        }
    }

    void DPERowWidget::RemoveChildAt(size_t childIndex, QWidget** newOwner)
    {
        const auto childIterator = m_domOrderedChildren.begin() + childIndex;
        auto childWidget = *childIterator;
        if (childWidget)
        {
            if (newOwner)
            {
                // transfer ownership
                *newOwner = childWidget;
            }

            // remove the existing child from this row and do any necessary book-keeping
            DPERowWidget* rowToRemove = qobject_cast<DPERowWidget*>(childWidget);
            if (rowToRemove)
            {
                // we're removing a row, remove any associated saved expander state
                auto dpe = GetDPE();
                if (dpe->ShouldEraseExpanderStateWhenRowRemoved())
                {
                    dpe->RemoveExpanderStateForRow(rowToRemove->BuildDomPath());
                }
                if (!newOwner)
                {
                    DocumentPropertyEditor::GetRowPool()->RecycleInstance(rowToRemove);
                }
            }
            else if (auto handlerInfo = DocumentPropertyEditor::GetInfoFromWidget(childWidget); !handlerInfo.IsNull())
            {
                childWidget->hide();
                m_columnLayout->removeWidget(childWidget);
                RemoveCachedAttributes(childIndex);
                if (!newOwner)
                {
                    DocumentPropertyEditor::ReleaseHandler(handlerInfo);
                }
            }
            else // not a row, not a PropertyHandler, must be a label
            {
                auto label = qobject_cast<AzQtComponents::ElidingLabel*>(childWidget);
                AZ_Assert(label, "not a label, unknown widget discovered!");
                if (label && !newOwner)
                {
                    DocumentPropertyEditor::GetLabelPool()->RecycleInstance(label);
                }
            }
        }
        m_domOrderedChildren.erase(childIterator);

        // check if the last row widget child was removed, and hide the expander if necessary
        const bool expanded = IsExpanded();
        auto isDPERow = [expanded](auto* widget)
        {
            // when not expanded, null children are just unseen child rows
            return ((!widget && !expanded) || qobject_cast<DPERowWidget*>(widget) != nullptr);
        };
        if (AZStd::find_if(m_domOrderedChildren.begin(), m_domOrderedChildren.end(), isDPERow) == m_domOrderedChildren.end())
        {
            m_columnLayout->SetExpanderShown(false);
        }
    }

    void DPERowWidget::SetValueFromDom(const AZ::Dom::Value& domArray)
    {
        auto domPath = BuildDomPath();
        SetAttributesFromDom(domArray);

        if (ValueHasChildRows(domArray))
        {
            // determine whether this node should be expanded, so we know whether to populate row children
            ApplyExpansionState(domPath, nullptr);
        }

        // populate all direct children of this row
        for (size_t arrayIndex = 0, numIndices = domArray.ArraySize(); arrayIndex < numIndices; ++arrayIndex)
        {
            auto& childValue = domArray[arrayIndex];
            AddChildFromDomValue(childValue, arrayIndex);
        }
    }

    void DPERowWidget::SetAttributesFromDom(const AZ::Dom::Value& domArray)
    {
        m_forceAutoExpand = AZ::Dpe::Nodes::Row::ForceAutoExpand.ExtractFromDomNode(domArray);
        m_expandByDefault = AZ::Dpe::Nodes::Row::AutoExpand.ExtractFromDomNode(domArray);
    }

    void DPERowWidget::SetPropertyEditorAttributes(size_t domIndex, const AZ::Dom::Value& domArray, QWidget* childWidget)
    {
        AttributeInfo updatedLayoutAttributes;
        updatedLayoutAttributes.m_alignment = AZ::Dpe::Nodes::PropertyEditor::Alignment.ExtractFromDomNode(domArray).value_or(
            AZ::Dpe::Nodes::PropertyEditor::Align::UseDefaultAlignment);
        updatedLayoutAttributes.m_sharePriorColumn =
            AZ::Dpe::Nodes::PropertyEditor::SharePriorColumn.ExtractFromDomNode(domArray).value_or(false);
        updatedLayoutAttributes.m_minimumWidth =
            AZ::Dpe::Nodes::PropertyEditor::UseMinimumWidth.ExtractFromDomNode(domArray).value_or(false);

        if (!updatedLayoutAttributes.m_sharePriorColumn)
        {
            m_columnLayout->SetAsStartOfNewColumn(domIndex);
        }

        // Remove any cached attribute info that is default, else cache it for the layout to use.
        // This way we only cache what is needed.
        auto layoutAttributeInfoIter = m_childIndexToCachedAttributeInfo.find(domIndex);
        if (layoutAttributeInfoIter != m_childIndexToCachedAttributeInfo.end() && updatedLayoutAttributes.IsDefault())
        {
            m_childIndexToCachedAttributeInfo.erase(layoutAttributeInfoIter);
        }
        else
        {
            m_childIndexToCachedAttributeInfo[domIndex] = updatedLayoutAttributes;
        }

        AZStd::string_view descriptionView = AZ::Dpe::Nodes::PropertyEditor::Description.ExtractFromDomNode(domArray).value_or("");
        QString descriptionString = QString::fromUtf8(descriptionView.data(), aznumeric_cast<int>(descriptionView.size()));
        if (descriptionString != childWidget->toolTip())
        {
            setToolTip(descriptionString);
            childWidget->setToolTip(descriptionString);
        }

        bool isDisabled = AZ::Dpe::Nodes::PropertyEditor::Disabled.ExtractFromDomNode(domArray).value_or(false) ||
            AZ::Dpe::Nodes::PropertyEditor::AncestorDisabled.ExtractFromDomNode(domArray).value_or(false);
        childWidget->setEnabled(!isDisabled);
    }

    DPERowWidget::AttributeInfo* DPERowWidget::GetCachedAttributes(size_t domIndex)
    {
        auto foundEntry = m_childIndexToCachedAttributeInfo.find(domIndex);
        if (foundEntry != m_childIndexToCachedAttributeInfo.end())
        {
            return &foundEntry->second;
        }
        else
        {
            return nullptr;
        }
    }

    void DPERowWidget::RemoveCachedAttributes(size_t domIndex)
    {
        auto foundEntry = m_childIndexToCachedAttributeInfo.find(domIndex);
        if (foundEntry != m_childIndexToCachedAttributeInfo.end())
        {
            m_childIndexToCachedAttributeInfo.erase(foundEntry);
            m_columnLayout->m_columnStarts.erase(domIndex);
        }
    }

    void DPERowWidget::ClearCachedAttributes()
    {
        m_childIndexToCachedAttributeInfo.clear();
        m_columnLayout->m_columnStarts.clear();
    }

    void DPERowWidget::HandleOperationAtPath(const AZ::Dom::PatchOperation& domOperation, size_t pathIndex)
    {
        if (domOperation.GetType() == AZ::Dom::PatchOperation::Type::Move)
        {
            /* For a move operation, note that source and/or destination widgets might not exist due to collapsed nodes.
             * if both source and destination location exist, move the existing widget
             * if source exists but not destination, it's a remove
             * if destination exists but not source, we need to look up the value and instantiate it. */
            auto* theDPE = GetDPE();
            auto sourceParentPath = domOperation.GetSourcePath();
            auto sourceIndex = sourceParentPath.Back().GetIndex();
            sourceParentPath.Pop();
            auto* sourceParentRow = qobject_cast<DPERowWidget*>(theDPE->GetWidgetAtPath(sourceParentPath));
            auto* sourceWidget =
                (sourceParentRow && sourceParentRow->m_domOrderedChildren.size() > sourceIndex
                     ? sourceParentRow->m_domOrderedChildren[sourceIndex]
                     : nullptr);

            auto destinationParentPath = domOperation.GetDestinationPath();
            auto destinationIndex = destinationParentPath.Back().GetIndex();
            destinationParentPath.Pop();
            auto* destinationParentRow = qobject_cast<DPERowWidget*>(theDPE->GetWidgetAtPath(destinationParentPath));

            if (sourceWidget)
            {
                if (destinationParentRow)
                {
                    auto widgetAsRow = qobject_cast<DPERowWidget*>(sourceWidget);
                    if (destinationParentRow->IsExpanded() || !widgetAsRow)
                    {
                        // both endpoints already exist, this is a real widget relocation
                        QWidget* newOwner = nullptr;
                        sourceParentRow->RemoveChildAt(sourceIndex, &newOwner);
                        AZ_Assert(newOwner == sourceWidget, "source widget could not be taken!");
                        if (widgetAsRow)
                        {
                            // this is a row move, change its position in the main DPE layout
                            destinationParentRow->AddRowChild(widgetAsRow, destinationIndex);
                        }
                        else
                        {
                            // this is a column widget move, consume its layout attributes and add
                            // it to the correct place in the (possibly) new layout
                            const auto valueForAttributes = theDPE->GetAdapter()->GetContents()[domOperation.GetDestinationPath()];
                            destinationParentRow->AddColumnWidget(newOwner, destinationIndex, valueForAttributes);
                        }
                    }
                    else if (destinationParentRow)
                    {
                        // new child is a row, but the destination parent isn't expanded. Just create a null placeholder
                        destinationParentRow->AddRowChild(nullptr, destinationIndex);

                        // remove old widget
                        sourceParentRow->RemoveChildAt(sourceIndex);
                    }
                }
                else
                {
                    // destination doesn't exist because it has a collapsed ancestor
                    // just remove the source widget - the destination will be instantiated if/when it is expanded
                    sourceParentRow->RemoveChildAt(sourceIndex);
                }
            }
            else
            {
                if (destinationParentRow)
                {
                    // check if the parent row exists but isn't expanded
                    if (sourceParentRow)
                    {
                        AZ_Assert(!sourceParentRow->IsExpanded(), "row should only have null children if it's not expanded!");
                        sourceParentRow->RemoveChildAt(sourceIndex);
                    }
                    // source is missing, but destination exists. Look up the value and treat it as an add at that location
                    auto parentValue = theDPE->GetDomValueForRow(destinationParentRow);
                    destinationParentRow->AddChildFromDomValue(parentValue[destinationIndex], destinationIndex);
                }
                // NB: no else case here. If neither source nor destination exist, the widgets aren't instantiated and nothing is moved
            }
        }
        else
        {
            const auto& fullPath = domOperation.GetDestinationPath();
            auto pathEntry = fullPath[pathIndex];

            const bool entryIsIndex = pathEntry.IsIndex();
            const bool entryAtEnd = (pathIndex == fullPath.Size() - 1); // this is the last entry in the path

            if (!entryIsIndex && entryAtEnd)
            {
                // patch isn't addressing a child index like a child row or widget, it's an attribute,
                // refresh this row from its corresponding DOM node
                auto subPath = fullPath;
                subPath.Pop();
                const auto valueAtSubPath = GetDPE()->GetAdapter()->GetContents()[subPath];
                SetAttributesFromDom(valueAtSubPath);
            }
            else if (entryAtEnd)
            {
                // if we're on the last entry in the path, this row widget is the direct owner
                const auto childCount = m_domOrderedChildren.size();
                size_t childIndex = 0;
                if (pathEntry.IsIndex())
                {
                    // remove and replace operations must match an existing index. Add operations can be one past the current end.
                    childIndex = pathEntry.GetIndex();
                    const bool indexValid =
                        (domOperation.GetType() == AZ::Dom::PatchOperation::Type::Add ? childIndex <= childCount : childIndex < childCount);
                    AZ_Assert(indexValid, "patch index is beyond the array bounds!");
                    if (!indexValid)
                    {
                        return;
                    }
                }

                // if this is a remove or replace, remove the existing entry first,
                // then, if this is a replace or add, add the new entry
                if (domOperation.GetType() == AZ::Dom::PatchOperation::Type::Remove ||
                    domOperation.GetType() == AZ::Dom::PatchOperation::Type::Replace)
                {
                    RemoveChildAt(childIndex);
                }

                if (domOperation.GetType() == AZ::Dom::PatchOperation::Type::Replace ||
                    domOperation.GetType() == AZ::Dom::PatchOperation::Type::Add)
                {
                    AddChildFromDomValue(domOperation.GetValue(), childIndex);
                }
            }
            else // not the direct owner of the entry to patch
            {
                auto theDPE = GetDPE();
                const auto childCount = m_domOrderedChildren.size();
                // find the next widget in the path and delegate the operation to them
                auto childIndex = (pathEntry.IsIndex() ? pathEntry.GetIndex() : childCount - 1);
                AZ_Assert(childIndex < childCount, "DPE: Patch failed to apply, invalid child index specified");
                if (childIndex >= childCount)
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
                    const auto valueAtSubPath = theDPE->GetAdapter()->GetContents()[subPath];

                    if (!childWidget)
                    {
                        // if there's a null entry in the current place for m_domOrderedChildren,
                        // that's ok if this entry isn't expanded to that depth and need not follow the change any further
                        // if we are expanded, then this patch references an unsupported handler, which might a problem
                        if (IsExpanded())
                        {
                            // widget doesn't exist, but maybe we can make one now with the known contents
                            auto handlerId =
                                AZ::Interface<PropertyEditorToolsSystemInterface>::Get()->GetPropertyHandlerForNode(valueAtSubPath);

                            if (handlerId)
                            {
                                // have a proper handlerID now, see if we can make a widget from this value now
                                auto replacementWidget = theDPE->CreateWidgetForHandler(handlerId, valueAtSubPath);
                                if (replacementWidget)
                                {
                                    m_domOrderedChildren[childIndex] = replacementWidget;
                                    AddColumnWidget(replacementWidget, childIndex, valueAtSubPath);
                                }
                            }
                            else if (AZ::DocumentPropertyEditor::PropertyEditorSystem::DPEDebugEnabled())
                            {
                                /* there are many unimplemented PropertyHandlers, so receiving an update for one is fine.
                                 * However, a warning here is useful when debugging a missing widget that is expected to appear */
                                AZ_Warning("Document Property Editor", false, "got patch for unimplemented PropertyHandler");
                            }
                        }
                        // new handler was created with the current value from the DOM, or not. Either way, we're done
                        return;
                    }

                    // check if it's a PropertyHandler; if it is, just set it from the DOM directly
                    if (auto handlerInfo = DocumentPropertyEditor::GetInfoFromWidget(childWidget); !handlerInfo.IsNull())
                    {
                        auto handlerId =
                            AZ::Interface<PropertyEditorToolsSystemInterface>::Get()->GetPropertyHandlerForNode(valueAtSubPath);

                        // check if this patch has morphed the PropertyHandler into a different type
                        if (handlerId != handlerInfo.handlerId)
                        {
                            childWidget->hide();
                            m_columnLayout->removeWidget(childWidget);
                            DocumentPropertyEditor::ReleaseHandler(handlerInfo);

                            // Replace the existing handler widget with one appropriate for the new type
                            auto replacementWidget = theDPE->CreateWidgetForHandler(handlerId, valueAtSubPath);
                            m_domOrderedChildren[childIndex] = replacementWidget;
                            AddColumnWidget(replacementWidget, childIndex, valueAtSubPath);
                        }
                        else
                        {
                            // handler is the same, set the existing handler with the new value
                            RemoveCachedAttributes(childIndex);
                            SetPropertyEditorAttributes(childIndex, valueAtSubPath, childWidget);
                            handlerInfo.handlerInterface->SetValueFromDom(valueAtSubPath);
                        }
                    }
                    else
                    {
                        auto changedLabel = qobject_cast<AzQtComponents::ElidingLabel*>(childWidget);
                        AZ_Assert(changedLabel, "not a label, unknown widget discovered!");
                        if (changedLabel)
                        {
                            auto labelString = AZ::Dpe::Nodes::Label::Value.ExtractFromDomNode(valueAtSubPath).value_or("");
                            changedLabel->setText(QString::fromUtf8(labelString.data(), aznumeric_cast<int>(labelString.size())));
                        }
                    }
                }
            }
        }
    }

    DocumentPropertyEditor* DPERowWidget::GetDPE() const
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

    void DPERowWidget::AddDomChildWidget(size_t domIndex, QWidget* childWidget)
    {
        const bool validIndex = (domIndex <= m_domOrderedChildren.size());
        AZ_Assert(validIndex, "trying to add an out-of-bounds child!");
        if (validIndex)
        {
            m_domOrderedChildren.insert(m_domOrderedChildren.begin() + domIndex, childWidget);
        }
    }

    void DPERowWidget::AddColumnWidget(QWidget* columnWidget, size_t domIndex, const AZ::Dom::Value& domValue)
    {
        if (!columnWidget)
        {
            return;
        }
        columnWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        // search for an existing column sibling with a lower dom index
        int priorColumnIndex = -1;
        for (int searchIndex = static_cast<int>(domIndex) - 1; (priorColumnIndex == -1 && searchIndex >= 0); --searchIndex)
        {
            priorColumnIndex = m_columnLayout->indexOf(GetChild(searchIndex));
        }

        SetPropertyEditorAttributes(domIndex, domValue, columnWidget);

        // insert after the found index; even if nothing were found and priorIndex is -1,
        // insert one after it, at position 0
        m_columnLayout->insertWidget(priorColumnIndex + 1, columnWidget);
        columnWidget->show();
    }

    void DPERowWidget::AddRowChild(DPERowWidget* rowWidget, size_t domIndex)
    {
        if (rowWidget)
        {
            rowWidget->m_parentRow = this;
            rowWidget->m_depth = m_depth + 1;
            rowWidget->m_enforceMinWidth = m_enforceMinWidth;
            rowWidget->m_columnLayout->Init(rowWidget->m_depth, m_enforceMinWidth, this);
        }

        m_columnLayout->SetExpanderShown(true);
        AddDomChildWidget(domIndex, rowWidget);

        if (rowWidget)
        {
            PlaceRowChild(rowWidget, domIndex);
        }
    }

    void DPERowWidget::PlaceRowChild(DPERowWidget* rowWidget, size_t domIndex)
    {
        rowWidget->setParent(this);
        auto dpe = GetDPE();

        // determine where to put this new row in the main DPE layout
        DPERowWidget* priorRowInLayout = GetPriorRowInLayout(domIndex);
        dpe->AddAfterWidget(priorRowInLayout, rowWidget);

        if (rowWidget->IsExpanded())
        {
            for (int childIndex = 0, numChildren = static_cast<int>(rowWidget->m_domOrderedChildren.size()); childIndex < numChildren;
                 ++childIndex)
            {
                DPERowWidget* childRow = qobject_cast<DPERowWidget*>(rowWidget->m_domOrderedChildren[childIndex]);
                if (childRow)
                {
                    rowWidget->PlaceRowChild(childRow, childIndex);
                }
            }
        }
    }

    DPERowWidget* DPERowWidget::GetLastDescendantInLayout()
    {
        // search for the last row child, which will be the last in the vertical layout for this level
        // if we find one, recurse to check if it has row children, which would be the last in the layout for the next level
        DPERowWidget* lastDescendant = nullptr;
        for (auto childIter = m_domOrderedChildren.rbegin(); (lastDescendant == nullptr && childIter != m_domOrderedChildren.rend());
             ++childIter)
        {
            lastDescendant = qobject_cast<DPERowWidget*>(*childIter);
        }

        if (lastDescendant)
        {
            // recurse to check for any child rows that would be displayed after this row
            lastDescendant = lastDescendant->GetLastDescendantInLayout();
        }
        else
        {
            // didn't find any row children, this row widget is the last descendant
            lastDescendant = this;
        }
        return lastDescendant;
    }

    AZ::Dom::Path DPERowWidget::BuildDomPath() const
    {
        auto pathToRoot = GetDPE()->GetPathToRoot(this);
        AZ::Dom::Path rowPath = AZ::Dom::Path();

        for (auto pathIter = pathToRoot.rbegin(); pathIter != pathToRoot.rend(); ++pathIter)
        {
            auto&& reversePathEntry = *pathIter;
            rowPath.Push(reversePathEntry);
        }

        return rowPath;
    }

    void DPERowWidget::SaveExpanderStatesForChildRows(bool isExpanded)
    {
        AZStd::stack<DPERowWidget*> stack;

        const auto pushAllChildRowsToStack = [&stack](AZStd::deque<QWidget*> children)
        {
            for (auto& child : children)
            {
                DPERowWidget* row = qobject_cast<DPERowWidget*>(child);
                if (row)
                {
                    stack.push(row);
                }
            }
        };

        pushAllChildRowsToStack(m_domOrderedChildren);

        while (!stack.empty())
        {
            DPERowWidget* row = stack.top();
            stack.pop();

            pushAllChildRowsToStack(row->m_domOrderedChildren);

            GetDPE()->SetSavedExpanderStateForRow(row->BuildDomPath(), isExpanded);
        }
    }

    bool DPERowWidget::ValueHasChildRows(const AZ::Dom::Value& rowValue)
    {
        bool childRowFound = false;

        for (size_t arrayIndex = 0, numIndices = rowValue.ArraySize(); arrayIndex < numIndices && !childRowFound; ++arrayIndex)
        {
            auto& childValue = rowValue[arrayIndex];
            if (childValue.GetNodeName() == AZ::Dpe::GetNodeName<AZ::Dpe::Nodes::Row>())
            {
                childRowFound = true;
            }
        }
        return childRowFound;
    }

    void DPERowWidget::SetExpanded(bool expanded, bool recurseToChildRows)
    {
        m_expandingProgrammatically = true;
        m_columnLayout->SetExpanded(expanded);

        if (recurseToChildRows)
        {
            for (auto& currentChild : m_domOrderedChildren)
            {
                DPERowWidget* rowChild = qobject_cast<DPERowWidget*>(currentChild);
                if (rowChild)
                {
                    rowChild->SetExpanded(expanded, recurseToChildRows);
                }
            }
        }
        m_expandingProgrammatically = false;
    }

    bool DPERowWidget::IsExpanded() const
    {
        return m_columnLayout->IsExpanded();
    }

    void DPERowWidget::ApplyExpansionState(const AZ::Dom::Path& rowPath, DocumentPropertyEditor* rowDPE)
    {
        if (m_forceAutoExpand.has_value())
        {
            // forced attribute always wins, set the expansion state
            SetExpanded(m_forceAutoExpand.value());
        }
        else
        {
            // nothing forced, so the user's saved expansion state, if it exists, should be used
            if (!rowDPE)
            {
                rowDPE = GetDPE();
            }
            if (rowDPE->IsRecursiveExpansionOngoing())
            {
                SetExpanded(true);
                rowDPE->SetSavedExpanderStateForRow(rowPath, true);
            }
            else if (rowDPE->HasSavedExpanderStateForRow(rowPath))
            {
                SetExpanded(rowDPE->GetSavedExpanderStateForRow(rowPath));
            }
            else
            {
                // no prior expansion state set, use the AutoExpand attribute, if it's set
                if (m_expandByDefault.has_value())
                {
                    SetExpanded(m_expandByDefault.value());
                }
                else
                {
                    // expander state is not explicitly set or saved anywhere, default to expanded
                    SetExpanded(true);
                }
            }
        }
    }

    void DPERowWidget::onExpanderChanged(int expanderState)
    {
        DocumentPropertyEditor* dpe = GetDPE();
        bool isExpanded = expanderState != Qt::Unchecked;

        const bool expandRecursively = (!m_expandingProgrammatically && QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier));
        if (!isExpanded)
        {
            if (expandRecursively)
            {
                // Store collapsed state for all children before deletion if expanding recursively
                SaveExpanderStatesForChildRows(false);
            }

            // expander is collapsed; search for row children and delete them,
            // which will zero out their QPointer in the deque, and remove them from the layout
            for (auto& currentChild : m_domOrderedChildren)
            {
                DPERowWidget* rowChild = qobject_cast<DPERowWidget*>(currentChild);
                if (rowChild)
                {
                    DocumentPropertyEditor::GetRowPool()->RecycleInstance(rowChild);
                    currentChild = nullptr;
                }
            }
        }
        else
        {
            bool initialRecursiveExpander = false;
            if (expandRecursively)
            {
                // Flag DPE as in the middle of a recursive expand operation if expanding recursively
                dpe->SetRecursiveExpansionOngoing(true);
                initialRecursiveExpander = true;
            }

            auto myValue = dpe->GetDomValueForRow(this);
            AZ_Assert(myValue.ArraySize() == m_domOrderedChildren.size(), "known child count does not match child count!");
            for (int valueIndex = 0; valueIndex < m_domOrderedChildren.size(); ++valueIndex)
            {
                if (m_domOrderedChildren[valueIndex] == nullptr)
                {
                    m_domOrderedChildren.erase(m_domOrderedChildren.begin() + valueIndex);
                    AddChildFromDomValue(myValue[valueIndex], valueIndex);
                }
            }
            if (initialRecursiveExpander)
            {
                dpe->SetRecursiveExpansionOngoing(false);
                SaveExpanderStatesForChildRows(true);
            }
        }

        if (!m_expandingProgrammatically)
        {
            // only save our expander state if our expanse/collapse was user-driven
            dpe->SetSavedExpanderStateForRow(BuildDomPath(), isExpanded);
            dpe->updateGeometry();
            dpe->ExpanderChangedByUser();
        }
    }

    bool DPERowWidget::HasChildRows() const
    {
        for (auto currChild : m_domOrderedChildren)
        {
            if (qobject_cast<DPERowWidget*>(currChild))
            {
                return true;
            }
        }
        return false;
    }

    int DPERowWidget::GetLevel() const
    {
        return m_depth;
    }

    DocumentPropertyEditor::DocumentPropertyEditor(QWidget* parentWidget)
        : QScrollArea(parentWidget)
    {
        QWidget* scrollSurface = new QWidget(this);
        m_layout = new QVBoxLayout(scrollSurface);
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setSpacing(2);

        setWidget(scrollSurface);
        setWidgetResizable(true);

        m_spawnDebugView = AZ::DocumentPropertyEditor::PropertyEditorSystem::DPEDebugEnabled();

        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        // register as a co-owner of the recycled widgets lists if they exist; create if not
        auto poolManager = static_cast<AZ::InstancePoolManager*>(AZ::Interface<AZ::InstancePoolManagerInterface>::Get());
        if (m_rowPool = poolManager->GetPool<DPERowWidget>(); !m_rowPool)
        {
            AZStd::function<void(DPERowWidget&)> resetRow = [](DPERowWidget& row)
            {
                row.Clear();
                DetachAndHide(&row);
            };
            m_rowPool = poolManager->CreatePool<DPERowWidget>(resetRow).GetValue();
        }
        if (m_labelPool = poolManager->GetPool<AzQtComponents::ElidingLabel>(); !m_labelPool)
        {
            AZStd::function<void(AzQtComponents::ElidingLabel&)> resetLabel = [](AzQtComponents::ElidingLabel& label)
            {
                DetachAndHide(&label);
            };
            m_labelPool = poolManager->CreatePool<AzQtComponents::ElidingLabel>(resetLabel).GetValue();
        }
    }

    DocumentPropertyEditor::~DocumentPropertyEditor()
    {
        Clear();
    }

    void DocumentPropertyEditor::SetAdapter(AZ::DocumentPropertyEditor::DocumentAdapterPtr theAdapter)
    {
        if (m_spawnDebugView)
        {
            QPointer<AzToolsFramework::DPEDebugWindow> theWindow = new AzToolsFramework::DPEDebugWindow(nullptr);
            theWindow->SetAdapter(theAdapter);
            theWindow->show();
        }

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

        m_domMessageHandler = AZ::DocumentPropertyEditor::DocumentAdapter::MessageEvent::Handler(
            [this](const AZ::DocumentPropertyEditor::AdapterMessage& message, AZ::Dom::Value& value)
            {
                this->HandleDomMessage(message, value);
            });
        m_adapter->ConnectMessageHandler(m_domMessageHandler);

        // Free the settings ptr which saves any in-memory settings to disk and replace it
        // with a default in-memory only settings object until a saved state key is specified
        m_dpeSettings.reset();
        m_dpeSettings = AZStd::unique_ptr<AZ::DocumentPropertyEditor::ExpanderSettings>(m_adapter->CreateExpanderSettings(m_adapter.get()));

        // populate the view from the full adapter contents, just like a reset
        HandleReset();
    }

    void DocumentPropertyEditor::Clear()
    {
        m_rowPool->RecycleInstance(m_rootNode);
        m_rootNode = nullptr;
    }

    void DocumentPropertyEditor::SetAllowVerticalScroll(bool allowVerticalScroll)
    {
        m_allowVerticalScroll = allowVerticalScroll;
        setVerticalScrollBarPolicy(allowVerticalScroll ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);

        auto existingPolicy = sizePolicy();
        setSizePolicy(existingPolicy.horizontalPolicy(), (allowVerticalScroll ? existingPolicy.verticalPolicy() : QSizePolicy::Fixed));
    }

    void DocumentPropertyEditor::SetEnforceMinWidth(bool enforceMinWidth)
    {
        if (m_enforceMinWidth != enforceMinWidth)
        {
            m_enforceMinWidth = enforceMinWidth;
            if (m_rootNode)
            {
                HandleReset();
            }
        }
    }

    QSize DocumentPropertyEditor::sizeHint() const
    {
        auto hint = QScrollArea::sizeHint();
        if (!m_allowVerticalScroll)
        {
            auto margins = QWidget::contentsMargins();
            hint.setHeight(m_layout->sizeHint().height() + margins.top() + margins.bottom());
        }
        return hint;
    }

    void DocumentPropertyEditor::AddAfterWidget(QWidget* precursor, QWidget* widgetToAdd)
    {
        if (precursor == m_rootNode)
        {
            m_layout->insertWidget(0, widgetToAdd);
        }
        else
        {
            int foundIndex = m_layout->indexOf(precursor);
            const bool validInsert = (foundIndex >= 0);
            AZ_Assert(validInsert, "AddAfterWidget: no existing widget found!");

            if (validInsert)
            {
                m_layout->insertWidget(foundIndex + 1, widgetToAdd);
            }
        }

        widgetToAdd->show();
    }

    void DocumentPropertyEditor::SetSavedStateKey(AZ::u32 key, AZStd::string propertyEditorName)
    {
        // We need to append some alphabetical characters to the key or it will be treated as a very large json array index
        AZStd::string keyStr = AZStd::string::format("uuid%s", AZStd::to_string(key).c_str());
        // Free the settings ptr before creating a new one. If the registry key is the same, we want
        // the in-memory settings to be saved to disk (in settings destructor) before they're loaded
        // from disk (in settings constructor)
        m_dpeSettings.reset();
        m_dpeSettings = AZStd::unique_ptr<AZ::DocumentPropertyEditor::ExpanderSettings>(
            m_adapter->CreateExpanderSettings(m_adapter.get(), keyStr, propertyEditorName));

        if (m_dpeSettings && m_dpeSettings->WereSettingsLoaded())
        {
            // Apply the newly loaded expansion states to the tree
            ApplyExpansionStates();
        }
    }

    void DocumentPropertyEditor::ClearInstances()
    {
        m_dpeSettings.reset();
        Clear();
    }

    void DocumentPropertyEditor::SetSavedExpanderStateForRow(const AZ::Dom::Path& rowPath, bool isExpanded)
    {
        if (m_dpeSettings)
        {
            m_dpeSettings->SetExpanderStateForRow(rowPath, isExpanded);
        }
    }

    bool DocumentPropertyEditor::GetSavedExpanderStateForRow(const AZ::Dom::Path& rowPath) const
    {
        if (m_dpeSettings)
        {
            return m_dpeSettings->GetExpanderStateForRow(rowPath);
        }
        return false;
    }

    bool DocumentPropertyEditor::HasSavedExpanderStateForRow(const AZ::Dom::Path& rowPath) const
    {
        if (m_dpeSettings)
        {
            return m_dpeSettings->HasSavedExpanderStateForRow(rowPath);
        }
        return false;
    }

    bool DocumentPropertyEditor::ShouldEraseExpanderStateWhenRowRemoved() const
    {
        return (m_dpeSettings && m_dpeSettings->ShouldEraseStateWhenRowRemoved());
    }

    void DocumentPropertyEditor::RemoveExpanderStateForRow(const AZ::Dom::Path& rowPath)
    {
        if (m_dpeSettings)
        {
            m_dpeSettings->RemoveExpanderStateForRow(rowPath);
        }
    }

    void DocumentPropertyEditor::ApplyExpansionStates()
    {
        auto applyExpansionRecursively =
            [](DPERowWidget* currRow, AZ::Dom::Path rowPath, DocumentPropertyEditor* theDPE, auto&& applyExpansionRecursively) -> void
        {
            // apply the saved expansion state to the current row and then each of its row children
            for (size_t childIndex = 0, numChildren = currRow->m_domOrderedChildren.size(); childIndex < numChildren; ++childIndex)
            {
                auto rowChild = qobject_cast<DPERowWidget*>(currRow->m_domOrderedChildren[childIndex]);
                auto childPath = rowPath / childIndex;
                if (rowChild)
                {
                    rowChild->ApplyExpansionState(childPath, theDPE);
                    applyExpansionRecursively(rowChild, childPath, theDPE, applyExpansionRecursively);
                }
            }
        };

        applyExpansionRecursively(m_rootNode, AZ::Dom::Path(), this, applyExpansionRecursively);
    }

    void DocumentPropertyEditor::ExpandAll()
    {
        for (auto child : m_rootNode->m_domOrderedChildren)
        {
            // all direct children of the root are rows
            auto row = static_cast<DPERowWidget*>(child);
            row->SetExpanded(true, true);
        }
    }

    void DocumentPropertyEditor::CollapseAll()
    {
        for (auto child : m_rootNode->m_domOrderedChildren)
        {
            // all direct children of the root are rows
            auto row = static_cast<DPERowWidget*>(child);
            row->SetExpanded(false, true);
        }
    }

    AZ::Dom::Value DocumentPropertyEditor::GetDomValueForRow(DPERowWidget* row) const
    {
        // Get the index of each dom child going up the chain. We can then reverse this
        // and use these indices to walk the adapter tree and get the Value for the node at this path
        AZStd::vector<size_t> reversePath = GetPathToRoot(row);

        // full index path is built, now get the value from the adapter
        auto returnValue = m_adapter->GetContents();
        for (auto reverseIter = reversePath.rbegin(); reverseIter != reversePath.rend(); ++reverseIter)
        {
            returnValue = returnValue[*reverseIter];
        }
        return returnValue;
    }

    void DocumentPropertyEditor::SetSpawnDebugView(bool shouldSpawn)
    {
        m_spawnDebugView = shouldSpawn;
    }

    bool DocumentPropertyEditor::ShouldReplaceCVarEditor()
    {
        bool dpeCVarEditorEnabled = false;
        if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            console->GetCvarValue(GetEnableCVarEditorName(), dpeCVarEditorEnabled);
        }
        return dpeCVarEditorEnabled;
    }

    QVBoxLayout* DocumentPropertyEditor::GetVerticalLayout()
    {
        return m_layout;
    }

    QWidget* DocumentPropertyEditor::GetWidgetAtPath(const AZ::Dom::Path& path)
    {
        DPERowWidget* currParent = m_rootNode;
        QWidget* currWidget = currParent;
        for (auto entry : path)
        {
            const auto entryIndex = entry.GetIndex();
            if (!currParent || currParent->m_domOrderedChildren.size() <= entryIndex)
            {
                return nullptr;
            }
            currWidget = currParent->m_domOrderedChildren[entryIndex];
            currParent = qobject_cast<DPERowWidget*>(currWidget);
        }
        return currWidget;
    }

    AZStd::vector<size_t> DocumentPropertyEditor::GetPathToRoot(const DPERowWidget* row) const
    {
        AZStd::vector<size_t> pathToRoot;
        const DPERowWidget* thisRow = row;
        const DPERowWidget* parentRow = thisRow->m_parentRow;

        // little lambda for reuse in two different container settings
        auto pushPathPiece = [&pathToRoot](auto container, const auto& element)
        {
            auto pathPiece = AZStd::find(container.begin(), container.end(), element);
            AZ_Assert(pathPiece != container.end(), "these path indices should always be found!");
            pathToRoot.push_back(pathPiece - container.begin());
        };

        // search upwards and get the index of each dom child going up the chain
        while (parentRow)
        {
            pushPathPiece(parentRow->m_domOrderedChildren, thisRow);
            thisRow = parentRow;
            parentRow = parentRow->m_parentRow;
        }
        return pathToRoot;
    }

    bool DocumentPropertyEditor::IsRecursiveExpansionOngoing() const
    {
        return m_isRecursiveExpansionOngoing;
    }

    void DocumentPropertyEditor::SetRecursiveExpansionOngoing(bool isExpanding)
    {
        m_isRecursiveExpansionOngoing = isExpanding;
    }

    void DocumentPropertyEditor::HandleReset()
    {
        // clear any pre-existing DPERowWidgets
        Clear();

        // invisible root node has a "depth" of -1; its children are all at indent 0
        m_rootNode = m_rowPool->GetInstance();
        m_rootNode->m_enforceMinWidth = m_enforceMinWidth;
        m_rootNode->setParent(this);
        m_rootNode->hide();

        auto topContents = m_adapter->GetContents();

        for (size_t arrayIndex = 0, numIndices = topContents.ArraySize(); arrayIndex < numIndices; ++arrayIndex)
        {
            auto& rowValue = topContents[arrayIndex];
            auto domName = rowValue.GetNodeName().GetStringView();
            const bool isRow = (domName == AZ::DocumentPropertyEditor::Nodes::Row::Name);
            AZ_Assert(isRow, "adapters must only have rows as direct children!");

            if (isRow)
            {
                m_rootNode->AddChildFromDomValue(topContents[arrayIndex], arrayIndex);
            }
        }
        m_layout->addStretch();
        updateGeometry();
        emit RequestSizeUpdate();
    }

    void DocumentPropertyEditor::HandleDomChange(const AZ::Dom::Patch& patch)
    {
        if (m_isBeingCleared)
        {
            AZ_Assert(false, "DocumentPropertyEditor::HandleDomChange called while being cleared.  check the callstack.  Suppress your signals during cleanup and destruction of widgets!");
            AZ_TracePrintf("Document Property Editor", "DocumentPropertyEditor::HandleDomChange leaving early");
            return;
        }

        if (m_rootNode)
        {
            bool needsReset = false;
            for (auto operationIterator = patch.begin(), endIterator = patch.end(); !needsReset && operationIterator != endIterator;
                 ++operationIterator)
            {
                if (operationIterator->GetDestinationPath().IsEmpty())
                {
                    needsReset = true;
                }
                else
                {
                    m_rootNode->HandleOperationAtPath(*operationIterator, 0);
                }
            }

            if (needsReset)
            {
                HandleReset();
            }
            else
            {
                updateGeometry();
                emit RequestSizeUpdate();
            }
        }
    }

    void DocumentPropertyEditor::HandleDomMessage(
        const AZ::DocumentPropertyEditor::AdapterMessage& message, [[maybe_unused]] AZ::Dom::Value& value)
    {
        // message match for QueryKey
        auto showKeyQueryDialog = [&](AZ::DocumentPropertyEditor::DocumentAdapterPtr* adapter, AZ::Dom::Path containerPath)
        {
            KeyQueryDPE keyQueryUi(*adapter);
            if (keyQueryUi.exec() == QDialog::Accepted)
            {
                AZ::DocumentPropertyEditor::Nodes::Adapter::AddContainerKey.InvokeOnDomNode(
                    m_adapter->GetContents(), adapter, containerPath);
            }
            else
            {
                AZ::DocumentPropertyEditor::Nodes::Adapter::RejectContainerKey.InvokeOnDomNode(m_adapter->GetContents(), containerPath);
            }
        };

        auto showQuerySubclassDialog =
            [&](AZStd::shared_ptr<AZStd::vector<const AZ::SerializeContext::ClassData*>>* sharedListPointer, AZ::Dom::Path containerPath)
        {
            auto sharedList(*sharedListPointer);
            if (sharedList->empty())
            {
                QMessageBox::warning(
                    this,
                    QString::fromUtf8("Add derived class"),
                    QString::fromUtf8("No suitable derived classes found!"),
                    QMessageBox::Ok,
                    QMessageBox::Ok);
            }
            else
            {
                const AZ::SerializeContext::ClassData* selectedClass = nullptr;
                QStringList derivedClassNames;
                for (auto& derivedClass : *sharedList)
                {
                    const char* derivedClassName = derivedClass->m_editData ? derivedClass->m_editData->m_name : derivedClass->m_name;
                    derivedClassNames.push_back(derivedClassName);
                }

                QString item;
                QInputDialog dialog(this);
                dialog.setWindowTitle(QObject::tr("Class to create"));
                dialog.setLabelText(QObject::tr("Classes"));
                dialog.setComboBoxItems(derivedClassNames);
                dialog.setTextValue(derivedClassNames.value(0));
                dialog.setComboBoxEditable(false);
                bool ok = dialog.exec();
                if (ok)
                {
                    auto selectedClassName = dialog.textValue().toUtf8();
                    for (size_t index = 0; index < sharedList->size() && !selectedClass; ++index)
                    {
                        auto& currentClass = (*sharedList)[index];
                        if (selectedClassName == (currentClass->m_editData ? currentClass->m_editData->m_name : currentClass->m_name))
                        {
                            selectedClass = currentClass;
                        }
                    }
                }
                if (selectedClass)
                {
                    AZ::DocumentPropertyEditor::Nodes::Adapter::AddContainerSubclass.InvokeOnDomNode(
                        m_adapter->GetContents(), selectedClass, containerPath);
                }
            }
        };

        message.Match(
            AZ::DocumentPropertyEditor::Nodes::Adapter::QueryKey,
            showKeyQueryDialog,
            AZ::DocumentPropertyEditor::Nodes::Adapter::QuerySubclass,
            showQuerySubclassDialog);
    }

    void DocumentPropertyEditor::RegisterHandlerPool(AZ::Name handlerName, AZStd::shared_ptr<AZ::InstancePoolBase> handlerPool)
    {
        AZ_Assert(
            m_handlerPools.find(handlerName) == m_handlerPools.end() || m_handlerPools[handlerName] == handlerPool,
            "Attempted to register a new handler pool to a handler name that is already in use.");

        // insertion to the handler pool hash map only succeeds if the handler name is a new key
        // so it won't overwrite any existing registered handler pool for that handler name
        m_handlerPools.insert({ handlerName, handlerPool });
    }

    DocumentPropertyEditor::HandlerInfo DocumentPropertyEditor::GetInfoFromWidget(const QWidget* widget)
    {
        auto infoVariant = widget->property(GetHandlerPropertyName());
        if (!infoVariant.isNull())
        {
            return infoVariant.value<HandlerInfo>();
        }
        return HandlerInfo{};
    }

    AZ::Name DocumentPropertyEditor::GetNameForHandlerId(PropertyEditorToolsSystemInterface::PropertyHandlerId handlerId)
    {
        auto name = AZStd::to_string(reinterpret_cast<uintptr_t>(handlerId));
        auto moduleId = AZ::Environment::GetModuleId();

        auto nameWithModuleId = AZStd::fixed_string<256>::format("%s%p", name.c_str(), moduleId);
        return AZ::Name(nameWithModuleId);
    }

    QWidget* DocumentPropertyEditor::CreateWidgetForHandler(
        PropertyEditorToolsSystemInterface::PropertyHandlerId handlerId, const AZ::Dom::Value& domValue)
    {
        QWidget* createdWidget = nullptr;
        // if we found a valid handler, grab its widget to add to the column layout
        if (handlerId)
        {
            // first try to get the instance pool from pool manager
            auto poolManager = static_cast<AZ::InstancePoolManager*>(AZ::Interface<AZ::InstancePoolManagerInterface>::Get());
            auto handlerName = GetNameForHandlerId(handlerId);
            auto handlerPool = poolManager->GetPool<PropertyHandlerWidgetInterface>(handlerName);

            // create the pool if it does not exist
            if (!handlerPool)
            {
                AZStd::function<void(PropertyHandlerWidgetInterface&)> resetHandler = [](PropertyHandlerWidgetInterface& handler)
                {
                    DetachAndHide(handler.GetWidget());
                };

                AZStd::function<PropertyHandlerWidgetInterface*()> createHandler = [handlerId]()
                {
                    auto createdHandler = AZ::Interface<PropertyEditorToolsSystemInterface>::Get()->CreateHandlerInstance(handlerId);
                    auto bareHandler = createdHandler.get();
                    createdHandler.release();
                    HandlerInfo handlerInfo = { handlerId, bareHandler };
                    bareHandler->GetWidget()->setProperty(GetHandlerPropertyName(), QVariant::fromValue(handlerInfo));
                    return bareHandler;
                };

                handlerPool = poolManager->CreatePool<PropertyHandlerWidgetInterface>(handlerName, resetHandler, createHandler).GetValue();
            }

            // register the handler pool in DPE view to co-own the handler pool
            // the registration is needed in case the handler pool is released by other DPE views
            RegisterHandlerPool(handlerName, handlerPool);

            auto handler = handlerPool->GetInstance();
            handler->SetValueFromDom(domValue);
            createdWidget = handler->GetWidget();
            createdWidget->setEnabled(true);
        }
        return createdWidget;
    }

    void DocumentPropertyEditor::ReleaseHandler(HandlerInfo& handler)
    {
        if (handler.handlerInterface->ResetToDefaults())
        {
            auto poolManager = static_cast<AZ::InstancePoolManager*>(AZ::Interface<AZ::InstancePoolManagerInterface>::Get());
            auto handlerName = GetNameForHandlerId(handler.handlerId);
            auto handlerPool = poolManager->GetPool<PropertyHandlerWidgetInterface>(handlerName);

            if (handlerPool)
            {
                handlerPool->RecycleInstance(handler.handlerInterface);
                return;
            }
        }

        // if the handler was not successfully recycled, then delete the handler immediately; parent widgets won't delete it twice
        delete handler.handlerInterface;
        handler.handlerInterface = nullptr;
    }
} // namespace AzToolsFramework
