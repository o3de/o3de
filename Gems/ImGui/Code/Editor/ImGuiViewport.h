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
#include <QTimer>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/vector.h>
#include <Cry_Math.h>
#include <VertexFormats.h>
#endif

namespace ImGui
{
    class ImGuiViewportWidget : public QWidget
    {
        Q_OBJECT
    public:
        ImGuiViewportWidget(QWidget* parent);
        ~ImGuiViewportWidget();

        void Render();

    private slots:
        void RefreshTick();

    private:
        struct SPreviousContext
        {
            int width;
            int height;
            HWND window;
            CCamera renderCamera;
            CCamera systemCamera;
            bool isMainViewport;
        };

        bool CreateRenderContext();
        void DestroyRenderContext();
        void StorePreviousContext();
        void SetCurrentContext();
        void RestorePreviousContext();

        void resizeEvent(QResizeEvent* ev) override;
        bool event(QEvent* ev) override;

        AZStd::stack<SPreviousContext> m_previousContexts;
        unsigned int m_width;
        unsigned int m_height;
        bool m_renderContextCreated;
        bool m_creatingRenderContext;
        int64 m_lastTime;
        float m_lastFrameTime;
        float m_averageFrameTime;
        AZStd::vector<SVF_P3F_C4B_T2F> m_vertBuffer;
        AZStd::vector<uint16> m_idxBuffer;
        QTimer m_updateTimer;
    };
}
