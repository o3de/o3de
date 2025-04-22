/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AudioControlsEditorWindow.h>

#include <AzCore/Utils/Utils.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <ATLControlsModel.h>
#include <ATLControlsPanel.h>
#include <AudioControlsEditorPlugin.h>
#include <AudioControlsEditorUndo.h>
#include <AudioFileUtils.h>
#include <AudioSystemPanel.h>
#include <IAudioSystem.h>
#include <ImplementationManager.h>
#include <InspectorPanel.h>
#include <QAudioControlEditorIcons.h>

#include <DockTitleBarWidget.h>

#include <QPaintEvent>
#include <QPushButton>
#include <QApplication>
#include <QPainter>
#include <QMessageBox>


void InitACEResources()
{
    Q_INIT_RESOURCE(AudioControlsEditorUI);
}

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    // static
    bool CAudioControlsEditorWindow::m_wasClosed = false;

    //-------------------------------------------------------------------------------------------//
    CAudioControlsEditorWindow::CAudioControlsEditorWindow(QWidget* parent)
        : QMainWindow(parent)
    {
        InitACEResources();

        setupUi(this);

        m_pATLModel = CAudioControlsEditorPlugin::GetATLModel();
        IAudioSystemEditor* pAudioSystemImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
        if (pAudioSystemImpl)
        {
            m_pATLControlsPanel = new CATLControlsPanel(m_pATLModel, CAudioControlsEditorPlugin::GetControlsTree());
            m_pInspectorPanel = new CInspectorPanel(m_pATLModel);
            m_pAudioSystemPanel = new CAudioSystemPanel();

            CDockTitleBarWidget* pTitleBar = new CDockTitleBarWidget(m_pATLControlsDockWidget);
            m_pATLControlsDockWidget->setTitleBarWidget(pTitleBar);

            pTitleBar = new CDockTitleBarWidget(m_pInspectorDockWidget);
            m_pInspectorDockWidget->setTitleBarWidget(pTitleBar);

            pTitleBar = new CDockTitleBarWidget(m_pMiddlewareDockWidget);
            m_pMiddlewareDockWidget->setTitleBarWidget(pTitleBar);

            // Custom title based on Middleware name
            m_pMiddlewareDockWidget->setWindowTitle(QString(pAudioSystemImpl->GetName().c_str()) + " Controls");

            m_pATLControlsDockLayout->addWidget(m_pATLControlsPanel);
            m_pInspectorDockLayout->addWidget(m_pInspectorPanel);
            m_pMiddlewareDockLayout->addWidget(m_pAudioSystemPanel);

            Update();

            connect(m_pATLControlsPanel, SIGNAL(SelectedControlChanged()), this, SLOT(UpdateInspector()));
            connect(m_pATLControlsPanel, SIGNAL(SelectedControlChanged()), this, SLOT(UpdateFilterFromSelection()));
            connect(m_pATLControlsPanel, SIGNAL(ControlTypeFiltered(EACEControlType, bool)), this, SLOT(FilterControlType(EACEControlType, bool)));

            connect(CAudioControlsEditorPlugin::GetImplementationManager(), SIGNAL(ImplementationChanged()), this, SLOT(Update()));

            connect(&m_fileSystemWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(ReloadMiddlewareData()));

            GetIEditor()->RegisterNotifyListener(this);

            // LY-11309: this call to reload middleware data will force refresh of the data when
            // making changes to the middleware project while the AudioControlsEditor window is closed.
            if (m_wasClosed)
            {
                ReloadMiddlewareData();
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    CAudioControlsEditorWindow::~CAudioControlsEditorWindow()
    {
        GetIEditor()->UnregisterNotifyListener(this);
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::StartWatchingFolder(const AZStd::string_view folder)
    {
        m_fileSystemWatcher.addPath(folder.data());

        auto fileIO = AZ::IO::FileIOBase::GetInstance();
        auto foundFiles = Audio::FindFilesInPath(folder, "*");
        for (auto& file : foundFiles)
        {
            if (fileIO->IsDirectory(file.c_str()))
            {
                AZ::IO::FixedMaxPath resolvedPath;
                fileIO->ReplaceAlias(resolvedPath, file);
                StartWatchingFolder(file.Native());
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::closeEvent(QCloseEvent* pEvent)
    {
        if (m_pATLModel && m_pATLModel->IsDirty())
        {
            QMessageBox messageBox(this);
            messageBox.setText(tr("There are unsaved changes."));
            messageBox.setInformativeText(tr("Do you want to save your changes?"));
            messageBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            messageBox.setDefaultButton(QMessageBox::Save);
            messageBox.setWindowTitle("Audio Controls Editor");
            switch (messageBox.exec())
            {
            case QMessageBox::Save:
                QApplication::setOverrideCursor(Qt::WaitCursor);
                Save();
                QApplication::restoreOverrideCursor();
                pEvent->accept();
                break;
            case QMessageBox::Discard:
                pEvent->accept();
                break;
            default:
                pEvent->ignore();
                return;
            }
        }
        else
        {
            pEvent->accept();
        }

        // If the close event was accepted, set this flag so next time the window opens it will refresh the middleware data.
        m_wasClosed = true;
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::Reload()
    {
        bool bReload = true;
        if (m_pATLModel && m_pATLModel->IsDirty())
        {
            QMessageBox messageBox(this);
            messageBox.setText(tr("If you reload you will lose all your unsaved changes."));
            messageBox.setInformativeText(tr("Are you sure you want to reload?"));
            messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            messageBox.setDefaultButton(QMessageBox::No);
            messageBox.setWindowTitle("Audio Controls Editor");
            bReload = (messageBox.exec() == QMessageBox::Yes);
        }

        if (bReload)
        {
            CAudioControlsEditorPlugin::ReloadModels();
            Update();
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::Update()
    {
        if (!m_pATLControlsPanel)
        {
            return;
        }

        m_pATLControlsPanel->Reload();
        m_pAudioSystemPanel->Reload();
        UpdateInspector();
        IAudioSystemEditor* pAudioSystemImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
        if (pAudioSystemImpl)
        {
            StartWatchingFolder(pAudioSystemImpl->GetDataPath().LexicallyNormal().Native());
            m_pMiddlewareDockWidget->setWindowTitle(QString(pAudioSystemImpl->GetName().c_str()) + " Controls");
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::RefreshAudioSystem()
    {
        if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get();
            audioSystem != nullptr)
        {
            QString sLevelName = GetIEditor()->GetLevelName();

            if (QString::compare(sLevelName, "Untitled", Qt::CaseInsensitive) == 0)
            {
                // Rather pass empty QString to indicate that no level is loaded!
                sLevelName = QString();
            }

            audioSystem->RefreshAudioSystem(sLevelName.toUtf8().data());
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::Save()
    {
        bool bPreloadsChanged = m_pATLModel->IsTypeDirty(eACET_PRELOAD);
        CAudioControlsEditorPlugin::SaveModels();
        UpdateAudioSystemData();

        // if preloads have been modified, ask the user if s/he wants to refresh the audio system
        if (bPreloadsChanged)
        {
            QMessageBox messageBox(this);
            messageBox.setText(tr("Preload requests have been modified.\n\nFor the new data to be loaded the audio system needs to be refreshed, this will stop all currently playing audio. Do you want to do this now?\n\nYou can always refresh manually at a later time through the Audio menu."));
            messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            messageBox.setDefaultButton(QMessageBox::No);
            messageBox.setWindowTitle("Audio Controls Editor");
            if (messageBox.exec() == QMessageBox::Yes)
            {
                RefreshAudioSystem();
            }
        }
        m_pATLModel->ClearDirtyFlags();
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::UpdateInspector()
    {
        m_pInspectorPanel->SetSelectedControls(m_pATLControlsPanel->GetSelectedControls());
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::UpdateFilterFromSelection()
    {
        bool bAllSameType = true;
        EACEControlType selectedType = eACET_NUM_TYPES;
        ControlList ids = m_pATLControlsPanel->GetSelectedControls();
        size_t size = ids.size();
        for (size_t i = 0; i < size; ++i)
        {
            CATLControl* pControl = m_pATLModel->GetControlByID(ids[i]);
            if (pControl)
            {
                if (selectedType == eACET_NUM_TYPES)
                {
                    selectedType = pControl->GetType();
                }
                else if (selectedType != pControl->GetType())
                {
                    bAllSameType = false;
                }
            }
        }

        bool bSelectedFolder = (selectedType == eACET_NUM_TYPES);
        for (int i = 0; i < eACET_NUM_TYPES; ++i)
        {
            EACEControlType type = (EACEControlType)i;
            bool bAllowed = bSelectedFolder || (bAllSameType && selectedType == type);
            m_pAudioSystemPanel->SetAllowedControls((EACEControlType)i, bAllowed);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::UpdateAudioSystemData()
    {
        auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get();
        IAudioSystemEditor* audioSystemImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
        if (!audioSystemImpl || !audioSystem)
        {
            return;
        }

        // clear the AudioSystem controls data
        Audio::SystemRequest::UnloadControls unloadControls;
        unloadControls.m_scope = Audio::eADS_ALL;
        audioSystem->PushRequest(AZStd::move(unloadControls));

        // parse the AudioSystem global config data
        // this is technically incorrect, we should just use GetControlsPath() unmodified when loading controls.
        // calling GetEditingGameDataFolder ensures that the reloaded file has been written to, a temp fix.
        // once we can listen to delete messages from Asset system, this can be changed to an EBus handler.
        const char* controlsPath = audioSystem->GetControlsPath();

        AZ::IO::FixedMaxPath controlsFolder{ controlsPath };

        // load the controls again
        Audio::SystemRequest::LoadControls loadGlobalControls;
        loadGlobalControls.m_scope = Audio::eADS_GLOBAL;
        audioSystem->PushRequest(AZStd::move(loadGlobalControls));

        // parse the AudioSystem level-specific config data
        AZStd::string levelName;
        AzToolsFramework::EditorRequestBus::BroadcastResult(levelName, &AzToolsFramework::EditorRequests::GetLevelName);
        if (!levelName.empty() && levelName != "Untitled")
        {
            controlsFolder /= "levels";
            controlsFolder /= levelName;

            Audio::SystemRequest::LoadControls loadLevelControls;
            loadLevelControls.m_scope = Audio::eADS_LEVEL_SPECIFIC;
            audioSystem->PushRequest(AZStd::move(loadLevelControls));
        }

        // inform the middleware specific plugin that the data has been saved
        // to disk (in case it needs to update something)
        audioSystemImpl->DataSaved();
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::OnEditorNotifyEvent(EEditorNotifyEvent event)
    {
        if (event == eNotify_OnEndSceneSave)
        {
            CAudioControlsEditorPlugin::ReloadScopes();
            m_pInspectorPanel->Reload();
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::FilterControlType(EACEControlType type, bool bShow)
    {
        m_pAudioSystemPanel->SetAllowedControls(type, bShow);
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioControlsEditorWindow::ReloadMiddlewareData()
    {
        IAudioSystemEditor* pAudioSystemImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
        if (pAudioSystemImpl)
        {
            pAudioSystemImpl->Reload();
        }
        m_pAudioSystemPanel->Reload();
        m_pInspectorPanel->Reload();
    }
} // namespace AudioControls

#include <Source/Editor/moc_AudioControlsEditorWindow.cpp>
