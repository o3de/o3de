/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/limits.h>
#include <Editor/InspectorBus.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionFX/CommandSystem/Source/MotionCommands.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/FileManager.h>
#include <EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/SaveChangedFilesManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionListWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionPropertiesWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionWindowPlugin.h>
#include <Editor/SaveDirtyFilesCallbacks.h>
#include <MCore/Source/LogManager.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <QApplication>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QToolBar>
#include <QVBoxLayout>


namespace EMStudio
{
    AZStd::vector<EMotionFX::MotionInstance*> MotionWindowPlugin::s_internalMotionInstanceSelection;


    MotionWindowPlugin::MotionWindowPlugin()
        : EMStudio::DockWidgetPlugin()
    {
        m_motionListWindow                   = nullptr;
        m_motionPropertiesWindow             = nullptr;
        m_addMotionsAction                   = nullptr;
        m_saveAction                         = nullptr;
    }

    MotionWindowPlugin::~MotionWindowPlugin()
    {
        delete m_motionPropertiesWindow;
        m_motionPropertiesWindow = nullptr;

        ClearMotionEntries();

        // unregister the command callbacks and get rid of the memory
        for (auto callback : m_callbacks)
        {
            GetCommandManager()->RemoveCommandCallback(callback, true);
        }
        m_callbacks.clear();
    }

    void MotionWindowPlugin::ClearMotionEntries()
    {
        const size_t numEntries = m_motionEntries.size();
        for (size_t i = 0; i < numEntries; ++i)
        {
            delete m_motionEntries[i];
        }
        m_motionEntries.clear();
    }

