/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_TRACKVIEWEXPORTKEYTIMEDLG_H
#define CRYINCLUDE_EDITOR_TRACKVIEWEXPORTKEYTIMEDLG_H
#pragma once

#include <QDialog>

namespace Ui
{
    class TrackViewExportKeyTimeDlg;
}

class CTrackViewExportKeyTimeDlg
    : public QDialog
{
    Q_OBJECT
public:
    CTrackViewExportKeyTimeDlg(QWidget* parent = nullptr);
    virtual ~CTrackViewExportKeyTimeDlg();

    bool IsAnimationExportChecked() const;
    bool IsSoundExportChecked() const;

private:
    QScopedPointer<Ui::TrackViewExportKeyTimeDlg> m_ui;
};
#endif // CRYINCLUDE_EDITOR_TRACKVIEWEXPORTKEYTIMEDLG_H
