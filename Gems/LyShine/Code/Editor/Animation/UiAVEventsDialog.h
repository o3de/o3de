/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once


#if !defined(Q_MOC_RUN)
#include <LyShine/Animation/IUiAnimation.h>

#include <QDialog>
#endif

namespace Ui
{
    class UiAVEventsDialog;
}

// CUiAVEventsDialog dialog
class CUiAVEventsDialog
    : public QDialog
{
    Q_OBJECT
public:
    CUiAVEventsDialog(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CUiAVEventsDialog();

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
    QScopedPointer<Ui::UiAVEventsDialog> m_ui;
    QString m_lastAddedEvent;
};
