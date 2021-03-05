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
#ifndef __IMGUI_MAINWINDOW_H__
#define __IMGUI_MAINWINDOW_H__

#pragma once

#if !defined(Q_MOC_RUN)
#include <QMainWindow>
#include "IEditor.h"
#include <Editor/ui_ImGuiMainwindow.h>
#endif

namespace Ui
{
    class ImGuiMainWindow;
}

namespace ImGui
{
    class ImGuiViewportWidget;

    class ImGuiMainWindow : public QMainWindow
    {
        Q_OBJECT
    public:

        static const GUID& GetClassID()
        {
            // {ECA9F41C-716E-4395-A096-5A519227F9A4}
            static const GUID guid =
            { 0xeca9f41c, 0x716e, 0x4395, { 0xa0, 0x96, 0x5a, 0x51, 0x92, 0x27, 0xf9, 0xa4 } };

            return guid;
        }

        explicit ImGuiMainWindow(QWidget* parent = nullptr);

    private:
        QScopedPointer<Ui::ImGuiMainWindow> m_ui;

        ImGuiViewportWidget* m_viewport;
    };
}

#endif //__IMGUI_MAINWINDOW_H__