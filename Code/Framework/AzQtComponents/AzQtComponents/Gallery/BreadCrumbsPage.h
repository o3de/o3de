/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    class BreadCrumbsPage;
}

class BreadCrumbsPage : public QWidget
{
    Q_OBJECT

public:
    explicit BreadCrumbsPage(QWidget* parent = nullptr);
    ~BreadCrumbsPage() override;

private:
    QScopedPointer<Ui::BreadCrumbsPage> ui;
};


