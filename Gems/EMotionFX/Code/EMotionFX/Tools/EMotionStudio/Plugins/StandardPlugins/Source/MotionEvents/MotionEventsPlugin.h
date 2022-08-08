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
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include <EMotionFX/CommandSystem/Source/MotionEventCommands.h>
#include <MysticQt/Source/DialogStack.h>
#include "MotionEventPresetsWidget.h"
#endif

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QShortcut)


namespace EMStudio
{
    class MotionEventsPlugin
        : public EMStudio::DockWidgetPlugin
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(MotionEventsPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000942
        };

        MotionEventsPlugin();
        ~MotionEventsPlugin() = default;

        void Reflect(AZ::ReflectContext* context) override;

        // overloaded
        const char* GetName() const override            { return "Motion Events"; }
        uint32 GetClassID() const override              { return MotionEventsPlugin::CLASS_ID; }
        bool GetIsClosable() const override             { return true;  }
        bool GetIsFloatable() const override            { return true;  }
        bool GetIsVertical() const override             { return false; }

        // overloaded main init function
        bool Init() override;
        EMStudioPlugin* Clone() const override { return new MotionEventsPlugin(); }

        void OnBeforeRemovePlugin(uint32 classID) override;

        MotionEventPresetsWidget* GetPresetsWidget() const                      { return m_motionEventPresetsWidget; }

        void ValidatePluginLinks();

        void FireColorChangedSignal()                                           { emit OnColorChanged(); }

    signals:
        void OnColorChanged();

    public slots:
        void OnEventPresetDropped(QPoint position);
        bool CheckIfIsPresetReadyToDrop();

    private:
        MysticQt::DialogStack*          m_dialogStack;
        MotionEventPresetsWidget*       m_motionEventPresetsWidget;

        TimeViewPlugin*                 m_timeViewPlugin;
    };
} // namespace EMStudio
