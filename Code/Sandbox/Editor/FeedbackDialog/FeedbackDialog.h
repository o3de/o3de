/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

namespace Ui {
    class FeedbackDialog;
}

class QStringListModel;

class FeedbackDialog
    : public QDialog
{
    Q_OBJECT

public:
    FeedbackDialog(QWidget* pParent = nullptr);
    ~FeedbackDialog();


private:
    QScopedPointer<Ui::FeedbackDialog> ui;
};