    bool MotionWindowPlugin::Init()
    {
        //LogInfo("Initializing motion window.");

        // create and register the command callbacks only (only execute this code once for all plugins)
        GetCommandManager()->RegisterCommandCallback<CommandImportMotionCallback>("ImportMotion", m_callbacks, true);
        GetCommandManager()->RegisterCommandCallback<CommandRemoveMotionPostCallback>("RemoveMotion", m_callbacks, false);
        GetCommandManager()->RegisterCommandCallback<CommandSaveMotionAssetInfoCallback>("SaveMotionAssetInfo", m_callbacks, false);
        GetCommandManager()->RegisterCommandCallback<CommandAdjustDefaultPlayBackInfoCallback>("AdjustDefaultPlayBackInfo", m_callbacks, false);
        GetCommandManager()->RegisterCommandCallback<CommandAdjustMotionCallback>("AdjustMotion", m_callbacks, false);
        GetCommandManager()->RegisterCommandCallback<CommandLoadMotionSetCallback>("LoadMotionSet", m_callbacks, false);
        GetCommandManager()->RegisterCommandCallback<CommandScaleMotionDataCallback>("ScaleMotionData", m_callbacks, false);
        GetCommandManager()->RegisterCommandCallback<CommandSelectCallback>("Select", m_callbacks, false);

        QWidget* container = new QWidget(m_dock);
        container->setLayout(new QVBoxLayout);
        m_dock->setWidget(container);

        QToolBar* toolBar = new QToolBar(container);
        container->layout()->addWidget(toolBar);

        // create the motion list stack window
        m_motionListWindow = new MotionListWindow(container, this);
        m_motionListWindow->Init();
        connect(m_motionListWindow, &MotionListWindow::SaveRequested, this, &MotionWindowPlugin::OnSave);
        connect(m_motionListWindow, &MotionListWindow::RemoveMotionsRequested, this, &MotionWindowPlugin::OnRemoveMotions);
        container->layout()->addWidget(m_motionListWindow);

        // reinitialize the motion table entries
        ReInit();

        m_addMotionsAction    = toolBar->addAction(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.svg"), tr("Load motions"), this, &MotionWindowPlugin::OnAddMotions);
        m_saveAction          = toolBar->addAction(MysticQt::GetMysticQt()->FindIcon("Images/Menu/FileSave.svg"), tr("Save selected motions"), this, &MotionWindowPlugin::OnSave);

        toolBar->addSeparator();
        AzQtComponents::FilteredSearchWidget* searchWidget = new AzQtComponents::FilteredSearchWidget(toolBar);
        connect(searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, m_motionListWindow, &MotionListWindow::OnTextFilterChanged);
        toolBar->addWidget(searchWidget);

        // add the motion properties stack window
        m_motionPropertiesWindow = new MotionPropertiesWindow(nullptr, this);
        m_motionPropertiesWindow->hide();

        // update the new interface and return success
        UpdateInterface();

        return true;
    }

    void MotionWindowPlugin::OnAddMotions()
    {
        const AZStd::vector<AZStd::string> filenames = GetMainWindow()->GetFileManager()->LoadMotionsFileDialog(m_motionListWindow);
        CommandSystem::LoadMotionsCommand(filenames);
    }

    void MotionWindowPlugin::OnClearMotions()
    {
        // show the save dirty files window before
        if (OnSaveDirtyMotions() == DirtyFileManager::CANCELED)
        {
            return;
        }

        // iterate through the motions and put them into some array
        const size_t numMotions = EMotionFX::GetMotionManager().GetNumMotions();
        AZStd::vector<EMotionFX::Motion*> motionsToRemove;
        motionsToRemove.reserve(numMotions);

        for (size_t i = 0; i < numMotions; ++i)
        {
            EMotionFX::Motion* motion = EMotionFX::GetMotionManager().GetMotion(i);
            if (motion->GetIsOwnedByRuntime())
            {
                continue;
            }
            motionsToRemove.push_back(motion);
        }

        // construct the command group and remove the selected motions
        AZStd::vector<EMotionFX::Motion*> failedRemoveMotions;
        CommandSystem::RemoveMotions(motionsToRemove, &failedRemoveMotions);

        // show the window if at least one failed remove motion
        if (!failedRemoveMotions.empty())
        {
            MotionListRemoveMotionsFailedWindow removeMotionsFailedWindow(m_motionListWindow, failedRemoveMotions);
            removeMotionsFailedWindow.exec();
        }
    }

    void MotionWindowPlugin::OnRemoveMotions()
    {
        const CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();

        // get the number of selected motions
        const size_t numMotions = selection.GetNumSelectedMotions();
        if (numMotions == 0)
        {
            return;
        }

        AZStd::vector<EMotionFX::Motion*> motionsToRemove;
        motionsToRemove.reserve(numMotions);

        // Make a backup of the selected motion list, but store motion id's instead of pointers.
        // We do this because motion's get reloaded when saving them, which might change the selection list pointers.
        // We use this backup list because the SaveDirtyMotion function below will trigger a reload of the motion asset.
        // Part of that reload is a RemoveMotion command. This RemoveMotion command also modifies the selection. We would
        // get a crash if we do not store a backup and use that list as the size of the seleciton list will shrink with each
        // call to the SaveDirtyMotion method.
        AZStd::vector<AZ::u32> selectionBackup;
        selectionBackup.reserve(numMotions);
        for (AZ::u32 i = 0; i < numMotions; ++i)
        {
            selectionBackup.emplace_back(selection.GetMotion(i)->GetID());
        }

        // Save all dirty motion files.
        EMotionFX::MotionManager& motionManager = EMotionFX::GetMotionManager();
        for (size_t i = 0; i < numMotions; ++i)
        {
            // Look up the motion by ID, using our backup seleciton list.
            // So even if our selection list in EMotion FX gets modified, we still iterate over the original selection now.
            EMotionFX::Motion* motion = motionManager.FindMotionByID(selectionBackup[i]);
            AZ_Assert(motion, "Expected to find the motion, did the motion id change while saving one of the motions?");

            // in case we modified the motion ask if the user wants to save changes it before removing it
            SaveDirtyMotionFilesCallback::SaveDirtyMotion(motion, /*commandGroup=*/nullptr, /*askBeforeSaving=*/true, /*showCancelButton=*/false);
            motionsToRemove.push_back(motion);
        }


        // Make sure we restore the array of motions to removed with valid pointers.
        // Because the motions get reloaded when saving (as the AP picks up changes), the pointers might also change.
        // So we rebuild the array by looking up their new pointer values again using the motion ids, which don't change when reloading.
        motionsToRemove.clear();
        for (const AZ::u32 motionId : selectionBackup)
        {
            EMotionFX::Motion* motion = motionManager.FindMotionByID(motionId);
            if (motion)
            {
                motionsToRemove.emplace_back(motion);
            }
        }

        // find the lowest row selected
        int lowestRowSelected = AZStd::numeric_limits<int>::max();
        const QList<QTableWidgetItem*> selectedItems = m_motionListWindow->GetMotionTable()->selectedItems();
        for (const QTableWidgetItem* selectedItem : selectedItems)
        {
            lowestRowSelected = AZStd::min(lowestRowSelected, selectedItem->row());
        }

        // construct the command group and remove the selected motions
        AZStd::vector<EMotionFX::Motion*> failedRemoveMotions;
        CommandSystem::RemoveMotions(motionsToRemove, &failedRemoveMotions);

        // selected the next row
        if (lowestRowSelected > (m_motionListWindow->GetMotionTable()->rowCount() - 1))
        {
            m_motionListWindow->GetMotionTable()->selectRow(lowestRowSelected - 1);
        }
        else
        {
            m_motionListWindow->GetMotionTable()->selectRow(lowestRowSelected);
        }

        // show the window if at least one failed remove motion
        if (!failedRemoveMotions.empty())
        {
            MotionListRemoveMotionsFailedWindow removeMotionsFailedWindow(m_motionListWindow, failedRemoveMotions);
            removeMotionsFailedWindow.exec();
        }
    }

    void MotionWindowPlugin::OnSave()
    {
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const size_t numMotions = selectionList.GetNumSelectedMotions();
        if (numMotions == 0)
        {
            return;
        }

        // Collect motion ids of the motion to be saved.
        AZStd::vector<AZ::u32> motionIds;
        motionIds.reserve(numMotions);
        for (size_t i = 0; i < numMotions; ++i)
        {
            const EMotionFX::Motion* motion = selectionList.GetMotion(i);
            motionIds.push_back(motion->GetID());
        }

        // Save all selected motions.
        for (const AZ::u32 motionId : motionIds)
        {
            const EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionId);
            AZ_Assert(motion, "Expected to find the motion pointer for motion with id %d.", motionId);
            if (motion->GetDirtyFlag())
            {
                GetMainWindow()->GetFileManager()->SaveMotion(motionId);
            }
        }
    }

