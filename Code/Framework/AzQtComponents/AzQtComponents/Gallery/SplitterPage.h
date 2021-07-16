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
    class SplitterPage;
}

class SplitterPage : public QWidget
{
    Q_OBJECT //AUTOMOC
public:
    explicit SplitterPage(QWidget* parent = nullptr);
    ~SplitterPage() override;

private:
    QScopedPointer<Ui::SplitterPage> ui;
};
