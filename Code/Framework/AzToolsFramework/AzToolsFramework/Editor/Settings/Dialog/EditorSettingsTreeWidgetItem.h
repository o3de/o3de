/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QTreeWidgetItem>
#include <QPixmap>

struct IPreferencesPage;

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
}

class EditorSettingsTreeWidgetItem
    : public QTreeWidgetItem
{
public:

    enum CustomType
    {
        EditorPreferencesPage = QTreeWidgetItem::UserType
    };

    EditorSettingsTreeWidgetItem(const QPixmap& selectedImage, QPixmap& unselectedImage);
    EditorSettingsTreeWidgetItem(const QIcon& icon);
    ~EditorSettingsTreeWidgetItem();

    void Filter(const QString& filter);
    void UpdateEditorFilter(AzToolsFramework::ReflectedPropertyEditor* editor, const QString& filter);

private:
    QPixmap m_selectedImage;
    QPixmap m_unselectedImage;
    QStringList m_propertyNames;
    bool m_entirePageMatchesFilter;
};

