/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "DocumentPropertyEditor.h"

#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>
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
#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyEditorToolsSystemInterface.h>

AZ_CVAR(
    bool,
    ed_enableDPE,
    false,
    nullptr,
    AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
    "If set, enables experimental Document Property Editor support, replacing the Reflected Property Editor where possible");

namespace AzToolsFramework
{
    DPELayout::DPELayout(int depth, QWidget* parentWidget)
        : QHBoxLayout(parentWidget)
        , m_depth(depth)
    {
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
        for (int layoutIndex = 0; layoutIndex < count(); ++layoutIndex)
        {
            auto widgetSizeHint = itemAt(layoutIndex)->sizeHint();
            cumulativeWidth += widgetSizeHint.width();
            preferredHeight = AZStd::max(widgetSizeHint.height(), preferredHeight);
        }

        m_cachedLayoutSize = QSize(cumulativeWidth, preferredHeight);

        return { cumulativeWidth, preferredHeight };
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

        return { cumulativeWidth, minimumHeight };
    }

    void DPELayout::setGeometry(const QRect& rect)
    {
        QLayout::setGeometry(rect);

        // todo: implement QSplitter-like functionality to allow the user to resize columns within a DPE

        //! Treat all widgets in a shared column as one item.
        //! Sum all the widgets, but remove all shared widgets other than the first widget of each shared column.
        int itemCount = count() - SharedWidgetCount() + (int)m_sharePriorColumn.size();

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

            if (m_showExpander)
            {
                if (!m_expanderWidget)
                {
                    CreateExpanderWidget();
                }
                m_expanderWidget->move(itemGeometry.topLeft());
                m_expanderWidget->show();
            }

            // space to leave for expander, whether it's there or not
            constexpr int expanderSpace = 16;
            itemGeometry.setLeft(itemGeometry.left() + expanderSpace);

            // used to iterate through the vector containing a shared column's first widget and size
            int sharedVectorIndex = 0;
            // iterate over each item, laying them left to right
            int layoutIndex = 0;
            const int itemCountActual = count();
            while (layoutIndex  < itemCountActual)
            {
                QWidget* currentWidget = itemAt(layoutIndex)->widget();

                //! If the current widget is the first widget of a shared column, create the shared column layout and add widgets to it
                if (sharedVectorIndex < m_sharePriorColumn.size() && currentWidget == m_sharePriorColumn[sharedVectorIndex].first)
                {
                    QHBoxLayout* sharedColumnLayout = new QHBoxLayout;
                    int numItems = m_sharePriorColumn[sharedVectorIndex].second;
                    int sharedWidgetIndex = 0;
                    // values used to remember the alignment of each widget
                    bool startSpacer = false, endSpacer = false;
                    // number of widgets that should be set to their minimum size
                    int minWidthCount = 0;

                    // Iterate over each item in the current shared column, adding them to a single layout
                    while (sharedWidgetIndex < numItems)
                    {
                        currentWidget = itemAt(layoutIndex + sharedWidgetIndex)->widget();

                        // Save the alignment of the last widget in the shared column with an alignment attribute
                        if (m_widgetAlignment.contains(currentWidget))
                        {
                            switch (m_widgetAlignment[currentWidget])
                            {
                            case Qt::AlignLeft:
                                startSpacer = false;
                                endSpacer = true;
                                break;
                            case Qt::AlignCenter:
                                startSpacer = true;
                                endSpacer = true;
                                break;
                            case Qt::AlignRight:
                                startSpacer = true;
                                endSpacer = false;
                                break;
                            }
                        }
                        sharedColumnLayout->addItem(itemAt(layoutIndex + sharedWidgetIndex));

                        // If a widget should only take up its minimum width, do not stretch it
                        if (m_minimumWidthWidgets.contains(currentWidget))
                        {
                            minWidthCount++;
                        }
                        else
                        {
                            sharedColumnLayout->setStretch(sharedColumnLayout->count() - 1, 1);
                        }
                        sharedWidgetIndex++;
                    }

                    // if all widgets in this shared column take up only their minimum width, set the appropriate alignment with spacers
                    if (minWidthCount == numItems)
                    {
                        if (startSpacer)
                        {
                            QSpacerItem* spacer = new QSpacerItem(perItemWidth, 1, QSizePolicy::Expanding, QSizePolicy::Fixed);
                            sharedColumnLayout->insertSpacerItem(0, spacer);
                        }
                        if (endSpacer)
                        {
                            QSpacerItem* spacer = new QSpacerItem(perItemWidth, 1, QSizePolicy::Expanding, QSizePolicy::Fixed);
                            sharedColumnLayout->addSpacerItem(spacer);
                        }
                    }

                    // Special case if this is the first column in a row
                    if (layoutIndex == 0)
                    {
                        sharedColumnLayout->setGeometry(itemGeometry);
                    }
                    else
                    {
                        itemGeometry.setLeft(itemGeometry.right() + 1);
                        itemGeometry.setRight(itemGeometry.left() + perItemWidth);
                        sharedColumnLayout->setGeometry(itemGeometry);
                    }
                    sharedVectorIndex++;
                    // Increase the layout index by the amount of widgets in the shared column we have iterated over
                    layoutIndex = layoutIndex + sharedWidgetIndex;
                }
                // Widget is not in a shared column, lay it individually with its appropriate alignment
                else
                {
                    if (layoutIndex == 0)
                    {
                        itemAt(layoutIndex)->setGeometry(itemGeometry);
                    }
                    else
                    {
                        itemGeometry.setLeft(itemGeometry.right() + 1);
                        itemGeometry.setRight(itemGeometry.left() + perItemWidth);
                        if (m_widgetAlignment.contains(currentWidget))
                        {
                            itemAt(layoutIndex)->setAlignment(m_widgetAlignment[currentWidget]);
                        }
                        itemAt(layoutIndex)->setGeometry(itemGeometry);
                    }
                    layoutIndex++;
                }
            }
        }
    }

    Qt::Orientations DPELayout::expandingDirections() const
    {
        return Qt::Vertical | Qt::Horizontal;
    }

    void DPELayout::onCheckstateChanged(int expanderState)
    {
        SetExpanded(expanderState == Qt::Checked);
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

    void DPELayout::CreateExpanderWidget()
    {
        m_expanderWidget = new QCheckBox(parentWidget());
        m_expanderWidget->setCheckState(m_expanded ? Qt::Checked : Qt::Unchecked);
        AzQtComponents::CheckBox::applyExpanderStyle(m_expanderWidget);
        connect(m_expanderWidget, &QCheckBox::stateChanged, this, &DPELayout::onCheckstateChanged);
    }

    //! If we are currently adding to an existing shared column group, increase the number of elements in the pair by 1,
    //! otherwise create a new pair of the first widget in the shared column, with size of 2 elements.
    void DPELayout::SharePriorColumn(QWidget* headWidget)
    {
        if (ShouldSharePrior())
        {
            int newWidgetCount = m_sharePriorColumn.back().second + 1;
            m_sharePriorColumn[m_sharePriorColumn.size() - 1].second = newWidgetCount;
        }
        else
        {
            auto widgetColumnPair = AZStd::pair<QWidget*, int>(headWidget, 2);
            m_sharePriorColumn.push_back(widgetColumnPair);
        }
    }

    void DPELayout::SetSharePrior(bool sharePrior)
    {
        m_shouldSharePrior = sharePrior;
    }

    bool DPELayout::ShouldSharePrior()
    {
        return m_shouldSharePrior;
    }

    // Returns the total number of widgets in shared columns.
    int DPELayout::SharedWidgetCount()
    {
        int numWidgets = 0;
        for (int index = 0; index < m_sharePriorColumn.size(); index++)
        {
            numWidgets = numWidgets + m_sharePriorColumn[index].second;
        }
        return numWidgets;
    }

    // Add the widget with its appropriate alignment to the widget alignment map
    void DPELayout::WidgetAlignment(QWidget* alignedWidget, Qt::Alignment widgetAlignment)
    {
        m_widgetAlignment[alignedWidget] = widgetAlignment;
    }

    void DPELayout::AddMinimumWidthWidget(QWidget* widget)
    {
        m_minimumWidthWidgets.insert(widget);
    }

    DPERowWidget::DPERowWidget(int depth, DPERowWidget* parentRow)
        : QFrame(nullptr) // parent will be set when the row is added to its layout
        , m_parentRow(parentRow)
        , m_depth(depth)
        , m_columnLayout(new DPELayout(depth, this))
    {
        // allow horizontal stretching, but use the vertical size hint exactly
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QObject::connect(m_columnLayout, &DPELayout::expanderChanged, this, &DPERowWidget::onExpanderChanged);
    }

    DPERowWidget::~DPERowWidget()
    {
        Clear();
    }

    void DPERowWidget::Clear()
    {
        if (!m_widgetToPropertyHandlerInfo.empty())
        {
            DocumentPropertyEditor* dpe = GetDPE();
            // propertyHandlers own their widgets, so don't destroy them here. Set them free!
            for (auto propertyWidgetIter = m_widgetToPropertyHandlerInfo.begin(), endIter = m_widgetToPropertyHandlerInfo.end();
                 propertyWidgetIter != endIter;
                 ++propertyWidgetIter)
            {
                QWidget* propertyWidget = propertyWidgetIter->first;
                auto toRemove = AZStd::remove(m_domOrderedChildren.begin(), m_domOrderedChildren.end(), propertyWidget);
                m_domOrderedChildren.erase(toRemove, m_domOrderedChildren.end());
                m_columnLayout->removeWidget(propertyWidget);

                propertyWidgetIter->first->setParent(nullptr);
                dpe->ReleaseHandler(AZStd::move(propertyWidgetIter->second.hanlderInterface));
            }
            m_widgetToPropertyHandlerInfo.clear();
        }

        // delete all remaining child widgets, this will also remove them from their layout
        for (auto entry : m_domOrderedChildren)
        {
            if (entry)
            {
                delete entry;
            }
        }
        m_domOrderedChildren.clear();
    }

    void DPERowWidget::AddChildFromDomValue(const AZ::Dom::Value& childValue, size_t domIndex)
    {
        // create a child widget from the given DOM value and add it to the correct layout
        auto childType = childValue.GetNodeName();

        if (childType == AZ::Dpe::GetNodeName<AZ::Dpe::Nodes::Row>())
        {
            m_columnLayout->SetExpanderShown(true);

            if (IsExpanded())
            {
                // determine where to put this new row in the main DPE layout
                auto newRow = new DPERowWidget(m_depth + 1, this);
                DPERowWidget* priorWidgetInLayout = nullptr;

                // search for an existing row sibling with a lower dom index
                for (int priorWidgetIndex = static_cast<int>(domIndex) - 1; priorWidgetInLayout == nullptr && priorWidgetIndex >= 0; --priorWidgetIndex)
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
                AddDomChildWidget(domIndex, newRow);
                GetDPE()->AddAfterWidget(priorWidgetInLayout, newRow);

                // if it's a row, recursively populate the children from the DOM array in the passed value
                newRow->SetValueFromDom(childValue);
            }
            else
            {
                // this row isn't expanded, don't create any row children, just log that there's a null widget at
                // the given DOM index
                AddDomChildWidget(domIndex, nullptr);
            }
        }
        else // not a row, so it's a column widget
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
                auto handlerId = AZ::Interface<PropertyEditorToolsSystemInterface>::Get()->GetPropertyHandlerForNode(childValue);
                addedWidget = CreateWidgetForHandler(handlerId, childValue);
            }
            else
            {
                AZ_Assert(0, "unknown node type for DPE");
                return;
            }

            if (addedWidget)
            {
                AddColumnWidget(addedWidget, domIndex, childValue);
            }
            AddDomChildWidget(domIndex, addedWidget);
        }
    }

    void DPERowWidget::SetValueFromDom(const AZ::Dom::Value& domArray)
    {
        Clear();

        m_domPath = BuildDomPath();

        // determine whether this node should be expanded
        auto forceExpandAttribute = AZ::Dpe::Nodes::Row::ForceAutoExpand.ExtractFromDomNode(domArray);
        if (forceExpandAttribute.has_value())
        {
            // forced attribute always wins, set the expansion state
            SetExpanded(forceExpandAttribute.value());
        }
        else
        {
            // nothing forced, so the user's saved expansion state, if it exists, should be used
            DocumentPropertyEditor* dpe = GetDPE();
            if (dpe->IsRecursiveExpansionOngoing())
            {
                SetExpanded(true);
                dpe->SetSavedExpanderStateForRow(m_domPath, true);
            }
            else if (dpe->HasSavedExpanderStateForRow(m_domPath))
            {
                SetExpanded(dpe->GetSavedExpanderStateForRow(m_domPath));
            }
            else
            {
                // no prior expansion state set, use the AutoExpand attribute, if it's set
                auto autoExpandAttribute = AZ::Dpe::Nodes::Row::AutoExpand.ExtractFromDomNode(domArray);
                if (autoExpandAttribute.has_value())
                {
                    SetExpanded(autoExpandAttribute.value());
                }
                else
                {
                    // expander state is not explicitly set or saved anywhere, default to expanded
                    SetExpanded(true);
                }
            }
        }

        // populate all direct children of this row
        for (size_t arrayIndex = 0, numIndices = domArray.ArraySize(); arrayIndex < numIndices; ++arrayIndex)
        {
            auto& childValue = domArray[arrayIndex];
            AddChildFromDomValue(childValue, arrayIndex);
        }
    }

    void DPERowWidget::HandleOperationAtPath(const AZ::Dom::PatchOperation& domOperation, size_t pathIndex)
    {
        const auto& fullPath = domOperation.GetDestinationPath();
        auto pathEntry = fullPath[pathIndex];

        const bool entryIsIndex = pathEntry.IsIndex() || pathEntry.IsEndOfArray();
        const bool entryAtEnd = (pathIndex == fullPath.Size() - 1); // this is the last entry in the path

        if (!entryIsIndex && entryAtEnd)
        {
            // patch isn't addressing a child index like a child row or widget, it's an attribute,
            // refresh this row from its corresponding DOM node
            auto subPath = fullPath;
            subPath.Pop();
            const auto valueAtSubPath = GetDPE()->GetAdapter()->GetContents()[subPath];
            SetValueFromDom(valueAtSubPath);
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
                DPERowWidget* rowToRemove = qobject_cast<DPERowWidget*>(*childIterator);
                if (rowToRemove)
                {
                    // we're removing a row, remove any associated saved expander state
                    GetDPE()->RemoveExpanderStateForRow(rowToRemove->GetPath());
                }

                delete (*childIterator); // deleting the widget also automatically removes it from the layout
                m_domOrderedChildren.erase(childIterator);

                // check if the last row widget child was removed, and hide the expander if necessary
                auto isDPERow = [](auto* widget)
                {
                    return qobject_cast<DPERowWidget*>(widget) != nullptr;
                };
                if (AZStd::find_if(m_domOrderedChildren.begin(), m_domOrderedChildren.end(), isDPERow) == m_domOrderedChildren.end())
                {
                    m_columnLayout->SetExpanderShown(false);
                }
            }

            if (domOperation.GetType() == AZ::Dom::PatchOperation::Type::Replace ||
                domOperation.GetType() == AZ::Dom::PatchOperation::Type::Add)
            {
                AddChildFromDomValue(domOperation.GetValue(), childIndex);
            }
        }
        else // not the direct owner of the entry to patch
        {
            const auto childCount = m_domOrderedChildren.size();
            // find the next widget in the path and delegate the operation to them
            auto childIndex = (pathEntry.IsIndex() ? pathEntry.GetIndex() : childCount - 1);
            AZ_Assert(childIndex <= childCount, "DPE: Patch failed to apply, invalid child index specified");
            if (childIndex > childCount)
            {
                return;
            }

            QWidget* childWidget = m_domOrderedChildren[childIndex];

            if (!childWidget)
            {
                // if there's a null entry in the current place for m_domOrderedChildren,
                // that's ok if this entry isn't expanded to that depth and need not follow the change any further
                // if we are expanded, then this patch references an unsupported handler, which might a problem
                if (IsExpanded())
                {
                    AZ_Warning("Document Property Editor", false, "got patch for unimplemented PropertyHandler");
                }
                return;
            }

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
                auto foundEntry = m_widgetToPropertyHandlerInfo.find(childWidget);
                if (foundEntry != m_widgetToPropertyHandlerInfo.end())
                {
                    auto handlerId = AZ::Interface<PropertyEditorToolsSystemInterface>::Get()->GetPropertyHandlerForNode(valueAtSubPath);

                    // check if this patch has morphed the PropertyHandler into a different type
                    if (handlerId != foundEntry->second.handlerId)
                    {
                        // CreateWidgetForHandler will add a new entry to m_widgetToPropertyHandlerInfo, kill the old entry
                        GetDPE()->ReleaseHandler(AZStd::move(foundEntry->second.hanlderInterface));
                        m_widgetToPropertyHandlerInfo.erase(foundEntry);

                        // Replace the existing handler widget with one appropriate for the new type
                        auto replacementWidget = CreateWidgetForHandler(handlerId, valueAtSubPath);
                        AddColumnWidget(replacementWidget, childIndex, valueAtSubPath);
                        AddDomChildWidget(childIndex, replacementWidget);
                    }
                    else
                    {
                        // handler is the same, set the existing handler with the new value
                        foundEntry->second.hanlderInterface->SetValueFromDom(valueAtSubPath);
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
        if (m_domOrderedChildren.size() > domIndex)
        {
            delete m_domOrderedChildren[domIndex];
            m_domOrderedChildren[domIndex] = childWidget;
        }
        else if (m_domOrderedChildren.size() == domIndex)
        {
            m_domOrderedChildren.push_back(childWidget);
        }
        else
        {
            AZ_Assert(0, "error: trying to add an out of bounds index");
            return;
        }
    }

    void DPERowWidget::AddColumnWidget(QWidget* columnWidget, size_t domIndex, const AZ::Dom::Value& domValue)
    {
        columnWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        // search for an existing column sibling with a lower dom index
        int priorColumnIndex = -1;
        for (int searchIndex = static_cast<int>(domIndex) - 1; (priorColumnIndex == -1 && searchIndex >= 0); --searchIndex)
        {
            priorColumnIndex = m_columnLayout->indexOf(m_domOrderedChildren[searchIndex]);
        }

        // if the alignment attribute is present, add the widget with its appropriate alignment to the column layout
        auto alignment = AZ::Dpe::Nodes::PropertyEditor::Alignment.ExtractFromDomNode(domValue);
        if (alignment.has_value())
        {
            Qt::Alignment widgetAlignment;
            switch (alignment.value())
            {
            case AZ::Dpe::Nodes::PropertyEditor::Align::AlignLeft:
                widgetAlignment = Qt::AlignLeft;
                break;
            case AZ::Dpe::Nodes::PropertyEditor::Align::AlignCenter:
                widgetAlignment = Qt::AlignCenter;
                break;
            case AZ::Dpe::Nodes::PropertyEditor::Align::AlignRight:
                widgetAlignment = Qt::AlignRight;
                break;
            }
            m_columnLayout->WidgetAlignment(columnWidget, widgetAlignment);
        }

        //! If the sharePrior attribute is present, add the previous widget to the column layout.
        //! Set the SharePrior boolean so we know to create a new shared column layout, or add to an existing one
        auto sharePrior = AZ::Dpe::Nodes::PropertyEditor::SharePriorColumn.ExtractFromDomNode(domValue).value_or(false);
        if (sharePrior)
        {
            m_columnLayout->SharePriorColumn(m_columnLayout->itemAt(priorColumnIndex)->widget());
            m_columnLayout->SetSharePrior(true);
        }
        else
        {
            m_columnLayout->SetSharePrior(false);
        }

        // If the UseMinimumWidth attribute is present, add the widget to set of widgets using their minimum width
        auto minimumWidth = AZ::Dpe::Nodes::PropertyEditor::UseMinimumWidth.ExtractFromDomNode(domValue);
        if (minimumWidth.has_value() && minimumWidth.value())
        {
            m_columnLayout->AddMinimumWidthWidget(columnWidget);
        }

        // insert after the found index; even if nothing were found and priorIndex is -1,
        // insert one after it, at position 0
        m_columnLayout->insertWidget(priorColumnIndex + 1, columnWidget);
    }

    QWidget* DPERowWidget::CreateWidgetForHandler(
        PropertyEditorToolsSystemInterface::PropertyHandlerId handlerId, const AZ::Dom::Value& domValue)
    {
        QWidget* createdWidget = nullptr;
        // if we found a valid handler, grab its widget to add to the column layout
        if (handlerId)
        {
            auto descriptionString = AZ::Dpe::Nodes::PropertyEditor::Description.ExtractFromDomNode(domValue).value_or("");
            auto shouldDisable = AZ::Dpe::Nodes::PropertyEditor::Disabled.ExtractFromDomNode(domValue).value_or(false);

            // if this row doesn't already have a tooltip, use the first valid
            // tooltip from a child PropertyEditor (like the RPE)
            if (!descriptionString.empty() && toolTip().isEmpty())
            {
                setToolTip(QString::fromUtf8(descriptionString.data(), aznumeric_cast<int>(descriptionString.size())));
            }

            // store, then reference the unique_ptr that will manage the handler's lifetime
            auto handler = AZ::Interface<PropertyEditorToolsSystemInterface>::Get()->CreateHandlerInstance(handlerId);
            handler->SetValueFromDom(domValue);
            createdWidget = handler->GetWidget();
            createdWidget->setEnabled(!shouldDisable);

            // only set the widget's tooltip if it doesn't already have its own
            if (!descriptionString.empty() && createdWidget->toolTip().isEmpty())
            {
                createdWidget->setToolTip(QString::fromUtf8(descriptionString.data(), aznumeric_cast<int>(descriptionString.size())));
            }
            m_widgetToPropertyHandlerInfo[createdWidget] = { handlerId, AZStd::move(handler) };
        }
        return createdWidget;
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

    AZ::Dom::Path DPERowWidget::BuildDomPath()
    {
        auto pathToRoot = GetDPE()->GetPathToRoot(this);
        AZ::Dom::Path rowPath = AZ::Dom::Path();

        for (auto reversePathEntry : pathToRoot | AZStd::views::reverse)
        {
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

            GetDPE()->SetSavedExpanderStateForRow(row->GetPath(), isExpanded);
        }
    }

    void DPERowWidget::SetExpanded(bool expanded, bool recurseToChildRows)
    {
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
    }

    bool DPERowWidget::IsExpanded() const
    {
        return m_columnLayout->IsExpanded();
    }

    void DPERowWidget::onExpanderChanged(int expanderState)
    {
        DocumentPropertyEditor* dpe = GetDPE();
        bool isExpanded = expanderState != Qt::Unchecked;

        if (!isExpanded)
        {
            if (QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier))
            {
                // Store collapsed state for all children before deletion if shift was pressed
                SaveExpanderStatesForChildRows(false);
            }

            // expander is collapsed; search for row children and delete them,
            // which will zero out their QPointer in the deque, and remove them from the layout
            for (auto& currentChild : m_domOrderedChildren)
            {
                DPERowWidget* rowChild = qobject_cast<DPERowWidget*>(currentChild);
                if (rowChild)
                {
                    delete rowChild;
                    currentChild = nullptr;
                }
            }
        }
        else
        {
            if (QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier))
            {
                // Flag DPE as in the middle of a recursive expand operation if shift was pressed
                dpe->SetRecursiveExpansionOngoing(true);
            }

            auto myValue = dpe->GetDomValueForRow(this);
            AZ_Assert(myValue.ArraySize() == m_domOrderedChildren.size(), "known child count does not match child count!");
            for (int valueIndex = 0; valueIndex < m_domOrderedChildren.size(); ++valueIndex)
            {
                if (m_domOrderedChildren[valueIndex] == nullptr)
                {
                    AddChildFromDomValue(myValue[valueIndex], valueIndex);
                }
            }

            dpe->SetRecursiveExpansionOngoing(false);
        }

        dpe->SetSavedExpanderStateForRow(m_domPath, isExpanded);
    }

    const AZ::Dom::Path DPERowWidget::GetPath() const
    {
        return m_domPath;
    }

    bool DPERowWidget::HasChildRows() const
    {
        return !m_domOrderedChildren.empty();
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
        setWidget(scrollSurface);
        setWidgetResizable(true);

        m_handlerCleanupTimer = new QTimer(this);
        m_handlerCleanupTimer->setSingleShot(true);
        m_handlerCleanupTimer->setInterval(0);
        connect(m_handlerCleanupTimer, &QTimer::timeout, this, &DocumentPropertyEditor::CleanupReleasedHandlers);

        m_spawnDebugView = AZ::DocumentPropertyEditor::PropertyEditorSystem::DPEDebugEnabled();

        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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

        // Free the settings ptr which in turn saves any in-memory settings to disk
        m_dpeSettings.reset();

        // populate the view from the full adapter contents, just like a reset
        HandleReset();
    }

    void DocumentPropertyEditor::Clear()
    {
        delete m_rootNode;
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
    }

    void DocumentPropertyEditor::SetSavedStateKey(AZ::u32 key, AZStd::string propertyEditorName)
    {
        // We need to append some alphabetical characters to the key or it will be treated as a very large json array index
        AZStd::string_view keyStr = AZStd::string::format("uuid%s", AZStd::to_string(key).c_str());
        m_dpeSettings = AZStd::make_unique<DocumentPropertyEditorSettings>(keyStr, propertyEditorName);

        if (m_dpeSettings && m_dpeSettings->WereSettingsLoaded())
        {
            m_dpeSettings->SetCleanExpanderStateCallback(
                [this](DocumentPropertyEditorSettings::ExpanderStateMap& storedStates)
                {
                    const auto& rootValue = m_adapter->GetContents();
                    auto numErased = AZStd::erase_if(storedStates,
                        [&rootValue](const AZStd::pair<AZStd::string, bool>& statePair)
                        {
                            return !rootValue.FindChild(AZ::Dom::Path(statePair.first)) ? true : false;
                        });
                    return numErased > 0;
                });

            // We need to rebuild the view using the stored expander states
            HandleReset();
        }
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

    void DocumentPropertyEditor::RemoveExpanderStateForRow(const AZ::Dom::Path& rowPath)
    {
        if (m_dpeSettings)
        {
            return m_dpeSettings->RemoveExpanderStateForRow(rowPath);
        }
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

    void DocumentPropertyEditor::ReleaseHandler(AZStd::unique_ptr<PropertyHandlerWidgetInterface>&& handler)
    {
        m_unusedHandlers.emplace_back(AZStd::move(handler));
        m_handlerCleanupTimer->start();
    }

    void DocumentPropertyEditor::SetSpawnDebugView(bool shouldSpawn)
    {
        m_spawnDebugView = shouldSpawn;
    }

    bool DocumentPropertyEditor::ShouldReplaceRPE()
    {
        bool dpeEnabled = false;
        if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            console->GetCvarValue("ed_enableDPE", dpeEnabled);
        }
        return dpeEnabled;
    }

    QVBoxLayout* DocumentPropertyEditor::GetVerticalLayout()
    {
        return m_layout;
    }

    AZStd::vector<size_t> DocumentPropertyEditor::GetPathToRoot(DPERowWidget* row) const
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
        m_rootNode = new DPERowWidget(-1, nullptr);
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
    }

    void DocumentPropertyEditor::HandleDomChange(const AZ::Dom::Patch& patch)
    {
        for (auto operationIterator = patch.begin(), endIterator = patch.end(); operationIterator != endIterator; ++operationIterator)
        {
            m_rootNode->HandleOperationAtPath(*operationIterator, 0);
        }
    }

    void DocumentPropertyEditor::HandleDomMessage(
        const AZ::DocumentPropertyEditor::AdapterMessage& message, [[maybe_unused]] AZ::Dom::Value& value)
    {
        // message match for QueryKey
        auto showKeyQueryDialog = [&](AZ::DocumentPropertyEditor::DocumentAdapterPtr* adapter, AZ::Dom::Path containerPath)
        {
            KeyQueryDPE keyQueryUi(adapter);
            if (keyQueryUi.exec() == QDialog::Accepted)
            {
                AZ::DocumentPropertyEditor::Nodes::Adapter::AddContainerKey.InvokeOnDomNode(
                    m_adapter->GetContents(), adapter, containerPath);
            }
            else
            {
                AZ::DocumentPropertyEditor::Nodes::Adapter::RejectContainerKey.InvokeOnDomNode(
                    m_adapter->GetContents(), adapter, containerPath);
            }
        };

        message.Match(AZ::DocumentPropertyEditor::Nodes::Adapter::QueryKey, showKeyQueryDialog);
    }

    void DocumentPropertyEditor::CleanupReleasedHandlers()
    {
        // Release unused handlers from the pool, thereby destroying them and their associated widgets
        m_unusedHandlers.clear();
    }
} // namespace AzToolsFramework
