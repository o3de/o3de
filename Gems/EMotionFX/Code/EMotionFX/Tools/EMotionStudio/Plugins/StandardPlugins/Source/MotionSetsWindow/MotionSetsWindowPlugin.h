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
#include <MysticQt/Source/DialogStack.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/MotionSetCommands.h>
#include <EMotionFX/Source/EventHandler.h>
#include "MotionSetWindow.h"
#include "MotionSetManagementWindow.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QDialog>
#endif


namespace EMStudio
{
    // forward declarations
    class SaveDirtyMotionSetFilesCallback;


    class MotionSetsWindowPlugin
        : public EMStudio::DockWidgetPlugin
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MotionSetsWindowPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000234
        };

        MotionSetsWindowPlugin();
        ~MotionSetsWindowPlugin();

        // overloaded
        const char* GetCompileDate() const override     { return MCORE_DATE; }
        const char* GetName() const override            { return "Motion Sets"; }
        uint32 GetClassID() const override              { return MotionSetsWindowPlugin::CLASS_ID; }
        const char* GetCreatorName() const override     { return "O3DE"; }
        float GetVersion() const override               { return 1.0f;  }
        bool GetIsClosable() const override             { return true;  }
        bool GetIsFloatable() const override            { return true;  }
        bool GetIsVertical() const override             { return false; }

        // overloaded main init function
        bool Init() override;
        EMStudioPlugin* Clone() override;
        void OnAfterLoadProject() override;

        void ReInit();

        EMotionFX::MotionSet* GetSelectedSet() const;

        void SetSelectedSet(EMotionFX::MotionSet* motionSet);
        int SaveDirtyMotionSet(EMotionFX::MotionSet* motionSet, MCore::CommandGroup* commandGroup, bool askBeforeSaving, bool showCancelButton = true);

        MotionSetManagementWindow*  GetManagementWindow()                                                       { return m_motionSetManagementWindow; }
        MotionSetWindow*            GetMotionSetWindow()                                                        { return m_motionSetWindow; }

        int OnSaveDirtyMotionSets();
        void LoadMotionSet(AZStd::string filename);

        static bool GetMotionSetCommandInfo(MCore::Command* command, const MCore::CommandLine& parameters,EMotionFX::MotionSet** outMotionSet, MotionSetsWindowPlugin** outPlugin);
        static EMotionFX::MotionSet::MotionEntry* FindBestMatchMotionEntryById(const AZStd::string& motionId);

    public slots:
        void WindowReInit(bool visible);

    private:
        MCORE_DEFINECOMMANDCALLBACK(CommandReinitCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRemoveMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandMotionSetAddMotionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandMotionSetRemoveMotionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandMotionSetAdjustMotionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandLoadMotionSetCallback);
        AZStd::vector<MCore::Command::Callback*> m_callbacks;

        MotionSetManagementWindow*              m_motionSetManagementWindow;
        MotionSetWindow*                        m_motionSetWindow;

        MysticQt::DialogStack*                  m_dialogStack;

        EMotionFX::MotionSet*                   m_selectedSet;

        SaveDirtyMotionSetFilesCallback*        m_dirtyFilesCallback;
    };
} // namespace EMStudio
