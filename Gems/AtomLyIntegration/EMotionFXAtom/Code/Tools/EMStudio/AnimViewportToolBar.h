/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QAction>
#include <QToolBar>
#include <QMenu>

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
            QMenu* menu, const char* menuEntryName, AZ::u8 actionIndex, bool visible = true, const char* iconFileName = nullptr);

        AtomRenderPlugin* m_plugin = nullptr;
        QAction* m_manipulatorActions[RenderOptions::ManipulatorMode::NUM_MODES] = { nullptr };
        QAction* m_renderActions[EMotionFX::ActorRenderFlagIndex::NUM_RENDERFLAGINDEXES] = { nullptr };
        QAction* m_followCharacterAction = nullptr;
    };
}
