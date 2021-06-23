/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#endif

namespace Ui
{
    class TabWidgetPage;
}

class TabWidgetPage
    : public QWidget
{
    Q_OBJECT

public:
    explicit TabWidgetPage(QWidget* parent = nullptr);

private:
    QScopedPointer<Ui::TabWidgetPage> ui;
};