    bool MotionWindowPlugin::AddMotion(uint32 motionID)
    {
        if (FindMotionEntryByID(motionID) == nullptr)
        {
            EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
            if (motion)
            {
                if (!motion->GetIsOwnedByRuntime())
                {
                    m_motionEntries.push_back(new MotionTableEntry(motion));
                    return m_motionListWindow->AddMotionByID(motionID);
                }
            }
        }

        return false;
    }


    bool MotionWindowPlugin::RemoveMotionByIndex(size_t index)
    {
        const uint32 motionID = m_motionEntries[index]->m_motionId;

        delete m_motionEntries[index];
        m_motionEntries.erase(m_motionEntries.begin() + index);

        return m_motionListWindow->RemoveMotionByID(motionID);
    }


    bool MotionWindowPlugin::RemoveMotionById(uint32 motionID)
    {
        const size_t numMotionEntries = m_motionEntries.size();
        for (size_t i = 0; i < numMotionEntries; ++i)
        {
            if (m_motionEntries[i]->m_motionId == motionID)
            {
                return RemoveMotionByIndex(i);
            }
        }

        return false;
    }

    void MotionWindowPlugin::ReInit()
    {
        // get the number of motions in the motion library and iterate through them
        const size_t numLibraryMotions = EMotionFX::GetMotionManager().GetNumMotions();
        for (size_t i = 0; i < numLibraryMotions; ++i)
        {
            // check if we have already added this motion, if not add it
            EMotionFX::Motion* motion = EMotionFX::GetMotionManager().GetMotion(i);
            if (motion->GetIsOwnedByRuntime())
            {
                continue;
            }
            if (FindMotionEntryByID(motion->GetID()) == nullptr)
            {
                m_motionEntries.push_back(new MotionTableEntry(motion));
            }
        }

        // iterate through all motions inside the motion window plugin
        AZStd::erase_if(m_motionEntries, [](MotionTableEntry* entry)
        {
            // check if the motion still is in the motion library, if not also remove it from the motion window plugin
            if (EMotionFX::GetMotionManager().FindMotionIndexByID(entry->m_motionId) == InvalidIndex)
            {
                delete entry;
                return true;
            }
            return false;
        });

        // update the motion list window
        m_motionListWindow->ReInit();
    }


