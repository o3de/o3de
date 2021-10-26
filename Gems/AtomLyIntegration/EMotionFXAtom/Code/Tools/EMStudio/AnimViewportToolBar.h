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
    class AnimViewportToolBar : public QToolBar
    {
    public:
        AnimViewportToolBar(QWidget* parent = nullptr);
        ~AnimViewportToolBar() = default;

        void SetRenderFlags(EMotionFX::ActorRenderFlagBitset renderFlags);

    private:
        void CreateViewOptionEntry(
            QMenu* menu, const char* menuEntryName, uint32_t actionIndex, bool visible = true, char* iconFileName = nullptr);

        QAction* m_actions[EMotionFX::ActorRenderFlag::NUM_RENDERFLAGS] = { nullptr };
    };
}
