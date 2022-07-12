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

#include <GradientSignal/Editor/EditorGradientPreviewRenderer.h>
#endif

class QToolButton;

namespace GradientSignal
{
    class GradientPreviewWidget
        : public QWidget
        , public EditorGradientPreviewRenderer
    {
        Q_OBJECT

    public:
        GradientPreviewWidget(QWidget* parent = nullptr, bool enablePopout = false);
        ~GradientPreviewWidget() override;

    Q_SIGNALS:
        void popoutClicked();

    protected:
        void enterEvent(QEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* paintEvent) override;
        void resizeEvent(QResizeEvent* resizeEvent) override;

        void OnUpdate() override;
        QSize GetPreviewSize() const override;

    private:
        QToolButton* m_popoutButton = nullptr;
    };

} //namespace GradientSignal
