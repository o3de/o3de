/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QWidget>
#include <QScopedPointer>

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
    ~TabWidgetPage() override;

private:
    QScopedPointer<Ui::TabWidgetPage> ui;
};