    void MotionWindowPlugin::UpdateMotions()
    {
        m_motionPropertiesWindow->UpdateMotions();
    }


    void MotionWindowPlugin::UpdateInterface()
    {
        AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = GetSelectedMotionInstances();
        for (EMotionFX::MotionInstance* motionInstance : motionInstances)
        {
            EMotionFX::Motion* motion = motionInstance->GetMotion();
            motionInstance->InitFromPlayBackInfo(*motion->GetDefaultPlayBackInfo(), false);

            // security check for motion mirroring, disable motion mirroring in case the node
            EMotionFX::ActorInstance* actorInstance = motionInstance->GetActorInstance();
            EMotionFX::Actor* actor = actorInstance->GetActor();
            if (actor->GetHasMirrorInfo() == false)
            {
                motionInstance->SetMirrorMotion(false);
            }
        }

        const CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        const bool hasSelectedMotions = selection.GetNumSelectedMotions() > 0;

        if (m_saveAction)
        {
            // related to the selected motions
            m_saveAction->setEnabled(hasSelectedMotions);
        }

        if (m_motionListWindow)
        {
            m_motionListWindow->UpdateInterface();
        }

        if (m_motionPropertiesWindow)
        {
            m_motionPropertiesWindow->UpdateInterface();
        }
    }

    AZStd::vector<EMotionFX::MotionInstance*>& MotionWindowPlugin::GetSelectedMotionInstances()
    {
        const CommandSystem::SelectionList& selectionList               = CommandSystem::GetCommandManager()->GetCurrentSelection();
        const size_t                        numSelectedActorInstances   = selectionList.GetNumSelectedActorInstances();
        const size_t                        numSelectedMotions          = selectionList.GetNumSelectedMotions();

        s_internalMotionInstanceSelection.clear();

        for (size_t i = 0; i < numSelectedActorInstances; ++i)
        {
            EMotionFX::ActorInstance*   actorInstance       = selectionList.GetActorInstance(i);
            EMotionFX::MotionSystem*    motionSystem        = actorInstance->GetMotionSystem();
            const size_t    numMotionInstances  = motionSystem->GetNumMotionInstances();

            for (size_t j = 0; j < numSelectedMotions; ++j)
            {
                EMotionFX::Motion* motion = selectionList.GetMotion(j);

                for (size_t k = 0; k < numMotionInstances; ++k)
                {
                    EMotionFX::MotionInstance* motionInstance = motionSystem->GetMotionInstance(k);
                    if (motionInstance->GetMotion() == motion)
                    {
                        s_internalMotionInstanceSelection.push_back(motionInstance);
                    }
                }
            }
        }

        return s_internalMotionInstanceSelection;
    }


    MotionWindowPlugin::MotionTableEntry* MotionWindowPlugin::FindMotionEntryByID(uint32 motionID)
    {
        const auto foundEntry = AZStd::find_if(begin(m_motionEntries), end(m_motionEntries), [motionID](const MotionTableEntry* entry)
        {
            return entry->m_motionId == motionID;
        });
        return foundEntry != end(m_motionEntries) ? *foundEntry : nullptr;
    }


