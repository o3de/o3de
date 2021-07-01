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
    class ScrollBarPage;
}

class ScrollBarPage
    : public QWidget
{
    Q_OBJECT

public:
    explicit ScrollBarPage(QWidget* parent = nullptr);
    ~ScrollBarPage() override;

private:
    QScopedPointer<Ui::ScrollBarPage> ui;
};
