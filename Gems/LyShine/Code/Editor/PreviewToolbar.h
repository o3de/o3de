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
