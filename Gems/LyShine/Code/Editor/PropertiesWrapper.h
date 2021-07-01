/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#endif

class EditorWindow;
class PropertiesWidget;
class HierarchyWidget;
class QMimeData;

class PropertiesWrapper
    : public QWidget
{
    Q_OBJECT

public:

    PropertiesWrapper(HierarchyWidget* hierarchy,
        EditorWindow* parent);

    PropertiesWidget* GetProperties();

    void ActiveCanvasChanged();

private:

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    // Drag/drop assets from asset browser
    bool AcceptsMimeData(const QMimeData* mimeData) const;
    void DropMimeDataAssets(const QMimeData* mimeData);

    PropertiesWidget* m_properties;
    ComponentButton* m_componentButton;
    EditorWindow* m_editorWindow;
    QCheckBox* m_editorOnlyCheckbox = nullptr;
};