    void MotionWindowPlugin::PlayMotion(EMotionFX::Motion* motion)
    {
        AZStd::vector<EMotionFX::Motion*> motions;
        motions.push_back(motion);
        PlayMotions(motions);
    }


    void MotionWindowPlugin::PlayMotions(const AZStd::vector<EMotionFX::Motion*>& motions)
    {
        AZStd::string command, commandParameters;
        MCore::CommandGroup commandGroup("Play motions");

        for (EMotionFX::Motion* motion : motions)
        {
            EMotionFX::PlayBackInfo*                defaultPlayBackInfo = motion->GetDefaultPlayBackInfo();

            // Don't blend in and out of the for previewing animations. We might only see a short bit of it for animations smaller than the blend in/out time.
            defaultPlayBackInfo->m_blendInTime = 0.0f;
            defaultPlayBackInfo->m_blendOutTime = 0.0f;
            defaultPlayBackInfo->m_freezeAtLastFrame = (defaultPlayBackInfo->m_numLoops != EMFX_LOOPFOREVER);

            commandParameters = CommandSystem::CommandPlayMotion::PlayBackInfoToCommandParameters(defaultPlayBackInfo);

            command = AZStd::string::format("PlayMotion -filename \"%s\" %s", motion->GetFileName(), commandParameters.c_str());
            commandGroup.AddCommandString(command);
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void MotionWindowPlugin::StopSelectedMotions()
    {
        const CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();

        // get the number of selected motions
        const size_t numMotions = selection.GetNumSelectedMotions();
        if (numMotions == 0)
        {
            return;
        }

        // create our remove motion command group
        MCore::CommandGroup commandGroup(AZStd::string::format("Stop %zu motion instances", numMotions).c_str());

        AZStd::string command;
        for (size_t i = 0; i < numMotions; ++i)
        {
            MotionWindowPlugin::MotionTableEntry* entry = FindMotionEntryByID(selection.GetMotion(i)->GetID());
            if (entry == nullptr)
            {
                AZ_Error("EMotionFX", false, "Cannot find motion table entry for the given motion.");
                continue;
            }

            command = AZStd::string::format("StopMotionInstances -filename \"%s\"", entry->m_motion->GetFileName());
            commandGroup.AddCommandString(command);
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void MotionWindowPlugin::LegacyRender(RenderPlugin* renderPlugin, EMStudioPlugin::RenderInfo* renderInfo)
    {
        MCommon::RenderUtil* renderUtil = renderInfo->m_renderUtil;

        // make sure the render objects are valid
        if (renderPlugin == nullptr || renderUtil == nullptr)
        {
            return;
        }
    }


    // constructor
    MotionWindowPlugin::MotionTableEntry::MotionTableEntry(EMotionFX::Motion* motion)
    {
        m_motionId           = motion->GetID();
        m_motion             = motion;
    }


    int MotionWindowPlugin::OnSaveDirtyMotions()
    {
        return GetMainWindow()->GetDirtyFileManager()->SaveDirtyFiles(SaveDirtyMotionFilesCallback::TYPE_ID);
    }

    //-----------------------------------------------------------------------------------------
    // Command callbacks
    //-----------------------------------------------------------------------------------------

    bool ReInitMotionWindowPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionWindowPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        MotionWindowPlugin* motionWindowPlugin = (MotionWindowPlugin*)plugin;
        motionWindowPlugin->ReInit();

        return true;
    }


    bool CallbackAddMotionByID(uint32 motionID)
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionWindowPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        MotionWindowPlugin* motionWindowPlugin = (MotionWindowPlugin*)plugin;
        motionWindowPlugin->AddMotion(motionID);
        return true;
    }


    bool CallbackRemoveMotion(uint32 motionID)
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionWindowPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        MotionWindowPlugin* motionWindowPlugin = (MotionWindowPlugin*)plugin;

        // Note: this has to use the index as the plugin always store a synced copy of all motions and this callback is called after the RemoveMotion command has been applied
        // this means the motion is not in the motion manager anymore
        motionWindowPlugin->RemoveMotionById(motionID);

        // An invalid motion id can be passed in case there is a command group where a remove a motion set is before remove a motion command while the motion was in the motion set.
        // In that case the RemoveMotion command can't fill the motion id for the command object as the motion object is already destructed. The root of this issue is that motion sets don't use
        // the asset system yet, nor are they constructed/destructed using the command system like all other motions.
        // For this case we'll call re-init which cleans entries to non-loaded motions and syncs the UI.
        if (motionID == MCORE_INVALIDINDEX32)
        {
            motionWindowPlugin->ReInit();
        }

        return true;
    }


    bool MotionWindowPlugin::CommandImportMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(commandLine);
        CommandSystem::CommandImportMotion* importMotionCommand = static_cast<CommandSystem::CommandImportMotion*>(command);
        return CallbackAddMotionByID(importMotionCommand->m_oldMotionId);
    }


