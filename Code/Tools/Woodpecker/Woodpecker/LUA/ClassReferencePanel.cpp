/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "Woodpecker_precompiled.h"

#include "ClassReferencePanel.hxx"
#include <Woodpecker/LUA/moc_ClassReferencePanel.cpp>

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
