/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include <QWidget>
#endif

namespace EMotionFX
{
    class Motion;
    class MotionEventTrack;
    class MotionEvent;
} // namespace EMotionFX

namespace EMStudio
{
    class MotionEventEditor;

    class MotionEventWidget
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MotionEventWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionEventWidget(QWidget* parent);

        void ReInit(EMotionFX::Motion* motion = nullptr, EMotionFX::MotionEvent* motionEvent = nullptr);

    private:
        void Init();

        MotionEventEditor* m_editor = nullptr;
    };
} // namespace EMStudio
