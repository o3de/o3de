/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QAction>
#include <QToolBar>
#include <QMenu>
#endif

#include <Integration/Rendering/RenderFlag.h>

namespace EMStudio
{
    class AnimRenderPlugin;

    class AnimViewportToolBar : public QToolBar
    {
    public:
        AnimViewportToolBar(AtomRenderPlugin* plugin, QWidget* parent);
        ~AnimViewportToolBar();

        void LoadSettings();

    private:
        void CreateViewOptionEntry(
            QMenu* menu, const char* menuEntryName, uint32_t actionIndex, bool visible = true, char* iconFileName = nullptr);

        AtomRenderPlugin* m_plugin = nullptr;
        QAction* m_manipulatorActions[RenderOptions::ManipulatorMode::NUM_MODES] = { nullptr };
        QAction* m_renderActions[EMotionFX::ActorRenderFlag::NUM_RENDERFLAGS] = { nullptr };
        QAction* m_followCharacterAction = nullptr;
    };
}
