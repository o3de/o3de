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
    class MenuPage;
}

class MenuPage
    : public QWidget
{
    Q_OBJECT

public:
    explicit MenuPage(QWidget* parent = nullptr);
    ~MenuPage() override;

private:
    QScopedPointer<Ui::MenuPage> ui;
};


