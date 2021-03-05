/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

