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
    class SvgLabelPage;
}

class SvgLabelPage : public QWidget
{
    Q_OBJECT //AUTOMOC

public:
    explicit SvgLabelPage(QWidget* parent = nullptr);
    virtual ~SvgLabelPage() override;

protected slots:
    void onWidthChanged(int newWidth);
    void onHeightChanged(int newHeight);
    void onResetSize();
    void onMaintainAspectToggled(bool checked);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QScopedPointer<Ui::SvgLabelPage> m_ui;
    QSize m_initialSize;
};

