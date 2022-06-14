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
    class MotionPropertiesWindow;

    class MotionSetsWindowPlugin
        : public EMStudio::DockWidgetPlugin
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(MotionSetsWindowPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000234
        };

        MotionSetsWindowPlugin();
        ~MotionSetsWindowPlugin();

        // overloaded
        const char* GetName() const override            { return "Motion Sets"; }
        uint32 GetClassID() const override              { return MotionSetsWindowPlugin::CLASS_ID; }
        bool GetIsClosable() const override             { return true;  }
        bool GetIsFloatable() const override            { return true;  }
        bool GetIsVertical() const override             { return false; }

        // overloaded main init function
        bool Init() override;
        EMStudioPlugin* Clone() const override { return new MotionSetsWindowPlugin(); }
        void OnAfterLoadProject() override;

        void ReInit();

        EMotionFX::MotionSet* GetSelectedSet() const;

        void SetSelectedSet(EMotionFX::MotionSet* motionSet, bool clearSelectionUpfront = false);

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
        MCORE_DEFINECOMMANDCALLBACK(CommandCreateMotionSetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandLoadMotionSetCallback);

        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustMotionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustDefaultPlayBackInfoCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSaveMotionAssetInfoCallback);
        AZStd::vector<MCore::Command::Callback*> m_callbacks;

        MotionSetManagementWindow*              m_motionSetManagementWindow;
        MotionSetWindow*                        m_motionSetWindow;
        MotionPropertiesWindow* m_motionPropertiesWindow = nullptr;
        AZStd::vector<EMotionFX::MotionInstance*> m_cachedSelectedMotionInstances;

        MysticQt::DialogStack*                  m_dialogStack;

        EMotionFX::MotionSet*                   m_selectedSet;
    };
} // namespace EMStudio
