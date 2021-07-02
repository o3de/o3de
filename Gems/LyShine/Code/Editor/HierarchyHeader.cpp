/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasEditor_precompiled.h"

#include "EditorCommon.h"

#define UICANVASEDITOR_HIERARCHY_HEADER_ICON_EYE        ":/Icons/Eye.svg"
#define UICANVASEDITOR_HIERARCHY_HEADER_ICON_PADLOCK    ":/Icons/Padlock.svg"

HierarchyHeader::HierarchyHeader(HierarchyWidget* parent)
    : QHeaderView(Qt::Horizontal, parent)
    , m_hierarchy(parent)
    , m_visibleIcon(UICANVASEDITOR_HIERARCHY_HEADER_ICON_EYE)
    , m_selectableIcon(UICANVASEDITOR_HIERARCHY_HEADER_ICON_PADLOCK)
{
    setMouseTracking(true);
    setSectionsMovable(false);
    setStretchLastSection(false);

    QObject::connect(this,
        &QHeaderView::sectionClicked,
        this,
        [parent](int logicalIndex)
        {
            if (logicalIndex == kHierarchyColumnName)
            {
                // Nothing to do.
                return;
            }

            HierarchyItemRawPtrList items = SelectionHelpers::GetSelectedHierarchyItems(parent, parent->selectedItems());
            if (items.empty())
            {
                // If nothing is selected, then act on all existing items.
                HierarchyHelpers::AppendAllChildrenToEndOfList(parent->invisibleRootItem(), items);

                if (items.empty())
                {
                    // Nothing to do.
                    return;
                }
            }

            if (logicalIndex == kHierarchyColumnIsVisible)
            {
                CommandHierarchyItemToggleIsVisible::Push(parent->GetEditorWindow()->GetActiveStack(),
                    parent,
                    items);
            }
            else if (logicalIndex == kHierarchyColumnIsSelectable)
            {
                CommandHierarchyItemToggleIsSelectable::Push(parent->GetEditorWindow()->GetActiveStack(),
                    parent,
                    items);
            }
            else
            {
                // This should NEVER happen.
                AZ_Assert(0, "Unxpected value for logicalIndex");
            }
        });
}

QSize HierarchyHeader::sizeHint() const
{
    // This controls the height of the header.
    return QSize(UICANVASEDITOR_HIERARCHY_HEADER_ICON_SIZE, UICANVASEDITOR_HIERARCHY_HEADER_ICON_SIZE);
}

void HierarchyHeader::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const
{
    if (logicalIndex == kHierarchyColumnIsVisible)
    {
        m_visibleIcon.paint(painter, rect);
    }
    else if (logicalIndex == kHierarchyColumnIsSelectable)
    {
        m_selectableIcon.paint(painter, rect);
    }

    // IMPORTANT: We DON'T want to call QHeaderView::paintSection() here.
    // Otherwise it will draw over our icons.
}

void HierarchyHeader::enterEvent(QEvent* ev)
{
    m_hierarchy->ClearItemBeingHovered();

    QHeaderView::enterEvent(ev);
}

#include <moc_HierarchyHeader.cpp>
