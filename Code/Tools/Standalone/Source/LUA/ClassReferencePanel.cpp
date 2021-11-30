/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ClassReferencePanel.hxx"
#include <Source/LUA/moc_ClassReferencePanel.cpp>

DHClassReferenceWidget::DHClassReferenceWidget(QWidget* parent)
    : QTreeView(parent)
{
    connect(this, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(OnDoubleClicked(const QModelIndex &)));
    //CreateContextMenu();

    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);
}
DHClassReferenceWidget::~DHClassReferenceWidget()
{
}
// QT tree view messages
void DHClassReferenceWidget::OnDoubleClicked(const QModelIndex& modelIdx)
{
    (void)modelIdx;
}
