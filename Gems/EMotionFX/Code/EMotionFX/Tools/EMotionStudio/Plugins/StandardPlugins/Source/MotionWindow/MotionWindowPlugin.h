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
#include <AzCore/std/containers/vector.h>
#include <MysticQt/Source/DialogStack.h>
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include <EMotionFX/CommandSystem/Source/ImporterCommands.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/Commands.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/EventHandler.h>
#endif


QT_FORWARD_DECLARE_CLASS(QLabel)


namespace EMStudio
{
    // forward declaration
    class MotionListWindow;
    class MotionPropertiesWindow;
    class MotionExtractionWindow;
    class MotionRetargetingWindow;
    class SaveDirtyMotionFilesCallback;

    class MotionWindowPlugin
        : public EMStudio::DockWidgetPlugin
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MotionWindowPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000005
        };

        MotionWindowPlugin();
        ~MotionWindowPlugin();

        // overloaded
        const char* GetName() const override            { return "Motions"; }
        uint32 GetClassID() const override              { return MotionWindowPlugin::CLASS_ID; }
        bool GetIsClosable() const override             { return true;  }
        bool GetIsFloatable() const override            { return true;  }
        bool GetIsVertical() const override             { return false; }

        // overloaded main init function
        bool Init() override;
        EMStudioPlugin* Clone() const override { return new MotionWindowPlugin(); }

        void LegacyRender(RenderPlugin* renderPlugin, EMStudioPlugin::RenderInfo* renderInfo) override;

        void ReInit();

        struct MotionTableEntry
        {
            MCORE_MEMORYOBJECTCATEGORY(MotionTableEntry, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

            MotionTableEntry(EMotionFX::Motion* motion);

            EMotionFX::Motion*          m_motion;
            uint32                      m_motionId;
        };

        MotionTableEntry* FindMotionEntryByID(uint32 motionID);
        MCORE_INLINE size_t GetNumMotionEntries()                                                   { return m_motionEntries.size(); }
        MCORE_INLINE MotionTableEntry* GetMotionEntry(size_t index)                                 { return m_motionEntries[index]; }
        bool AddMotion(uint32 motionID);
        bool RemoveMotionByIndex(size_t index);
        bool RemoveMotionById(uint32 motionID);

        static AZStd::vector<EMotionFX::MotionInstance*>& GetSelectedMotionInstances();

        MCORE_INLINE MotionRetargetingWindow* GetMotionRetargetingWindow()                          { return m_motionRetargetingWindow; }
        MCORE_INLINE MotionExtractionWindow* GetMotionExtractionWindow()                            { return m_motionExtractionWindow; }
        MCORE_INLINE MotionListWindow* GetMotionListWindow()                                        { return m_motionListWindow; }
        MCORE_INLINE const char* GetDefaultNodeSelectionLabelText()                                 { return "Click to select node"; }

        int OnSaveDirtyMotions();
        int SaveDirtyMotion(EMotionFX::Motion* motion, MCore::CommandGroup* commandGroup, bool askBeforeSaving, bool showCancelButton = true);

        void PlayMotions(const AZStd::vector<EMotionFX::Motion*>& motions);
        void PlayMotion(EMotionFX::Motion* motion);
        void StopSelectedMotions();

    public slots:
        void UpdateInterface();
        void UpdateMotions();
        void VisibilityChanged(bool visible);
        void OnAddMotions();
        void OnClearMotions();
        void OnRemoveMotions();
        void OnSave();


    private:
        void ClearMotionEntries();

        // declare the callbacks
        MCORE_DEFINECOMMANDCALLBACK(CommandImportMotionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandLoadMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveMotionPostCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSaveMotionAssetInfoCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustDefaultPlayBackInfoCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustMotionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandScaleMotionDataCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);

        AZStd::vector<MCore::Command::Callback*> m_callbacks;

        AZStd::vector<MotionTableEntry*>                m_motionEntries;

        MysticQt::DialogStack*                          m_dialogStack;
        MotionListWindow*                               m_motionListWindow;
        MotionPropertiesWindow*                         m_motionPropertiesWindow;
        MotionExtractionWindow*                         m_motionExtractionWindow;
        MotionRetargetingWindow*                        m_motionRetargetingWindow;

        SaveDirtyMotionFilesCallback*                   m_dirtyFilesCallback;

        QAction*                                        m_addMotionsAction;
        QAction*                                        m_saveAction;

        QLabel*                                         m_motionNameLabel;

        static AZStd::vector<EMotionFX::MotionInstance*> s_internalMotionInstanceSelection;
    };
} // namespace EMStudio
