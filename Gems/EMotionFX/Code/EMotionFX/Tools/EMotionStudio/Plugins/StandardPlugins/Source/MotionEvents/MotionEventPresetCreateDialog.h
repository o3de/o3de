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
#include <QDialog>
#include <EMotionStudio/EMStudioSDK/Source/MotionEventPresetManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/EventDataEditor.h>
#endif

QT_FORWARD_DECLARE_CLASS(QLineEdit)


namespace EMotionFX
{
    class ObjectEditor;
}

namespace EMStudio
{
    class MotionEventPresetCreateDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        AZ_TYPE_INFO(EMStudio::MotionEventPresetCreateDialog, "{644087A8-D442-4A48-AF04-8DD34D9DF4D7}");

        MotionEventPresetCreateDialog(const MotionEventPreset& preset = MotionEventPreset(), QWidget* parent = nullptr);

        MotionEventPreset& GetPreset();

    private slots:
        void OnCreateButton();

    private:
        void Init();

        MotionEventPreset m_preset;
        EMotionFX::ObjectEditor* m_editor;
        EMStudio::EventDataEditor m_eventDataEditor;
    };
} // namespace EMStudio
