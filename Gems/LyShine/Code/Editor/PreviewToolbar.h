/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QToolBar>

#include "CanvasSizeToolbarSection.h"
#endif

class EditorWindow;
class QResizeEvent;

class PreviewToolbar
    : public QToolBar
{
    Q_OBJECT

public:

    explicit PreviewToolbar(EditorWindow* parent);

    void ViewportHasResized(QResizeEvent* ev);
    void UpdatePreviewCanvasScale(float scale);

private:

    QPushButton* m_editButton;
    QLabel* m_viewportSizeLabel;
    QLabel* m_canvasScaleLabel;
    std::unique_ptr<CanvasSizeToolbarSection> m_canvasSizeToolbarSection;
};
