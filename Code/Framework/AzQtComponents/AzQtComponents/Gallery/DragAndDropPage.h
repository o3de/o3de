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

namespace Ui {
    class DragAndDropPage;
}

class DragAndDropPage
    : public QWidget
{
    Q_OBJECT

public:
    explicit DragAndDropPage(QWidget* parent = nullptr);
    ~DragAndDropPage() override;

private:
    QScopedPointer<Ui::DragAndDropPage> ui;
};