    bool MotionWindowPlugin::CommandImportMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        // calls the RemoveMotion command internally, so the callback from that is already called
        return true;
    }


    bool MotionWindowPlugin::CommandRemoveMotionPostCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(commandLine);
        CommandSystem::CommandRemoveMotion* removeMotionCommand = static_cast<CommandSystem::CommandRemoveMotion*>(command);
        return CallbackRemoveMotion(removeMotionCommand->m_oldMotionId);
    }


    bool MotionWindowPlugin::CommandRemoveMotionPostCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);

        // calls the ImportMotion command internally, so the callback from that is already called
        return true;
    }


    bool MotionWindowPlugin::CommandSaveMotionAssetInfoCallback::Execute([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        return ReInitMotionWindowPlugin();
    }


    bool MotionWindowPlugin::CommandSaveMotionAssetInfoCallback::Undo([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        return ReInitMotionWindowPlugin();
    }


    bool MotionWindowPlugin::CommandScaleMotionDataCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        if (commandLine.GetValueAsBool("skipInterfaceUpdate", command))
        {
            return true;
        }
        return ReInitMotionWindowPlugin();
    }


    bool MotionWindowPlugin::CommandScaleMotionDataCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        if (commandLine.GetValueAsBool("skipInterfaceUpdate", command))
        {
            return true;
        }
        return ReInitMotionWindowPlugin();
    }


    bool MotionWindowPlugin::CommandLoadMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)      { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitMotionWindowPlugin(); }
    bool MotionWindowPlugin::CommandLoadMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)         { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitMotionWindowPlugin(); }


    bool MotionWindowPlugin::CommandAdjustMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return ReInitMotionWindowPlugin();
    }


    bool MotionWindowPlugin::CommandAdjustMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return ReInitMotionWindowPlugin();
    }

    bool UpdateInterfaceMotionWindowPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionWindowPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        MotionWindowPlugin* motionWindowPlugin = (MotionWindowPlugin*)plugin;
        motionWindowPlugin->UpdateInterface();

        return true;
    }


    bool MotionWindowPlugin::CommandAdjustDefaultPlayBackInfoCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)  { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateInterfaceMotionWindowPlugin(); }
    bool MotionWindowPlugin::CommandAdjustDefaultPlayBackInfoCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateInterfaceMotionWindowPlugin(); }


    bool MotionWindowPlugin::CommandSelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);

        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine))
        {
            EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionWindowPlugin::CLASS_ID);
            if (plugin == nullptr)
            {
                return false;
            }

            MotionWindowPlugin* motionWindowPlugin = static_cast<MotionWindowPlugin*>(plugin);
            motionWindowPlugin->UpdateInterface();
        }

        return true;
    }

    bool MotionWindowPlugin::CommandSelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);

        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine))
        {
            EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionWindowPlugin::CLASS_ID);
            if (plugin == nullptr)
            {
                return false;
            }

            MotionWindowPlugin* motionWindowPlugin = static_cast<MotionWindowPlugin*>(plugin);
            motionWindowPlugin->UpdateInterface();
        }

        return true;
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/moc_MotionWindowPlugin.cpp>
