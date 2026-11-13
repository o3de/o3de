/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemRequirementListView.h>
#include <GemCatalog/GemRequirementDelegate.h>
#include <FlatValues.h>

#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/ScrollBar.h>

namespace O3DE::ProjectManager
{
    GemRequirementListView::GemRequirementListView(QAbstractItemModel* model, QItemSelectionModel* selectionModel, QWidget* parent)
        : QListView(parent)
    {
        setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

        // The legacy GUI sets the sheet, which has no selector and so reaches the list, its viewport and its scroll bars: their lanes
        // had this fill in every state, the pointer on a bar included (a widget's sheet outranks the application's `QScrollBar:hover`,
        // ScrollBar.qss:76-81). The flattened GUI sets none: the colour is handed to the list's palette and to the lanes of its bars
        // (ScrollBar::setBackgroundColor), as the colour of the consoles is.
        AzQtComponents::StyleManager::setLegacyStyleSheet(this,
            QString("background-color: %1;").arg(FlatValues::GemRequirementListFill.name()));
        if (AzQtComponents::StyleManager::flatStylingActive())
        {
            AzQtComponents::ScrollBar::setBackgroundColor(this, FlatValues::GemRequirementListFill);
        }

        setModel(model);
        setSelectionModel(selectionModel);
        setItemDelegate(new GemRequirementDelegate(model, this));
    }
} // namespace O3DE::ProjectManager
