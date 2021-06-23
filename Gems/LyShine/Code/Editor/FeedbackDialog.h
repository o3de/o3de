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

class FeedbackDialog
    : public QDialog
{
    Q_OBJECT

public:
    FeedbackDialog(QWidget* pParent = nullptr);

private:
};
