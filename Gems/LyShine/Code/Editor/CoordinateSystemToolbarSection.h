/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QObject>
#endif

class EditorWindow;
class QComboBox;
class QCheckBox;
class QToolBar;

class CoordinateSystemToolbarSection
    : public QObject
{
    Q_OBJECT

public:

    explicit CoordinateSystemToolbarSection(QToolBar* parent, bool addSeparator);

    void SetIsEnabled(bool enabled);

    void SetCurrentIndex(int index);

    void SetSnapToGridIsChecked(bool checked);

private slots:

    //! Triggered by keyboard shortcuts.
    //@{
    void HandleCoordinateSystemCycle();
    void HandleSnapToGridToggle();
    //@}

private:

    int CycleSelectedItem();

    void UpdateCanvasSnapEnabled();

    EditorWindow* m_editorWindow;
    QComboBox* m_combobox;
    QCheckBox* m_snapCheckbox;
};
