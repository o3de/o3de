/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QWidget>

#include <GradientSignal/Editor/EditorGradientPreviewRenderer.h>

namespace GradientSignal
{
    class GradientPreviewWidget
        : public QWidget
        , public EditorGradientPreviewRenderer
    {
    public:
        GradientPreviewWidget(QWidget* parent = nullptr);
        ~GradientPreviewWidget() override;

    protected:
        void paintEvent(QPaintEvent* paintEvent) override;
        void resizeEvent(QResizeEvent* resizeEvent) override;

        void OnUpdate() override;
        QSize GetPreviewSize() const override;
    };

} //namespace GradientSignal
