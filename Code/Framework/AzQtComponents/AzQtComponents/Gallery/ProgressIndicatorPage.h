/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QScopedPointer>
#endif

namespace Ui {
    class ProgressIndicatorPage;
}

class ProgressIndicatorPage : public QWidget
{
    Q_OBJECT

public:
    explicit ProgressIndicatorPage(QWidget* parent = nullptr);
    ~ProgressIndicatorPage() override;

private:
    QScopedPointer<Ui::ProgressIndicatorPage> ui;
};


