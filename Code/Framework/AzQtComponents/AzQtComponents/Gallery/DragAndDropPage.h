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
    class DragAndDropPage;
}

class DragAndDropPage
    : public QWidget
{
    Q_OBJECT

public:
    explicit DragAndDropPage(QWidget* parent = nullptr);
    ~DragAndDropPage() override = default;

private:
    QScopedPointer<Ui::DragAndDropPage> ui;
};
