/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorSettingsTreeWidgetItem.h"

// AzToolsFramework
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>

#include <AzQtComponents/Components/StyleManager.h>

EditorSettingsTreeWidgetItem::EditorSettingsTreeWidgetItem(const QPixmap& selectedImage, QPixmap& unselectedImage)
    : QTreeWidgetItem(EditorPreferencesPage)
    , m_selectedImage(selectedImage)
    , m_unselectedImage(unselectedImage)
    , m_entirePageMatchesFilter(true)
{
}

EditorSettingsTreeWidgetItem::EditorSettingsTreeWidgetItem(const QIcon& icon)
    : QTreeWidgetItem(EditorPreferencesPage)
    , m_entirePageMatchesFilter(true)
{
    setIcon(0, icon);

    setData(0, Qt::DecorationRole, icon);
}

EditorSettingsTreeWidgetItem::~EditorSettingsTreeWidgetItem()
{
}

void EditorSettingsTreeWidgetItem::Filter(const QString& filter)
{
    // Everything on a page matches the filter if its or any of its parents' text matches
    m_entirePageMatchesFilter = false;
    QTreeWidgetItem* item = this;
    while (item && !m_entirePageMatchesFilter)
    {
        m_entirePageMatchesFilter = item->text(0).contains(filter, Qt::CaseInsensitive);
        item = item->parent();
    }

    // Otherwise, just check the contents to see if there's a match
    bool filtered = !m_entirePageMatchesFilter;
    for (auto it = m_propertyNames.begin(), end = m_propertyNames.end(); it != end && filtered; ++it)
    {
        filtered = !(*it).contains(filter, Qt::CaseInsensitive);
    }

    setHidden(filtered);
}

void EditorSettingsTreeWidgetItem::UpdateEditorFilter(AzToolsFramework::ReflectedPropertyEditor* editor, const QString& filter)
{
    QString filterText = m_entirePageMatchesFilter ? QString() : filter;
    editor->InvalidateAll(filterText.toUtf8().constData());
    editor->ExpandAll();
}
