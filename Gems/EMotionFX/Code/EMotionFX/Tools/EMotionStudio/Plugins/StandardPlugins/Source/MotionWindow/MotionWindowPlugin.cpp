/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/MotionCommands.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/FileManager.h>
#include <EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/SaveChangedFilesManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionExtractionWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionListWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionPropertiesWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionRetargetingWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionWindowPlugin.h>
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
    class SaveDirtyMotionFilesCallback
        : public SaveDirtyFilesCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(SaveDirtyMotionFilesCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS)

    public:
        SaveDirtyMotionFilesCallback(MotionWindowPlugin* plugin)
            : SaveDirtyFilesCallback() { mPlugin = plugin; }
        ~SaveDirtyMotionFilesCallback()                                                     {}

        enum
        {
            TYPE_ID = 0x00000003
        };
        uint32 GetType() const override                                                     { return TYPE_ID; }
        uint32 GetPriority() const override                                                 { return 3; }
        bool GetIsPostProcessed() const override                                            { return false; }

        void GetDirtyFileNames(AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<ObjectPointer>* outObjects) override
        {
            // get the number of motions and iterate through them
            const uint32 numMotions = EMotionFX::GetMotionManager().GetNumMotions();
            for (uint32 i = 0; i < numMotions; ++i)
            {
                EMotionFX::Motion* motion = EMotionFX::GetMotionManager().GetMotion(i);

                if (motion->GetIsOwnedByRuntime())
                {
                    continue;
                }

                // return in case we found a dirty file
                if (motion->GetDirtyFlag())
                {
                    // add the filename to the dirty filenames array
                    outFileNames->push_back(motion->GetFileName());

                    // add the link to the actual object
                    ObjectPointer objPointer;
                    objPointer.mMotion = motion;
                    outObjects->push_back(objPointer);
                }
            }
        }

        int SaveDirtyFiles(const AZStd::vector<AZStd::string>& filenamesToSave, const AZStd::vector<ObjectPointer>& objects, MCore::CommandGroup* commandGroup) override
        {
            MCORE_UNUSED(filenamesToSave);

            const size_t numObjects = objects.size();
            for (size_t i = 0; i < numObjects; ++i)
            {
                // get the current object pointer and skip directly if the type check fails
                ObjectPointer objPointer = objects[i];
                if (objPointer.mMotion == nullptr)
                {
                    continue;
                }

                EMotionFX::Motion* motion = objPointer.mMotion;
                if (mPlugin->SaveDirtyMotion(motion, commandGroup, false) == DirtyFileManager::CANCELED)
                {
                    return DirtyFileManager::CANCELED;
                }
            }

            return DirtyFileManager::FINISHED;
        }

        const char* GetExtension() const override       { return "motion"; }
        const char* GetFileType() const override        { return "motion"; }
        const AZ::Uuid GetFileRttiType() const override
        {
            return azrtti_typeid<EMotionFX::Motion>();
        }

    private:
        MotionWindowPlugin* mPlugin;
    };


    AZStd::vector<EMotionFX::MotionInstance*> MotionWindowPlugin::mInternalMotionInstanceSelection;


    MotionWindowPlugin::MotionWindowPlugin()
        : EMStudio::DockWidgetPlugin()
    {
        mDialogStack                        = nullptr;
        mMotionListWindow                   = nullptr;
        mMotionPropertiesWindow             = nullptr;
        mMotionExtractionWindow             = nullptr;
        mMotionRetargetingWindow            = nullptr;
        mDirtyFilesCallback                 = nullptr;
        mAddMotionsAction                   = nullptr;
        mSaveAction                         = nullptr;
        mMotionNameLabel                    = nullptr;
    }


    MotionWindowPlugin::~MotionWindowPlugin()
    {
        delete mDialogStack;
        ClearMotionEntries();

        // unregister the command callbacks and get rid of the memory
        for (auto callback : m_callbacks)
        {
            GetCommandManager()->RemoveCommandCallback(callback, true);
        }
        m_callbacks.clear();

        GetMainWindow()->GetDirtyFileManager()->RemoveCallback(mDirtyFilesCallback, false);
        delete mDirtyFilesCallback;
    }


    void MotionWindowPlugin::ClearMotionEntries()
    {
        const size_t numEntries = mMotionEntries.size();
        for (size_t i = 0; i < numEntries; ++i)
        {
            delete mMotionEntries[i];
        }
        mMotionEntries.clear();
    }


    EMStudioPlugin* MotionWindowPlugin::Clone()
    {
        return new MotionWindowPlugin();
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

        QWidget* container = new QWidget(mDock);
        container->setLayout(new QVBoxLayout);
        mDock->setWidget(container);

        QToolBar* toolBar = new QToolBar(container);
        container->layout()->addWidget(toolBar);

        QSplitter* splitterWidget = new QSplitter(container);
        splitterWidget->setOrientation(Qt::Horizontal);
        splitterWidget->setChildrenCollapsible(false);

        container->layout()->addWidget(splitterWidget);

        // create the motion list stack window
        mMotionListWindow = new MotionListWindow(splitterWidget, this);
        mMotionListWindow->Init();
        connect(mMotionListWindow, &MotionListWindow::SaveRequested, this, &MotionWindowPlugin::OnSave);
        connect(mMotionListWindow, &MotionListWindow::RemoveMotionsRequested, this, &MotionWindowPlugin::OnRemoveMotions);
        splitterWidget->addWidget(mMotionListWindow);

        // reinitialize the motion table entries
        ReInit();

        mAddMotionsAction    = toolBar->addAction(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Plus.svg"), tr("Load motions"), this, &MotionWindowPlugin::OnAddMotions);
        mSaveAction          = toolBar->addAction(MysticQt::GetMysticQt()->FindIcon("Images/Menu/FileSave.svg"), tr("Save selected motions"), this, &MotionWindowPlugin::OnSave);

        toolBar->addSeparator();
        AzQtComponents::FilteredSearchWidget* searchWidget = new AzQtComponents::FilteredSearchWidget(toolBar);
        connect(searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, mMotionListWindow, &MotionListWindow::OnTextFilterChanged);
        toolBar->addWidget(searchWidget);

        // create the dialog stack
        assert(mDialogStack == nullptr);
        mDialogStack = new MysticQt::DialogStack(splitterWidget);
        mDialogStack->setMinimumWidth(279);
        splitterWidget->addWidget(mDialogStack);

        // add the motion properties stack window
        mMotionPropertiesWindow = new MotionPropertiesWindow(mDialogStack, this);
        mMotionPropertiesWindow->Init();
        mDialogStack->Add(mMotionPropertiesWindow, "Motion Properties", false, true);

        // add the motion name label
        QWidget* motionName = new QWidget();
        QBoxLayout* motionNameLayout = new QHBoxLayout(motionName);
        mMotionNameLabel = new QLabel();
        QLabel* label = new QLabel(tr("Motion name"));
        motionNameLayout->addWidget(label);
        motionNameLayout->addWidget(mMotionNameLabel);
        motionNameLayout->setStretchFactor(label, 3);
        motionNameLayout->setStretchFactor(mMotionNameLabel, 2);
        mMotionPropertiesWindow->layout()->addWidget(motionName);

        // add the motion extraction stack window
        mMotionExtractionWindow = new MotionExtractionWindow(mDialogStack, this);
        mMotionExtractionWindow->Init();
        mMotionPropertiesWindow->AddSubProperties(mMotionExtractionWindow);

        // add the motion retargeting stack window
        mMotionRetargetingWindow = new MotionRetargetingWindow(mDialogStack, this);
        mMotionRetargetingWindow->Init();
        mMotionPropertiesWindow->AddSubProperties(mMotionRetargetingWindow);

        mMotionPropertiesWindow->FinalizeSubProperties();

        // connect the window activation signal to refresh if reactivated
        connect(mDock, &QDockWidget::visibilityChanged, this, &MotionWindowPlugin::VisibilityChanged);

        // update the new interface and return success
        UpdateInterface();

        // initialize the dirty files callback
        mDirtyFilesCallback = new SaveDirtyMotionFilesCallback(this);
        GetMainWindow()->GetDirtyFileManager()->AddCallback(mDirtyFilesCallback);

        return true;
    }

    void MotionWindowPlugin::OnAddMotions()
    {
        const AZStd::vector<AZStd::string> filenames = GetMainWindow()->GetFileManager()->LoadMotionsFileDialog(mMotionListWindow);
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
        const uint32 numMotions = EMotionFX::GetMotionManager().GetNumMotions();
        AZStd::vector<EMotionFX::Motion*> motionsToRemove;
        motionsToRemove.reserve(numMotions);

        for (uint32 i = 0; i < numMotions; ++i)
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
            MotionListRemoveMotionsFailedWindow removeMotionsFailedWindow(mMotionListWindow, failedRemoveMotions);
            removeMotionsFailedWindow.exec();
        }
    }

    void MotionWindowPlugin::OnRemoveMotions()
    {
        const CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();

        // get the number of selected motions
        const uint32 numMotions = selection.GetNumSelectedMotions();
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
        for (uint32 i = 0; i < numMotions; ++i)
        {
            // Look up the motion by ID, using our backup seleciton list.
            // So even if our selection list in EMotion FX gets modified, we still iterate over the original selection now.
            EMotionFX::Motion* motion = motionManager.FindMotionByID(selectionBackup[i]);
            AZ_Assert(motion, "Expected to find the motion, did the motion id change while saving one of the motions?");

            // in case we modified the motion ask if the user wants to save changes it before removing it
            SaveDirtyMotion(motion, nullptr, true, false);
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
        uint32 lowestRowSelected = MCORE_INVALIDINDEX32;
        const QList<QTableWidgetItem*> selectedItems = mMotionListWindow->GetMotionTable()->selectedItems();
        const int numSelectedItems = selectedItems.size();
        for (int i = 0; i < numSelectedItems; ++i)
        {
            if ((uint32)selectedItems[i]->row() < lowestRowSelected)
            {
                lowestRowSelected = (uint32)selectedItems[i]->row();
            }
        }

        // construct the command group and remove the selected motions
        AZStd::vector<EMotionFX::Motion*> failedRemoveMotions;
        CommandSystem::RemoveMotions(motionsToRemove, &failedRemoveMotions);

        // selected the next row
        if (lowestRowSelected > ((uint32)mMotionListWindow->GetMotionTable()->rowCount() - 1))
        {
            mMotionListWindow->GetMotionTable()->selectRow(lowestRowSelected - 1);
        }
        else
        {
            mMotionListWindow->GetMotionTable()->selectRow(lowestRowSelected);
        }

        // show the window if at least one failed remove motion
        if (!failedRemoveMotions.empty())
        {
            MotionListRemoveMotionsFailedWindow removeMotionsFailedWindow(mMotionListWindow, failedRemoveMotions);
            removeMotionsFailedWindow.exec();
        }
    }

    void MotionWindowPlugin::OnSave()
    {
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const AZ::u32 numMotions = selectionList.GetNumSelectedMotions();
        if (numMotions == 0)
        {
            return;
        }

        // Collect motion ids of the motion to be saved.
        AZStd::vector<AZ::u32> motionIds;
        motionIds.reserve(numMotions);
        for (AZ::u32 i = 0; i < numMotions; ++i)
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
                    mMotionEntries.push_back(new MotionTableEntry(motion));
                    return mMotionListWindow->AddMotionByID(motionID);
                }
            }
        }

        return false;
    }


    bool MotionWindowPlugin::RemoveMotionByIndex(size_t index)
    {
        const uint32 motionID = mMotionEntries[index]->mMotionID;

        delete mMotionEntries[index];
        mMotionEntries.erase(mMotionEntries.begin() + index);

        return mMotionListWindow->RemoveMotionByID(motionID);
    }


    bool MotionWindowPlugin::RemoveMotionById(uint32 motionID)
    {
        const size_t numMotionEntries = mMotionEntries.size();
        for (size_t i = 0; i < numMotionEntries; ++i)
        {
            if (mMotionEntries[i]->mMotionID == motionID)
            {
                return RemoveMotionByIndex(i);
            }
        }

        return false;
    }

    void MotionWindowPlugin::ReInit()
    {
        uint32 i;

        // get the number of motions in the motion library and iterate through them
        const uint32 numLibraryMotions = EMotionFX::GetMotionManager().GetNumMotions();
        for (i = 0; i < numLibraryMotions; ++i)
        {
            // check if we have already added this motion, if not add it
            EMotionFX::Motion* motion = EMotionFX::GetMotionManager().GetMotion(i);
            if (motion->GetIsOwnedByRuntime())
            {
                continue;
            }
            if (FindMotionEntryByID(motion->GetID()) == nullptr)
            {
                mMotionEntries.push_back(new MotionTableEntry(motion));
            }
        }

        // iterate through all motions inside the motion window plugin
        i = 0;
        while (i < mMotionEntries.size())
        {
            MotionTableEntry*   entry   = mMotionEntries[i];
            // check if the motion still is in the motion library, if not also remove it from the motion window plugin
            if (EMotionFX::GetMotionManager().FindMotionIndexByID(entry->mMotionID) == MCORE_INVALIDINDEX32)
            {
                delete mMotionEntries[i];
                mMotionEntries.erase(mMotionEntries.begin() + i);
            }
            else
            {
                i++;
            }
        }

        // update the motion list window
        mMotionListWindow->ReInit();
    }


    void MotionWindowPlugin::UpdateMotions()
    {
        mMotionRetargetingWindow->UpdateMotions();
    }


    void MotionWindowPlugin::UpdateInterface()
    {
        AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = GetSelectedMotionInstances();
        const size_t numMotionInstances = motionInstances.size();
        for (size_t i = 0; i < numMotionInstances; ++i)
        {
            EMotionFX::MotionInstance* motionInstance = motionInstances[i];
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

        if (mMotionNameLabel)
        {
            MotionTableEntry* entry = hasSelectedMotions ? FindMotionEntryByID(selection.GetMotion(0)->GetID()) : nullptr;
            EMotionFX::Motion* motion = entry ? entry->mMotion : nullptr;
            mMotionNameLabel->setText(motion ? motion->GetName() : nullptr);
        }

        const uint32 numMotions = EMotionFX::GetMotionManager().GetNumMotions();

        if (mSaveAction)
        {
            // related to the selected motions
            mSaveAction->setEnabled(hasSelectedMotions);
        }

        if (mMotionListWindow)
        {
            mMotionListWindow->UpdateInterface();
        }
        if (mMotionExtractionWindow)
        {
            mMotionExtractionWindow->UpdateInterface();
        }
        if (mMotionRetargetingWindow)
        {
            mMotionRetargetingWindow->UpdateInterface();
        }
    }



    void MotionWindowPlugin::VisibilityChanged(bool visible)
    {
        if (visible)
        {
            //mMotionRetargetingWindow->UpdateSelection();
            //mMotionExtractionWindow->UpdateExtractionNodeLabel();
        }
    }


    AZStd::vector<EMotionFX::MotionInstance*>& MotionWindowPlugin::GetSelectedMotionInstances()
    {
        const CommandSystem::SelectionList& selectionList               = CommandSystem::GetCommandManager()->GetCurrentSelection();
        const uint32                        numSelectedActorInstances   = selectionList.GetNumSelectedActorInstances();
        const uint32                        numSelectedMotions          = selectionList.GetNumSelectedMotions();

        mInternalMotionInstanceSelection.clear();

        for (uint32 i = 0; i < numSelectedActorInstances; ++i)
        {
            EMotionFX::ActorInstance*   actorInstance       = selectionList.GetActorInstance(i);
            EMotionFX::MotionSystem*    motionSystem        = actorInstance->GetMotionSystem();
            const uint32    numMotionInstances  = motionSystem->GetNumMotionInstances();

            for (uint32 j = 0; j < numSelectedMotions; ++j)
            {
                EMotionFX::Motion* motion = selectionList.GetMotion(j);

                for (uint32 k = 0; k < numMotionInstances; ++k)
                {
                    EMotionFX::MotionInstance* motionInstance = motionSystem->GetMotionInstance(k);
                    if (motionInstance->GetMotion() == motion)
                    {
                        mInternalMotionInstanceSelection.push_back(motionInstance);
                    }
                }
            }
        }

        return mInternalMotionInstanceSelection;
    }


    MotionWindowPlugin::MotionTableEntry* MotionWindowPlugin::FindMotionEntryByID(uint32 motionID)
    {
        const size_t numMotionEntries = mMotionEntries.size();
        for (size_t i = 0; i < numMotionEntries; ++i)
        {
            MotionTableEntry* entry = mMotionEntries[i];
            if (entry->mMotionID == motionID)
            {
                return entry;
            }
        }

        return nullptr;
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

        const size_t numMotions = motions.size();
        for (size_t i = 0; i < numMotions; ++i)
        {
            EMotionFX::Motion*                      motion              = motions[i];
            EMotionFX::PlayBackInfo*                defaultPlayBackInfo = motion->GetDefaultPlayBackInfo();

            // Don't blend in and out of the for previewing animations. We might only see a short bit of it for animations smaller than the blend in/out time.
            defaultPlayBackInfo->mBlendInTime = 0.0f;
            defaultPlayBackInfo->mBlendOutTime = 0.0f;
            defaultPlayBackInfo->mFreezeAtLastFrame = (defaultPlayBackInfo->mNumLoops != EMFX_LOOPFOREVER);

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
        const uint32 numMotions = selection.GetNumSelectedMotions();
        if (numMotions == 0)
        {
            return;
        }

        // create our remove motion command group
        MCore::CommandGroup commandGroup(AZStd::string::format("Stop %u motion instances", numMotions).c_str());

        AZStd::string command;
        for (uint32 i = 0; i < numMotions; ++i)
        {
            MotionWindowPlugin::MotionTableEntry* entry = FindMotionEntryByID(selection.GetMotion(i)->GetID());
            if (entry == nullptr)
            {
                AZ_Error("EMotionFX", false, "Cannot find motion table entry for the given motion.");
                continue;
            }

            command = AZStd::string::format("StopMotionInstances -filename \"%s\"", entry->mMotion->GetFileName());
            commandGroup.AddCommandString(command);
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void MotionWindowPlugin::Render(RenderPlugin* renderPlugin, EMStudioPlugin::RenderInfo* renderInfo)
    {
        MCommon::RenderUtil* renderUtil = renderInfo->mRenderUtil;

        // make sure the render objects are valid
        if (renderPlugin == nullptr || renderUtil == nullptr)
        {
            return;
        }
        /*
        if (mMotionRetargetingWindow->GetRenderMotionBindPose())
        {
            const CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();

            // get the number of selected actor instances and iterate through them
            const uint32 numActorInstances = selection.GetNumSelectedActorInstances();
            for (uint32 j = 0; j < numActorInstances; ++j)
            {
                EMotionFX::ActorInstance*   actorInstance   = selection.GetActorInstance(j);
                EMotionFX::Actor*           actor           = actorInstance->GetActor();

                // get the number of selected motions and iterate through them
                const uint32 numMotions = selection.GetNumSelectedMotions();
                for (uint32 i = 0; i < numMotions; ++i)
                {
                    EMotionFX::Motion* motion = selection.GetMotion(i);
                    if (motion->GetType() == EMotionFX::SkeletalMotion::TYPE_ID)
                    {
                        EMotionFX::SkeletalMotion* skeletalMotion = (EMotionFX::SkeletalMotion*)motion;

                        EMotionFX::AnimGraphPosePool& posePool = EMotionFX::GetEMotionFX().GetThreadData(0)->GetPosePool();
                        EMotionFX::AnimGraphPose* pose = posePool.RequestPose(m_actorInstance);

                        skeletalMotion->CalcMotionBindPose(actor, pose->GetPose());

                        // for all nodes in the actor
                        const uint32 numNodes = actorInstance->GetNumEnabledNodes();
                        for (uint32 n = 0; n < numNodes; ++n)
                        {
                            EMotionFX::Node* curNode = actor->GetSkeleton()->GetNode(actorInstance->GetEnabledNode(n));

                            // skip root nodes, you could also use curNode->IsRootNode()
                            // but we use the parent index here, as we will reuse it
                            uint32 parentIndex = curNode->GetParentIndex();
                            if (parentIndex == MCORE_INVALIDINDEX32)
                            {
                                AZ::Vector3 startPos = mGlobalMatrices[curNode->GetNodeIndex()].GetTranslation();
                                AZ::Vector3 endPos   = startPos + AZ::Vector3(0.0f, 3.0f, 0.0f);
                                renderUtil->RenderLine(startPos, endPos, MCore::RGBAColor(0.0f, 1.0f, 1.0f));
                            }
                            else
                            {
                                AZ::Vector3 startPos = mGlobalMatrices[curNode->GetNodeIndex()].GetTranslation();
                                AZ::Vector3 endPos   = mGlobalMatrices[parentIndex].GetTranslation();
                                renderUtil->RenderLine(startPos, endPos, MCore::RGBAColor(0.0f, 1.0f, 1.0f));
                            }
                        }

                        posePool.FreePose(pose);
                    }
                }
            }
        }
        */
    }


    // constructor
    MotionWindowPlugin::MotionTableEntry::MotionTableEntry(EMotionFX::Motion* motion)
    {
        mMotionID           = motion->GetID();
        mMotion             = motion;
    }


    int MotionWindowPlugin::OnSaveDirtyMotions()
    {
        return GetMainWindow()->GetDirtyFileManager()->SaveDirtyFiles(SaveDirtyMotionFilesCallback::TYPE_ID);
    }


    int MotionWindowPlugin::SaveDirtyMotion(EMotionFX::Motion* motion, [[maybe_unused]] MCore::CommandGroup* commandGroup, bool askBeforeSaving, bool showCancelButton)
    {
        // only process changed files
        if (motion->GetDirtyFlag() == false)
        {
            return DirtyFileManager::NOFILESTOSAVE;
        }

        if (askBeforeSaving)
        {
            EMStudio::GetApp()->setOverrideCursor(QCursor(Qt::ArrowCursor));

            QMessageBox msgBox(GetMainWindow());
            AZStd::string text;

            if (motion->GetFileNameString().empty() == false)
            {
                text = AZStd::string::format("Save changes to '%s'?", motion->GetFileName());
            }
            else if (motion->GetNameString().empty() == false)
            {
                text = AZStd::string::format("Save changes to the motion named '%s'?", motion->GetName());
            }
            else
            {
                text = "Save changes to untitled motion?";
            }

            msgBox.setText(text.c_str());
            msgBox.setWindowTitle("Save Changes");

            if (showCancelButton)
            {
                msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            }
            else
            {
                msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard);
            }

            msgBox.setDefaultButton(QMessageBox::Save);
            msgBox.setIcon(QMessageBox::Question);

            int messageBoxResult = msgBox.exec();
            switch (messageBoxResult)
            {
            case QMessageBox::Save:
            {
                GetMainWindow()->GetFileManager()->SaveMotion(motion->GetID());
                break;
            }
            case QMessageBox::Discard:
            {
                EMStudio::GetApp()->restoreOverrideCursor();
                return DirtyFileManager::FINISHED;
            }
            case QMessageBox::Cancel:
            {
                EMStudio::GetApp()->restoreOverrideCursor();
                return DirtyFileManager::CANCELED;
            }
            }
        }
        else
        {
            // save without asking first
            GetMainWindow()->GetFileManager()->SaveMotion(motion->GetID());
        }

        return DirtyFileManager::FINISHED;
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
        return CallbackAddMotionByID(importMotionCommand->mOldMotionID);
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
        return CallbackRemoveMotion(removeMotionCommand->mOldMotionID);
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
