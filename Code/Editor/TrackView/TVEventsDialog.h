/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once


#if !defined(Q_MOC_RUN)
#include <IMovieSystem.h>

#include <QDialog>
#endif

namespace Ui
{
    class TVEventsDialog;
}

// CTVEventsDialog dialog

class CTVEventsDialog
    : public QDialog
{
    Q_OBJECT
public:
    CTVEventsDialog(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CTVEventsDialog();

    void OnBnClickedButtonAddEvent();
    void OnBnClickedButtonRemoveEvent();
    void OnBnClickedButtonRenameEvent();
    void OnBnClickedButtonUpEvent();
    void OnBnClickedButtonDownEvent();
    void OnListItemChanged();

    const QString& GetLastAddedEvent();

protected:
    void OnInitDialog();

    void UpdateButtons();

private:
    QScopedPointer<Ui::TVEventsDialog> m_ui;
    QString m_lastAddedEvent;
};
