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
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MotionEventsPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000942
        };

        MotionEventsPlugin();
        ~MotionEventsPlugin();

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
        void ReInit();
        void MotionSelectionChanged();
        void WindowReInit(bool visible);
        void OnEventPresetDropped(QPoint position);
        bool CheckIfIsPresetReadyToDrop();

    private:
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustMotionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearSelectionCallback);
        CommandAdjustMotionCallback*    m_adjustMotionCallback;
        CommandSelectCallback*          m_selectCallback;
        CommandUnselectCallback*        m_unselectCallback;
        CommandClearSelectionCallback*  m_clearSelectionCallback;

        MysticQt::DialogStack*          m_dialogStack;
        MotionEventPresetsWidget*       m_motionEventPresetsWidget;

        TimeViewPlugin*                 m_timeViewPlugin;
        TrackHeaderWidget*              m_trackHeaderWidget;
        TrackDataWidget*                m_trackDataWidget;
        MotionWindowPlugin*             m_motionWindowPlugin;
        MotionListWindow*               m_motionListWindow;
        EMotionFX::Motion*              m_motion;
    };
} // namespace EMStudio
