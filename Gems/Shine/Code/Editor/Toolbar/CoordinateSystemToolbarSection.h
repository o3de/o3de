/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QObject>

class EditorWindow;
class QComboBox;
class QCheckBox;
class QToolBar;

class CoordinateSystemToolbarSection : public QObject
{
    Q_OBJECT

public:
    explicit CoordinateSystemToolbarSection(QToolBar* parent, bool addSeparator);

    void SetIsEnabled(bool enabled);
    void SetCurrentIndex(int index);
    void SetSnapToGridIsChecked(bool checked);

private Q_SLOTS:

    //! Triggered by keyboard shortcuts.
    //@{
    void HandleCoordinateSystemCycle();
    void HandleSnapToGridToggle();
    //@}

private:
    int CycleSelectedItem();
    void UpdateCanvasSnapEnabled();

private:
    EditorWindow* m_editorWindow;
    QComboBox* m_combobox;
    QCheckBox* m_snapCheckbox;
};
