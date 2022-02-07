/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "CryEditDoc.h"

// Qt
#include <QDateTime>
#include <QDialogButtonBox>

// AzCore
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Utils/Utils.h>
#include <MathConversion.h>

// AzFramework
#include <AzFramework/Archive/IArchive.h>
#include <AzFramework/API/ApplicationAPI.h>

// AzToolsFramework
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/UI/Layer/NameConflictWarning.hxx>
#include <AzToolsFramework/API/EditorLevelNotificationBus.h>

// Editor
#include "Settings.h"

#include "PluginManager.h"
#include "ViewManager.h"
#include "DisplaySettings.h"
#include "GameEngine.h"

#include "CryEdit.h"
#include "ActionManager.h"
#include "Include/IObjectManager.h"
#include "ErrorReportDialog.h"
#include "SurfaceTypeValidator.h"
#include "Util/AutoLogTime.h"
#include "CheckOutDialog.h"
#include "GameExporter.h"
#include "MainWindow.h"
#include "LevelFileDialog.h"
#include "StatObjBus.h"
#include "Undo/Undo.h"

#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

// LmbrCentral
#include <LmbrCentral/Audio/AudioSystemComponentBus.h>
#include <LmbrCentral/Rendering/EditorLightComponentBus.h> // for LmbrCentral::EditorLightComponentRequestBus

//#define PROFILE_LOADING_WITH_VTUNE

// profilers api.
//#include "pure.h"
#ifdef PROFILE_LOADING_WITH_VTUNE
#include "C:\Program Files\Intel\Vtune\Analyzer\Include\VTuneApi.h"
#pragma comment(lib,"C:\\Program Files\\Intel\\Vtune\\Analyzer\\Lib\\VTuneApi.lib")
#endif

static const char* kAutoBackupFolder = "_autobackup";
static const char* kHoldFolder = "$tmp_hold"; // conform to the ignored file types $tmp[0-9]*_ regex
static const char* kSaveBackupFolder = "_savebackup";
static const char* kResizeTempFolder = "$tmp_resize"; // conform to the ignored file types $tmp[0-9]*_ regex

static const char* kBackupOrTempFolders[] =
{
    kAutoBackupFolder,
    kHoldFolder,
    kSaveBackupFolder,
    kResizeTempFolder,
    "_hold", // legacy name
    "_tmpresize", // legacy name
};

static const char* kLevelPathForSliceEditing = "EngineAssets/LevelForSliceEditing/LevelForSliceEditing.ly";

static bool IsSliceFile(const QString& filePath)
{
    return filePath.endsWith(AzToolsFramework::SliceUtilities::GetSliceFileExtension().c_str(), Qt::CaseInsensitive);
}

namespace Internal
{
    bool SaveLevel()
    {
        if (!GetIEditor()->GetDocument()->DoSave(GetIEditor()->GetDocument()->GetActivePathName(), true))
        {
            return false;
        }

        return true;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CCryEditDoc construction/destruction

CCryEditDoc::CCryEditDoc()
    : doc_validate_surface_types(nullptr)
    , m_modifiedModuleFlags(eModifiedNothing)
{
    ////////////////////////////////////////////////////////////////////////
    // Set member variables to initial values
    ////////////////////////////////////////////////////////////////////////

    m_fogTemplate = GetIEditor()->FindTemplate("Fog");
    m_environmentTemplate = GetIEditor()->FindTemplate("Environment");

    if (m_environmentTemplate)
    {
        m_fogTemplate = m_environmentTemplate->findChild("Fog");
    }
    else
    {
        m_environmentTemplate = XmlHelpers::CreateXmlNode("Environment");
    }

    GetIEditor()->SetDocument(this);
    CLogFile::WriteLine("Document created");
    RegisterConsoleVariables();

    MainWindow::instance()->GetActionManager()->RegisterActionHandler(ID_FILE_SAVE_AS, this, &CCryEditDoc::OnFileSaveAs);
    bool isPrefabSystemEnabled = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(isPrefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);
    if (isPrefabSystemEnabled)
    {
        m_prefabSystemComponentInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();
        AZ_Assert(m_prefabSystemComponentInterface, "PrefabSystemComponentInterface is not found.");
        m_prefabEditorEntityOwnershipInterface = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
        AZ_Assert(m_prefabEditorEntityOwnershipInterface, "PrefabEditorEntityOwnershipInterface is not found.");
        m_prefabLoaderInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get();
        AZ_Assert(m_prefabLoaderInterface, "PrefabLoaderInterface is not found.");
        m_prefabIntegrationInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabIntegrationInterface>::Get();
        AZ_Assert(m_prefabIntegrationInterface, "PrefabIntegrationInterface is not found.");
    }
}

CCryEditDoc::~CCryEditDoc()
{
    GetIEditor()->SetDocument(nullptr);

    CLogFile::WriteLine("Document destroyed");

    AzToolsFramework::SliceEditorEntityOwnershipServiceNotificationBus::Handler::BusDisconnect();
}

bool CCryEditDoc::IsModified() const
{
    return m_modified;
}

void CCryEditDoc::SetModifiedFlag(bool modified)
{
    m_modified = modified;
}

QString CCryEditDoc::GetLevelPathName() const
{
    return m_pathName;
}

void CCryEditDoc::SetPathName(const QString& pathName)
{
    if (IsSliceFile(pathName))
    {
        m_pathName = kLevelPathForSliceEditing;
        m_slicePathName = pathName;
    }
    else
    {
        m_pathName = pathName;
        m_slicePathName.clear();
    }
    SetTitle(pathName.isEmpty() ? tr("Untitled") : PathUtil::GetFileName(pathName.toUtf8().data()).c_str());
}

QString CCryEditDoc::GetSlicePathName() const
{
    return m_slicePathName;
}

CCryEditDoc::DocumentEditingMode CCryEditDoc::GetEditMode() const
{
    return m_slicePathName.isEmpty() ? CCryEditDoc::DocumentEditingMode::LevelEdit : CCryEditDoc::DocumentEditingMode::SliceEdit;
}

QString CCryEditDoc::GetActivePathName() const
{
    return GetEditMode() == CCryEditDoc::DocumentEditingMode::SliceEdit ? GetSlicePathName() : GetLevelPathName();
}

QString CCryEditDoc::GetTitle() const
{
    return m_title;
}

void CCryEditDoc::SetTitle(const QString& title)
{
    m_title = title;
}

bool CCryEditDoc::IsBackupOrTempLevelSubdirectory(const QString& folderName)
{
    for (const char* backupOrTempFolderName : kBackupOrTempFolders)
    {
        if (!folderName.compare(backupOrTempFolderName, Qt::CaseInsensitive))
        {
            return true;
        }
    }

    return false;
}

bool CCryEditDoc::DoSave(const QString& pathName, bool replace)
{
    if (!OnSaveDocument(pathName.isEmpty() ? GetActivePathName() : pathName))
    {
        return false;
    }

    if (replace)
    {
        SetPathName(pathName);
    }

    return true;
}

bool CCryEditDoc::Save()
{
    return OnSaveDocument(GetActivePathName());
}

void CCryEditDoc::DeleteContents()
{
    m_hasErrors = false;
    SetDocumentReady(false);

    GetIEditor()->Notify(eNotify_OnCloseScene);
    CrySystemEventBus::Broadcast(&CrySystemEventBus::Events::OnCryEditorCloseScene);

    EBUS_EVENT(AzToolsFramework::EditorEntityContextRequestBus, ResetEditorContext);

    // [LY-90904] move this to the EditorVegetationManager component
    InstanceStatObjEventBus::Broadcast(&InstanceStatObjEventBus::Events::ReleaseData);

    //////////////////////////////////////////////////////////////////////////
    // Clear all undo info.
    //////////////////////////////////////////////////////////////////////////
    GetIEditor()->FlushUndo();

    // Notify listeners.
    for (IDocListener* listener : m_listeners)
    {
        listener->OnCloseDocument();
    }

    GetIEditor()->ResetViews();

    // Delete all objects from Object Manager.
    GetIEditor()->GetObjectManager()->DeleteAllObjects();

    // Load scripts data
    SetModifiedFlag(false);
    SetModifiedModules(eModifiedNothing);
    // Clear error reports if open.
    CErrorReportDialog::Clear();

    // Unload level specific audio binary data.
    LmbrCentral::AudioSystemComponentRequestBus::Broadcast(&LmbrCentral::AudioSystemComponentRequestBus::Events::LevelUnloadAudio);

    GetIEditor()->Notify(eNotify_OnSceneClosed);
    CrySystemEventBus::Broadcast(&CrySystemEventBus::Events::OnCryEditorSceneClosed);
}


void CCryEditDoc::Save(CXmlArchive& xmlAr)
{
    TDocMultiArchive arrXmlAr;
    FillXmlArArray(arrXmlAr, &xmlAr);
    Save(arrXmlAr);
}

void CCryEditDoc::Save(TDocMultiArchive& arrXmlAr)
{
    bool isPrefabEnabled = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(isPrefabEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

    if (!isPrefabEnabled)
    {
        CAutoDocNotReady autoDocNotReady;

        if (arrXmlAr[DMAS_GENERAL] != nullptr)
        {
            (*arrXmlAr[DMAS_GENERAL]).root = XmlHelpers::CreateXmlNode("Level");
            (*arrXmlAr[DMAS_GENERAL]).root->setAttr("WaterColor", m_waterColor);

            char version[50];
            GetIEditor()->GetFileVersion().ToString(version, AZ_ARRAY_SIZE(version));
            (*arrXmlAr[DMAS_GENERAL]).root->setAttr("SandboxVersion", version);

            SerializeViewSettings((*arrXmlAr[DMAS_GENERAL]));

            // Fog settings  ///////////////////////////////////////////////////////
            SerializeFogSettings((*arrXmlAr[DMAS_GENERAL]));

            SerializeNameSelection((*arrXmlAr[DMAS_GENERAL]));
        }
    }
    AfterSave();
}


void CCryEditDoc::Load(CXmlArchive& xmlAr, const QString& szFilename)
{
    TDocMultiArchive arrXmlAr;
    FillXmlArArray(arrXmlAr, &xmlAr);
    CCryEditDoc::Load(arrXmlAr, szFilename);
}

//////////////////////////////////////////////////////////////////////////
void CCryEditDoc::Load(TDocMultiArchive& arrXmlAr, const QString& szFilename)
{
    bool isPrefabEnabled = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(isPrefabEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

    m_hasErrors = false;

    // Register a unique load event
    QString fileName = Path::GetFileName(szFilename);
    QString levelHash;
    if (!isPrefabEnabled)
    {
        levelHash = GetIEditor()->GetSettingsManager()->GenerateContentHash(arrXmlAr[DMAS_GENERAL]->root, fileName);
    }
    else
    {
        levelHash = szFilename;
    }
    SEventLog loadEvent("Level_" + Path::GetFileName(fileName), "", levelHash);

    // Register this level and its content hash as version
    GetIEditor()->GetSettingsManager()->AddToolVersion(fileName, levelHash);
    GetIEditor()->GetSettingsManager()->RegisterEvent(loadEvent);

    CAutoDocNotReady autoDocNotReady;

    HEAP_CHECK

    CLogFile::FormatLine("Loading from %s...", szFilename.toUtf8().data());
    QString szLevelPath = Path::GetPath(szFilename);

    {
        // Set game g_levelname variable to the name of current level.
        QString szGameLevelName = Path::GetFileName(szFilename);
        ICVar* sv_map = gEnv->pConsole->GetCVar("sv_map");
        if (sv_map)
        {
            sv_map->Set(szGameLevelName.toUtf8().data());
        }
    }

    // Starts recording the opening of files using the level category
    if (auto archive = AZ::Interface<AZ::IO::IArchive>::Get(); archive && archive->GetRecordFileOpenList() == AZ::IO::IArchive::RFOM_EngineStartup)
    {
        archive->RecordFileOpen(AZ::IO::IArchive::RFOM_Level);
    }

    GetIEditor()->Notify(eNotify_OnBeginSceneOpen);
    GetIEditor()->GetMovieSystem()->RemoveAllSequences();

    {
        // Start recording errors
        const ICVar* pShowErrorDialogOnLoad = gEnv->pConsole->GetCVar("ed_showErrorDialogOnLoad");
        CErrorsRecorder errorsRecorder(pShowErrorDialogOnLoad && (pShowErrorDialogOnLoad->GetIVal() != 0));

        bool usePrefabSystemForLevels = false;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(
            usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);

        if (!usePrefabSystemForLevels)
        {
            AZStd::string levelPakPath;
            if (AzFramework::StringFunc::Path::ConstructFull(szLevelPath.toUtf8().data(), "level", "pak", levelPakPath, true))
            {
                // Check whether level.pak is present
                if (!gEnv->pFileIO->Exists(levelPakPath.c_str()))
                {
                    CryWarning(
                        VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING,
                        "level.pak is missing.  This will cause other errors.  To fix this, re-export the level.");
                }
            }
        }

        int t0 = GetTickCount();

#ifdef PROFILE_LOADING_WITH_VTUNE
        VTResume();
#endif
        // Load level-specific audio data.
        AZStd::string levelFileName{ fileName.toUtf8().constData() };
        AZStd::to_lower(levelFileName.begin(), levelFileName.end());
        LmbrCentral::AudioSystemComponentRequestBus::Broadcast(
            &LmbrCentral::AudioSystemComponentRequestBus::Events::LevelLoadAudio, AZStd::string_view{ levelFileName });

        {
            CAutoLogTime logtime("Game Engine level load");
            GetIEditor()->GetGameEngine()->LoadLevel(true, true);
        }

        if (!isPrefabEnabled)
        {
            //////////////////////////////////////////////////////////////////////////
            // Load water color.
            //////////////////////////////////////////////////////////////////////////
            (*arrXmlAr[DMAS_GENERAL]).root->getAttr("WaterColor", m_waterColor);

            //////////////////////////////////////////////////////////////////////////
            // Load View Settings
            //////////////////////////////////////////////////////////////////////////
            SerializeViewSettings((*arrXmlAr[DMAS_GENERAL]));

            //////////////////////////////////////////////////////////////////////////
            // Fog settings
            //////////////////////////////////////////////////////////////////////////
            SerializeFogSettings((*arrXmlAr[DMAS_GENERAL]));
        }

        if (!isPrefabEnabled)
        {
            // Serialize Shader Cache.
            CAutoLogTime logtime("Load Level Shader Cache");
        }

        {
            // support old version of sequences
            IMovieSystem* pMs = GetIEditor()->GetMovieSystem();

            if (pMs)
            {
                for (int k = 0; k < pMs->GetNumSequences(); ++k)
                {
                    IAnimSequence* seq = pMs->GetSequence(k);
                    QString fullname = seq->GetName();
                    CBaseObject* pObj = GetIEditor()->GetObjectManager()->FindObject(fullname);

                    if (!pObj)
                    {
                        pObj = GetIEditor()->GetObjectManager()->NewObject("SequenceObject", nullptr, fullname);
                    }
                }
            }
        }

        if (!isPrefabEnabled)
        {
            // Name Selection groups
            SerializeNameSelection((*arrXmlAr[DMAS_GENERAL]));
        }

        {
            CAutoLogTime logtime("Post Load");

            // Notify listeners.
            for (IDocListener* listener : m_listeners)
            {
                listener->OnLoadDocument();
            }
        }

        CSurfaceTypeValidator().Validate();

#ifdef PROFILE_LOADING_WITH_VTUNE
        VTPause();
#endif

        LogLoadTime(GetTickCount() - t0);
        // Loaded with success, remove event from log file
        GetIEditor()->GetSettingsManager()->UnregisterEvent(loadEvent);
    }

    GetIEditor()->Notify(eNotify_OnEndSceneOpen);
}

void CCryEditDoc::AfterSave()
{
    // When saving level also save editor settings
    // Save settings
    gSettings.Save();
    GetIEditor()->GetDisplaySettings()->SaveRegistry();
    MainWindow::instance()->SaveConfig();
}

void CCryEditDoc::SerializeViewSettings(CXmlArchive& xmlAr)
{
    // Load or restore the viewer settings from an XML
    if (xmlAr.bLoading)
    {
        bool useOldViewFormat = false;
        // Loading
        CLogFile::WriteLine("Loading View settings...");

        int numberOfGameViewports = GetIEditor()->GetViewManager()->GetNumberOfGameViewports();

        for (int i = 0; i < numberOfGameViewports; i++)
        {
            XmlNodeRef view;
            Vec3 vp(0.0f, 0.0f, 256.0f);
            Ang3 va(ZERO);

            auto viewName = QString("View%1").arg(i);
            view = xmlAr.root->findChild(viewName.toUtf8().constData());

            if (!view)
            {
                view = xmlAr.root->findChild("View");
                if (view)
                {
                    useOldViewFormat = true;
                }
            }

            if (view)
            {
                auto viewerPosName = QString("ViewerPos%1").arg(useOldViewFormat ? "" : QString::number(i));
                view->getAttr(viewerPosName.toUtf8().constData(), vp);
                auto viewerAnglesName = QString("ViewerAngles%1").arg(useOldViewFormat ? "" : QString::number(i));
                view->getAttr(viewerAnglesName.toUtf8().constData(), va);
            }

            Matrix34 tm = Matrix34::CreateRotationXYZ(va);
            tm.SetTranslation(vp);

            auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
            if (auto viewportContext = viewportContextManager->GetViewportContextById(i))
            {
                viewportContext->SetCameraTransform(LYTransformToAZTransform(tm));
            }
        }
    }
    else
    {
        // Storing
        CLogFile::WriteLine("Storing View settings...");

        int numberOfGameViewports = GetIEditor()->GetViewManager()->GetNumberOfGameViewports();

        for (int i = 0; i < numberOfGameViewports; i++)
        {
            auto viewName = QString("View%1").arg(i);
            XmlNodeRef view = xmlAr.root->newChild(viewName.toUtf8().constData());

            CViewport* pVP = GetIEditor()->GetViewManager()->GetView(i);

            if (pVP)
            {
                Vec3 pos = pVP->GetViewTM().GetTranslation();
                Ang3 angles = Ang3::GetAnglesXYZ(Matrix33(pVP->GetViewTM()));
                auto viewerPosName = QString("ViewerPos%1").arg(i);
                view->setAttr(viewerPosName.toUtf8().constData(), pos);
                auto viewerAnglesName = QString("ViewerAngles%1").arg(i);
                view->setAttr(viewerAnglesName.toUtf8().constData(), angles);
            }
        }
    }
}

void CCryEditDoc::SerializeFogSettings(CXmlArchive& xmlAr)
{
    if (xmlAr.bLoading)
    {
        CLogFile::WriteLine("Loading Fog settings...");

        XmlNodeRef fog = xmlAr.root->findChild("Fog");

        if (!fog)
        {
            return;
        }

        if (m_fogTemplate)
        {
            CXmlTemplate::GetValues(m_fogTemplate, fog);
        }
    }
    else
    {
        CLogFile::WriteLine("Storing Fog settings...");

        XmlNodeRef fog = xmlAr.root->newChild("Fog");

        if (m_fogTemplate)
        {
            CXmlTemplate::SetValues(m_fogTemplate, fog);
        }
    }
}

void CCryEditDoc::SerializeNameSelection(CXmlArchive& xmlAr)
{
    IObjectManager* pObjManager = GetIEditor()->GetObjectManager();

    if (pObjManager)
    {
        pObjManager->SerializeNameSelection(xmlAr.root, xmlAr.bLoading);
    }
}

void CCryEditDoc::SetModifiedModules(EModifiedModule eModifiedModule, bool boSet)
{
    if (!boSet)
    {
        m_modifiedModuleFlags &= ~eModifiedModule;
    }
    else
    {
        if (eModifiedModule == eModifiedNothing)
        {
            m_modifiedModuleFlags = eModifiedNothing;
        }
        else
        {
            m_modifiedModuleFlags |= eModifiedModule;
        }
    }
}

int CCryEditDoc::GetModifiedModule()
{
    return m_modifiedModuleFlags;
}

bool CCryEditDoc::CanCloseFrame()
{
    // Ask the base class to ask for saving, which also includes the save
    // status of the plugins. Additionaly we query if all the plugins can exit
    // now. A reason for a failure might be that one of the plugins isn't
    // currently processing data or has other unsaved information which
    // are not serialized in the project file
    if (!SaveModified())
    {
        return false;
    }

    if (!GetIEditor()->GetPluginManager()->CanAllPluginsExitNow())
    {
        return false;
    }

    // If there is an export in process, exiting will corrupt it
    if (CGameExporter::GetCurrentExporter() != nullptr)
    {
        return false;
    }

    return true;
}

bool CCryEditDoc::SaveModified()
{
    if (!IsModified())
    {
        return true;
    }

    bool usePrefabSystemForLevels = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);
    if (!usePrefabSystemForLevels)
    {
        QMessageBox saveModifiedMessageBox(AzToolsFramework::GetActiveWindow());
        saveModifiedMessageBox.setText(QString("Save changes to %1?").arg(GetTitle()));
        saveModifiedMessageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        saveModifiedMessageBox.setIcon(QMessageBox::Icon::Question);

        auto button = QMessageBox::question(
            AzToolsFramework::GetActiveWindow(), QString(), tr("Save changes to %1?").arg(GetTitle()),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        switch (button)
        {
        case QMessageBox::Cancel:
            return false;
        case QMessageBox::Yes:
            return DoFileSave();
        case QMessageBox::No:
            SetModifiedFlag(false);
            return true;
        }
        Q_UNREACHABLE();
    }
    else
    {
        AzToolsFramework::Prefab::TemplateId rootPrefabTemplateId = m_prefabEditorEntityOwnershipInterface->GetRootPrefabTemplateId();
        if (!m_prefabSystemComponentInterface->AreDirtyTemplatesPresent(rootPrefabTemplateId))
        {
            return true;
        }

        int prefabSaveSelection = m_prefabIntegrationInterface->ExecuteClosePrefabDialog(rootPrefabTemplateId);

        // In order to get the accept and reject codes of QDialog and QDialogButtonBox aligned, we do (1-prefabSaveSelection) here.
        // For example, QDialog::Rejected(0) is emitted when dialog is closed. But the int value corresponds to
        // QDialogButtonBox::AcceptRole(0).
        switch (1 - prefabSaveSelection)
        {
        case QDialogButtonBox::AcceptRole:
            return true;
        case QDialogButtonBox::RejectRole:
            return false;
        case QDialogButtonBox::InvalidRole:
            SetModifiedFlag(false);
            return true;
        }
        Q_UNREACHABLE();
    }
}

void CCryEditDoc::OnFileSaveAs()
{
    CLevelFileDialog levelFileDialog(false);
    levelFileDialog.show();
    levelFileDialog.adjustSize();

    if (levelFileDialog.exec() == QDialog::Accepted)
    {
        if (OnSaveDocument(levelFileDialog.GetFileName()))
        {
            CCryEditApp::instance()->AddToRecentFileList(levelFileDialog.GetFileName());
            bool usePrefabSystemForLevels = false;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(
                usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);
            if (usePrefabSystemForLevels)
            {
                AzToolsFramework::Prefab::TemplateId rootPrefabTemplateId =
                    m_prefabEditorEntityOwnershipInterface->GetRootPrefabTemplateId();
                SetModifiedFlag(m_prefabSystemComponentInterface->AreDirtyTemplatesPresent(rootPrefabTemplateId));
            }
        }
    }
}

bool CCryEditDoc::OnOpenDocument(const QString& lpszPathName)
{
    TOpenDocContext context;
    if (!BeforeOpenDocument(lpszPathName, context))
    {
        return false;
    }
    return DoOpenDocument(context);
}

bool CCryEditDoc::BeforeOpenDocument(const QString& lpszPathName, TOpenDocContext& context)
{
    CTimeValue loading_start_time = gEnv->pTimer->GetAsyncTime();

    bool usePrefabSystemForLevels = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);

    if (!usePrefabSystemForLevels)
    {
        // ensure we close any open packs
        if (!GetIEditor()->GetLevelFolder().isEmpty())
        {
            GetIEditor()->GetSystem()->GetIPak()->ClosePack((GetIEditor()->GetLevelFolder() + "\\level.pak").toUtf8().data());
        }
    }

    // restore directory to root.
    QDir::setCurrent(GetIEditor()->GetPrimaryCDFolder());

    QString absolutePath = lpszPathName;
    QFileInfo fileInfo(absolutePath);
    QString friendlyDisplayName = Path::GetRelativePath(absolutePath, true);
    CLogFile::FormatLine("Opening level %s", friendlyDisplayName.toUtf8().data());

    // normalize the file path.
    absolutePath = Path::ToUnixPath(QFileInfo(absolutePath).canonicalFilePath());
    context.loading_start_time = loading_start_time;
    if (IsSliceFile(absolutePath))
    {
        context.absoluteLevelPath = Path::GamePathToFullPath(kLevelPathForSliceEditing);
        context.absoluteSlicePath = absolutePath;
    }
    else
    {
        context.absoluteLevelPath = absolutePath;
        context.absoluteSlicePath = "";
    }
    return true;
}

bool CCryEditDoc::DoOpenDocument(TOpenDocContext& context)
{
    CTimeValue& loading_start_time = context.loading_start_time;

    bool isPrefabEnabled = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(isPrefabEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

    // normalize the path so that its the same in all following calls:
    QString levelFilePath = QFileInfo(context.absoluteLevelPath).absoluteFilePath();
    context.absoluteLevelPath = levelFilePath;

    m_bLoadFailed = false;

    auto pIPak = GetIEditor()->GetSystem()->GetIPak();

    QString levelFolderAbsolutePath = QFileInfo(context.absoluteLevelPath).absolutePath();


    if (!isPrefabEnabled)
    {
        // if the level pack exists, open that, too:
        QString levelPackFileAbsolutePath = QDir(levelFolderAbsolutePath).absoluteFilePath("level.pak");

        // we mount the pack (level.pak) using the folder its sitting in as the mountpoint (first parameter)
        pIPak->OpenPack(levelFolderAbsolutePath.toUtf8().constData(), levelPackFileAbsolutePath.toUtf8().constData());
    }

    TDocMultiArchive arrXmlAr = {};

    if (!isPrefabEnabled)
    {
        if (!LoadXmlArchiveArray(arrXmlAr, levelFilePath, levelFolderAbsolutePath))
        {
            m_bLoadFailed = true;
            return false;
        }
    }
    if (!LoadLevel(arrXmlAr, context.absoluteLevelPath))
    {
        m_bLoadFailed = true;
    }

    ReleaseXmlArchiveArray(arrXmlAr);

    if (m_bLoadFailed)
    {
        return false;
    }

    // Load AZ entities for the editor.
    if (context.absoluteSlicePath.isEmpty())
    {
        if (!LoadEntitiesFromLevel(context.absoluteLevelPath))
        {
            m_bLoadFailed = true;
        }
    }
    else
    {
        if (!LoadEntitiesFromSlice(context.absoluteSlicePath))
        {
            m_bLoadFailed = true;
        }
    }

    if (m_bLoadFailed)
    {
        return false;
    }

    StartStreamingLoad();

    CTimeValue loading_end_time = gEnv->pTimer->GetAsyncTime();

    CLogFile::FormatLine("-----------------------------------------------------------");
    CLogFile::FormatLine("Successfully opened document %s", context.absoluteLevelPath.toUtf8().data());
    CLogFile::FormatLine("Level loading time: %.2f seconds", (loading_end_time - loading_start_time).GetSeconds());
    CLogFile::FormatLine("-----------------------------------------------------------");

    // It assumes loaded levels have already been exported. Can be a big fat lie, though.
    // The right way would require us to save to the level folder the export status of the
    // level.
    SetLevelExported(true);

    return true;
}

bool CCryEditDoc::OnNewDocument()
{
    DeleteContents();
    m_pathName.clear();
    m_slicePathName.clear();
    SetModifiedFlag(false);
    return true;
}

bool CCryEditDoc::OnSaveDocument(const QString& lpszPathName)
{
    bool saveSuccess = false;
    bool shouldSaveLevel = true;
    if (gEnv->IsEditorSimulationMode())
    {
        // Don't allow saving in AI/Physics mode.
        // Prompt the user to exit Simulation Mode (aka AI/Phyics mode) before saving.
        QWidget* mainWindow = nullptr;
        EBUS_EVENT_RESULT(mainWindow, AzToolsFramework::EditorRequests::Bus, GetMainWindow);

        QMessageBox msgBox(mainWindow);
        msgBox.setText(tr("You must exit AI/Physics mode before saving."));
        msgBox.setInformativeText(tr("The level will not be saved."));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
    }
    else
    {
        if (m_hasErrors || m_bLoadFailed)
        {
            QWidget* mainWindow = nullptr;
            AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                mainWindow,
                &AzToolsFramework::EditorRequests::Bus::Events::GetMainWindow);

            // Prompt the user that saving may result in data loss. Most of the time this is not desired
            // (which is why 'cancel' is the default interaction), but this does provide users a way to still
            // save their level if this is the only way they can solve the erroneous data.
            QMessageBox msgBox(mainWindow);
            msgBox.setText(tr("Your level loaded with errors, you may lose work if you save."));
            msgBox.setInformativeText(tr("Do you want to save your changes?"));
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            int result = msgBox.exec();
            switch (result)
            {
            case QMessageBox::Save:
                // The user wishes to save, so don't bail.
                break;
            case QMessageBox::Cancel:
                // The user is canceling the save operation, so stop any saving from occuring.
                shouldSaveLevel = false;
                break;
            }
        }

        TSaveDocContext context;
        if (shouldSaveLevel && BeforeSaveDocument(lpszPathName, context))
        {
            DoSaveDocument(lpszPathName, context);
            saveSuccess = AfterSaveDocument(lpszPathName, context);
        }
    }

    return saveSuccess;
}

bool CCryEditDoc::BeforeSaveDocument(const QString& lpszPathName, TSaveDocContext& context)
{
    // Don't save level data if any conflict exists
    if (HasLayerNameConflicts())
    {
        return false;
    }

    // Restore directory to root.
    QDir::setCurrent(GetIEditor()->GetPrimaryCDFolder());

    // If we do not have a level loaded, we will also have an empty path, and that will
    // cause problems later in the save process.  Early out here if that's the case
    QString levelFriendlyName = QFileInfo(lpszPathName).fileName();
    if (levelFriendlyName.isEmpty())
    {
        return false;
    }

    CryLog("Saving to %s...", levelFriendlyName.toUtf8().data());
    GetIEditor()->Notify(eNotify_OnBeginSceneSave);

    bool bSaved(true);

    context.bSaved = bSaved;
    return true;
}

bool CCryEditDoc::HasLayerNameConflicts() const
{
    AZStd::vector<AZ::Entity*> editorEntities;
    AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
        &AzToolsFramework::EditorEntityContextRequestBus::Events::GetLooseEditorEntities,
        editorEntities);

    AZStd::unordered_map<AZStd::string, int> nameConflictMapping;
    for (AZ::Entity* entity : editorEntities)
    {
        AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
            entity->GetId(),
            &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::UpdateLayerNameConflictMapping,
            nameConflictMapping);
    }

    if (!nameConflictMapping.empty())
    {
        AzToolsFramework::Layers::NameConflictWarning* nameConflictWarning = new AzToolsFramework::Layers::NameConflictWarning(
            MainWindow::instance(),
            nameConflictMapping);
        nameConflictWarning->exec();

        return true;
    }

    return false;
}

bool CCryEditDoc::DoSaveDocument(const QString& filename, TSaveDocContext& context)
{
    bool& bSaved = context.bSaved;
    if (!bSaved)
    {
        return false;
    }
    // Paranoia - we shouldn't get this far into the save routine without a level loaded (empty levelPath)
    // If nothing is loaded, we don't need to save anything
    if (filename.isEmpty())
    {
        bSaved = false;
        return false;
    }

    // Save Tag Point locations to file if auto save of tag points disabled
    if (!gSettings.bAutoSaveTagPoints)
    {
        CCryEditApp::instance()->SaveTagLocations();
    }

    QString normalizedPath = Path::ToUnixPath(filename);
    if (IsSliceFile(normalizedPath))
    {
        bSaved = SaveSlice(normalizedPath);
    }
    else
    {
        bSaved = SaveLevel(normalizedPath);
    }

    // Changes filename for this document.
    SetPathName(normalizedPath);
    return bSaved;
}

bool CCryEditDoc::AfterSaveDocument([[maybe_unused]] const QString& lpszPathName, TSaveDocContext& context, bool bShowPrompt)
{
    bool bSaved = context.bSaved;

    GetIEditor()->Notify(eNotify_OnEndSceneSave);

    if (!bSaved)
    {
        if (bShowPrompt)
        {
            QMessageBox::warning(QApplication::activeWindow(), QString(), QObject::tr("Save Failed"), QMessageBox::Ok);
        }
        CLogFile::WriteLine("$4Document saving has failed.");
    }
    else
    {
        CLogFile::WriteLine("$3Document successfully saved");
        SetModifiedFlag(false);
        SetModifiedModules(eModifiedNothing);
        MainWindow::instance()->ResetAutoSaveTimers();
    }

    return bSaved;
}

static bool TryRenameFile(const QString& oldPath, const QString& newPath, int retryAttempts=10)
{
    QFile(newPath).setPermissions(QFile::ReadOther | QFile::WriteOther);
    QFile::remove(newPath);

    // try a few times, something can lock the file (such as virus scanner, etc).
    for (int attempts = 0; attempts < retryAttempts; ++attempts)
    {
        if (QFile::rename(oldPath, newPath))
        {
            return true;
        }

        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
    }

    return false;
}

bool CCryEditDoc::SaveLevel(const QString& filename)
{
    AZ_PROFILE_FUNCTION(Editor);
    QWaitCursor wait;

    CAutoCheckOutDialogEnableForAll enableForAll;

    QString fullPathName = Path::ToUnixPath(filename);
    QString originaLevelFilename = Path::GetFile(m_pathName);
    if (QFileInfo(filename).isRelative())
    {
        // Resolving the path through resolvepath would normalize and lowcase it, and in this case, we don't want that.
        fullPathName = Path::ToUnixPath(QDir(QString::fromUtf8(gEnv->pFileIO->GetAlias("@projectroot@"))).absoluteFilePath(fullPathName));
    }

    if (!CFileUtil::OverwriteFile(fullPathName))
    {
        return false;
    }

    {

        AZ_PROFILE_SCOPE(Editor, "CCryEditDoc::SaveLevel BackupBeforeSave");
        BackupBeforeSave();
    }

    // need to copy existing level data before saving to different folder
    const QString oldLevelFolder = Path::GetPath(GetLevelPathName()); // get just the folder name
    QString newLevelFolder = Path::GetPath(fullPathName);

    CFileUtil::CreateDirectory(newLevelFolder.toUtf8().data());
    GetIEditor()->GetGameEngine()->SetLevelPath(newLevelFolder);

    // QFileInfo operator== takes care of many side cases and will return true
    // if the folder is the same folder, even if other things (like slashes, etc) are wrong
    if (QFileInfo(oldLevelFolder) != QFileInfo(newLevelFolder))
    {
        // if we're saving to a new folder, we need to copy the old folder tree.
        auto pIPak = GetIEditor()->GetSystem()->GetIPak();

        const QString oldLevelPattern = QDir(oldLevelFolder).absoluteFilePath("*.*");
        const QString oldLevelName = Path::GetFile(GetLevelPathName());
        const QString oldLevelXml = Path::ReplaceExtension(oldLevelName, "xml");
        AZ::IO::ArchiveFileIterator findHandle = pIPak->FindFirst(oldLevelPattern.toUtf8().data(), AZ::IO::IArchive::eFileSearchType_AllowOnDiskAndInZips);
        if (findHandle)
        {
            do
            {
                const QString sourceName{ QString::fromUtf8(findHandle.m_filename.data(), aznumeric_cast<int>(findHandle.m_filename.size())) };
                if ((findHandle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory)
                {
                    // we only end up here if sourceName is a folder name.
                    bool skipDir = sourceName == "." || sourceName == "..";
                    skipDir |= IsBackupOrTempLevelSubdirectory(sourceName);
                    skipDir |= sourceName == "Layers"; // layers folder will be created and written out as part of saving
                    if (!skipDir)
                    {
                        QString oldFolderName = QDir(oldLevelFolder).absoluteFilePath(sourceName);
                        QString newFolderName = QDir(newLevelFolder).absoluteFilePath(sourceName);

                        CFileUtil::CreateDirectory(newFolderName.toUtf8().data());
                        CFileUtil::CopyTree(oldFolderName, newFolderName);
                    }
                    continue;
                }

                bool skipFile = sourceName.endsWith(".cry", Qt::CaseInsensitive) ||
                                sourceName.endsWith(".ly", Qt::CaseInsensitive) ||
                                sourceName == originaLevelFilename; // level file will be written out by saving, ignore the source one
                if (skipFile)
                {
                    continue;
                }

                // close any paks in the source folder so that when the paks are re-opened there is
                // no stale cached metadata in the pak system
                if (sourceName.endsWith(".pak", Qt::CaseInsensitive))
                {
                    QString oldPackName = QDir(oldLevelFolder).absoluteFilePath(sourceName);
                    pIPak->ClosePack(oldPackName.toUtf8().constData());
                }

                QString destName = sourceName;
                // copy oldLevel.xml -> newLevel.xml
                if (sourceName.compare(oldLevelXml, Qt::CaseInsensitive) == 0)
                {
                    destName = Path::ReplaceExtension(Path::GetFile(fullPathName), "xml");
                }

                QString oldFilePath = QDir(oldLevelFolder).absoluteFilePath(sourceName);
                QString newFilePath = QDir(newLevelFolder).absoluteFilePath(destName);
                CFileUtil::CopyFile(oldFilePath, newFilePath);
            } while ((findHandle = pIPak->FindNext(findHandle)));
            pIPak->FindClose(findHandle);
        }

        // ensure that copied files are not read-only
        CFileUtil::ForEach(newLevelFolder, [](const QString& filePath)
        {
            QFile(filePath).setPermissions(QFile::ReadOther | QFile::WriteOther);
        });

    }

    // Save level to XML archive.
    CXmlArchive xmlAr;
    Save(xmlAr);

    // temp files (to be ignored by AssetProcessor take the form $tmp[0-9]*_...).  we will conform
    // to that to make this file invisible to AP until it has been written completely.
    QString tempSaveFile = QDir(newLevelFolder).absoluteFilePath("$tmp_levelSave.tmp");
    QFile(tempSaveFile).setPermissions(QFile::ReadOther | QFile::WriteOther);
    QFile::remove(tempSaveFile);

    // Save AZ entities to the editor level.

    bool contentsAllSaved = false; // abort level save if anything within it fails

    auto tempFilenameStrData = tempSaveFile.toStdString();
    auto filenameStrData = fullPathName.toStdString();

    bool isPrefabEnabled = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(isPrefabEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

    if (!isPrefabEnabled)
    {
        AZStd::vector<char> entitySaveBuffer;
        bool savedEntities = false;
        CPakFile pakFile;

        {
            AZ_PROFILE_SCOPE(Editor, "CCryEditDoc::SaveLevel Open PakFile");
            if (!pakFile.Open(tempSaveFile.toUtf8().data(), false))
            {
                gEnv->pLog->LogWarning("Unable to open pack file %s for writing", tempSaveFile.toUtf8().data());
                return false;
            }
        }


        AZStd::vector<AZ::Entity*> editorEntities;
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::GetLooseEditorEntities,
            editorEntities);

        AZStd::vector<AZ::Entity*> layerEntities;
        AZ::SliceComponent::SliceReferenceToInstancePtrs instancesInLayers;
        for (AZ::Entity* entity : editorEntities)
        {
            AzToolsFramework::Layers::LayerResult layerSaveResult(AzToolsFramework::Layers::LayerResult::CreateSuccess());
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                layerSaveResult,
                entity->GetId(),
                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::WriteLayerAndGetEntities,
                newLevelFolder,
                layerEntities,
                instancesInLayers);
            layerSaveResult.MessageResult();
        }

        AZ::IO::ByteContainerStream<AZStd::vector<char>> entitySaveStream(&entitySaveBuffer);
        {
            AZ_PROFILE_SCOPE(Editor, "CCryEditDoc::SaveLevel Save Entities To Stream");
            EBUS_EVENT_RESULT(
                savedEntities, AzToolsFramework::EditorEntityContextRequestBus, SaveToStreamForEditor, entitySaveStream, layerEntities,
                instancesInLayers);
        }

        for (AZ::Entity* entity : editorEntities)
        {
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::Event(
                entity->GetId(), &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::RestoreEditorData);
        }

        if (savedEntities)
        {
            AZ_PROFILE_SCOPE(AzToolsFramework, "CCryEditDoc::SaveLevel Updated PakFile levelEntities.editor_xml");
            pakFile.UpdateFile("levelentities.editor_xml", entitySaveBuffer.begin(), static_cast<int>(entitySaveBuffer.size()));

            // Save XML archive to pak file.
            bool bSaved = xmlAr.SaveToPak(Path::GetPath(tempSaveFile), pakFile);
            if (bSaved)
            {
                contentsAllSaved = true;
            }
            else
            {
                gEnv->pLog->LogWarning("Unable to write the level data to file %s", tempSaveFile.toUtf8().data());
            }
        }
        else
        {
            gEnv->pLog->LogWarning("Unable to generate entity data for level save %s", tempSaveFile.toUtf8().data());
        }

        pakFile.Close();
    }
    else
    {
        if (m_prefabEditorEntityOwnershipInterface)
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "No File IO implementation available");

            AZ::IO::HandleType tempSaveFileHandle;
            AZ::IO::Result openResult = fileIO->Open(tempFilenameStrData.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, tempSaveFileHandle);
            contentsAllSaved = openResult;
            if (openResult)
            {
                AZ::IO::FileIOStream stream(tempSaveFileHandle, AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, false);
                contentsAllSaved = m_prefabEditorEntityOwnershipInterface->SaveToStream(stream, AZStd::string_view(filenameStrData.data(), filenameStrData.size()));
                stream.Close();
            }
        }
    }

    if (!contentsAllSaved)
    {
        AZ_Error("Editor", false, "Error when writing level '%s' into tmpfile '%s'", filenameStrData.c_str(), tempFilenameStrData.c_str());
        QFile::remove(tempSaveFile);
        return false;
    }

    if (!TryRenameFile(tempSaveFile, fullPathName))
    {
        gEnv->pLog->LogWarning("Unable to move file %s to %s when saving", tempSaveFile.toUtf8().data(), fullPathName.toUtf8().data());
        return false;
    }

    // Commit changes to the disk.
    _flushall();

    AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::OnSaveLevel);

    return true;
}

bool CCryEditDoc::SaveSlice(const QString& filename)
{
    using namespace AzToolsFramework::SliceUtilities;

    // Gather entities from live slice in memory
    AZ::SliceComponent* liveSlice = nullptr;
    AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(liveSlice,
        &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
    if (!liveSlice)
    {
        gEnv->pLog->LogWarning("Slice data not found.");
        return false;
    }

    AZStd::unordered_set<AZ::EntityId> liveEntityIds;
    if (!liveSlice->GetEntityIds(liveEntityIds))
    {
        gEnv->pLog->LogWarning("Error getting entities from slice.");
        return false;
    }


    // Prevent save when there are multiple root entities.
    bool foundRootEntity = false;
    for (AZ::EntityId entityId : liveEntityIds)
    {
        AZ::EntityId parentId;
        AZ::TransformBus::EventResult(parentId, entityId, &AZ::TransformBus::Events::GetParentId);
        if (!parentId.IsValid())
        {
            if (foundRootEntity)
            {
                gEnv->pLog->LogWarning("Cannot save a slice with multiple root entities.");
                return false;
            }

            foundRootEntity = true;
        }
    }

    // Find target slice asset, and check if it's the same asset we opened
    AZ::Data::AssetId targetAssetId;
    AZ::Data::AssetCatalogRequestBus::BroadcastResult(targetAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, filename.toUtf8().data(), azrtti_typeid<AZ::SliceAsset>(), false);

    QString openedFilepath = Path::ToUnixPath(Path::GetRelativePath(m_slicePathName, true));
    AZ::Data::AssetId openedAssetId;
    AZ::Data::AssetCatalogRequestBus::BroadcastResult(openedAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, openedFilepath.toUtf8().data(), azrtti_typeid<AZ::SliceAsset>(), false);

    if (!targetAssetId.IsValid() || openedAssetId != targetAssetId)
    {
        gEnv->pLog->LogWarning("Slice editor can only modify existing slices. 'New Slice' and 'Save As' are not currently supported.");
        return false;
    }

    AZ::Data::Asset<AZ::SliceAsset> sliceAssetRef = AZ::Data::AssetManager::Instance().GetAsset<AZ::SliceAsset>(targetAssetId, AZ::Data::AssetLoadBehavior::Default);

    sliceAssetRef.BlockUntilLoadComplete();

    if (!sliceAssetRef)
    {
        gEnv->pLog->LogWarning("Error loading slice: %s", filename.toUtf8().data());
        return false;
    }

    // Get entities from target slice asset.
    AZ::SliceComponent* assetSlice = sliceAssetRef.Get()->GetComponent();
    if (!assetSlice)
    {
        gEnv->pLog->LogWarning("Error reading slice: %s", filename.toUtf8().data());
        return false;
    }

    AZStd::unordered_set<AZ::EntityId> assetEntityIds;
    if (!assetSlice->GetEntityIds(assetEntityIds))
    {
        gEnv->pLog->LogWarning("Error getting entities from slice: %s", filename.toUtf8().data());
        return false;
    }

    AZStd::unordered_set<AZ::EntityId> entityAdds;
    AZStd::unordered_set<AZ::EntityId> entityUpdates;
    AZStd::unordered_set<AZ::EntityId> entityRemovals = assetEntityIds;

    for (AZ::EntityId liveEntityId : liveEntityIds)
    {
        entityRemovals.erase(liveEntityId);
        if (assetEntityIds.find(liveEntityId) != assetEntityIds.end())
        {
            entityUpdates.insert(liveEntityId);
        }
        else
        {
            entityAdds.insert(liveEntityId);
        }
    }

    // Make a transaction targeting the specified slice
    SliceTransaction::TransactionPtr transaction = SliceTransaction::BeginSlicePush(sliceAssetRef);
    if (!transaction)
    {
        gEnv->pLog->LogWarning("Unable to update slice: %s", filename.toUtf8().data());
        return false;
    }

    // Tell the transaction about all adds/updates/removals
    for (AZ::EntityId id : entityAdds)
    {
        SliceTransaction::Result result = transaction->AddEntity(id);
        if (!result)
        {
            gEnv->pLog->LogWarning("Error adding entity with ID %s to slice: %s\n\n%s",
                id.ToString().c_str(), filename.toUtf8().data(), result.GetError().c_str());
            return false;
        }
    }

    for (AZ::EntityId id : entityRemovals)
    {
        SliceTransaction::Result result = transaction->RemoveEntity(id);
        if (!result)
        {
            gEnv->pLog->LogWarning("Error removing entity with ID %s from slice: %s\n\n%s",
                id.ToString().c_str(), filename.toUtf8().data(), result.GetError().c_str());
            return false;
        }
    }

    for (AZ::EntityId id : entityUpdates)
    {
        SliceTransaction::Result result = transaction->UpdateEntity(id);
        if (!result)
        {
            gEnv->pLog->LogWarning("Error updating entity with ID %s in slice: %s\n\n%s",
                id.ToString().c_str(), filename.toUtf8().data(), result.GetError().c_str());
            return false;
        }
    }

    // Commit
    SliceTransaction::Result commitResult = transaction->Commit(
        targetAssetId,
        SlicePreSaveCallbackForWorldEntities,
        nullptr,
        SliceTransaction::SliceCommitFlags::DisableUndoCapture);

    if (!commitResult)
    {
        gEnv->pLog->LogWarning("Failed to to save slice \"%s\".\n\nError:\n%s",
            filename.toUtf8().data(), commitResult.GetError().c_str());
        return false;
    }

    return true;
}

bool CCryEditDoc::LoadEntitiesFromLevel(const QString& levelPakFile)
{
    bool isPrefabEnabled = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(isPrefabEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

    bool loadedSuccessfully = false;

    if (!isPrefabEnabled)
    {
        auto pakSystem = GetIEditor()->GetSystem()->GetIPak();
        bool pakOpened = pakSystem->OpenPack(levelPakFile.toUtf8().data());
        if (pakOpened)
        {
            const QString entityFilename = Path::GetPath(levelPakFile) + "levelentities.editor_xml";

            CCryFile entitiesFile;
            if (entitiesFile.Open(entityFilename.toUtf8().data(), "rt"))
            {
                AZStd::vector<char> fileBuffer;
                fileBuffer.resize(entitiesFile.GetLength());
                if (!fileBuffer.empty())
                {
                    if (fileBuffer.size() == entitiesFile.ReadRaw(fileBuffer.begin(), fileBuffer.size()))
                    {
                        AZ::IO::ByteContainerStream<AZStd::vector<char>> fileStream(&fileBuffer);

                        EBUS_EVENT_RESULT(
                            loadedSuccessfully, AzToolsFramework::EditorEntityContextRequestBus, LoadFromStreamWithLayers, fileStream,
                            levelPakFile);
                    }
                    else
                    {
                        AZ_Error(
                            "Editor", false, "Failed to load level entities because the file \"%s\" could not be read.",
                            entityFilename.toUtf8().data());
                    }
                }
                else
                {
                    AZ_Error(
                        "Editor", false, "Failed to load level entities because the file \"%s\" is empty.", entityFilename.toUtf8().data());
                }

                entitiesFile.Close();
            }
            else
            {
                AZ_Error(
                    "Editor", false, "Failed to load level entities because the file \"%s\" was not found.",
                    entityFilename.toUtf8().data());
            }

            pakSystem->ClosePack(levelPakFile.toUtf8().data());
        }
    }
    else
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "No File IO implementation available");

        AZ::IO::HandleType fileHandle;
        AZ::IO::Result openResult = fileIO->Open(levelPakFile.toUtf8().data(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, fileHandle);
        if (openResult)
        {
            AZ::IO::FileIOStream stream(fileHandle, AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, false);
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                loadedSuccessfully, &AzToolsFramework::EditorEntityContextRequests::LoadFromStreamWithLayers, stream, levelPakFile);
            stream.Close();
        }
    }

    return loadedSuccessfully;
}

bool CCryEditDoc::LoadEntitiesFromSlice(const QString& sliceFile)
{
    bool sliceLoaded = false;
    {
        AZ::IO::FileIOStream sliceFileStream(sliceFile.toUtf8().data(), AZ::IO::OpenMode::ModeRead);
        if (!sliceFileStream.IsOpen())
        {
            AZ_Error("Editor", false, "Failed to load entities because the file \"%s\" could not be read.", sliceFile.toUtf8().data());
            return false;
        }

        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(sliceLoaded, &AzToolsFramework::EditorEntityContextRequestBus::Events::LoadFromStream, sliceFileStream);
    }

    if (!sliceLoaded)
    {
        AZ_Error("Editor", false, "Failed to load entities from slice file \"%s\"", sliceFile.toUtf8().data());
        return false;
    }

    return true;
}

bool CCryEditDoc::LoadLevel(TDocMultiArchive& arrXmlAr, const QString& absoluteCryFilePath)
{
    bool isPrefabEnabled = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(isPrefabEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

    auto pIPak = GetIEditor()->GetSystem()->GetIPak();

    QString folderPath = QFileInfo(absoluteCryFilePath).absolutePath();

    OnStartLevelResourceList();

    // Load next level resource list.
    if (!isPrefabEnabled)
    {
        pIPak->GetResourceList(AZ::IO::IArchive::RFOM_NextLevel)->Load(Path::Make(folderPath, "resourcelist.txt").toUtf8().data());
    }

    GetIEditor()->Notify(eNotify_OnBeginLoad);
    CrySystemEventBus::Broadcast(&CrySystemEventBus::Events::OnCryEditorBeginLoad);
    //GetISystem()->GetISystemEventDispatcher()->OnSystemEvent( ESYSTEM_EVENT_LEVEL_LOAD_START,0,0 );
    DeleteContents();

    // Set level path directly *after* DeleteContents(), since that will unload the previous level and clear the level path.
    GetIEditor()->GetGameEngine()->SetLevelPath(folderPath);

    SetModifiedFlag(true);  // dirty during de-serialize
    SetModifiedModules(eModifiedAll);
    Load(arrXmlAr, absoluteCryFilePath);

    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_LOAD_END, 0, 0);
    // We don't need next level resource list anymore.
    if (!isPrefabEnabled)
    {
        pIPak->GetResourceList(AZ::IO::IArchive::RFOM_NextLevel)->Clear();
    }
    SetModifiedFlag(false); // start off with unmodified
    SetModifiedModules(eModifiedNothing);
    SetDocumentReady(true);
    GetIEditor()->Notify(eNotify_OnEndLoad);
    CrySystemEventBus::Broadcast(&CrySystemEventBus::Events::OnCryEditorEndLoad);

    GetIEditor()->SetStatusText("Ready");

    return true;
}

void CCryEditDoc::Hold(const QString& holdName)
{
    Hold(holdName, holdName);
}

void CCryEditDoc::Hold(const QString& holdName, const QString& relativeHoldPath)
{
    if (!IsDocumentReady() || GetEditMode() == CCryEditDoc::DocumentEditingMode::SliceEdit)
    {
        return;
    }

    QString levelPath = GetIEditor()->GetGameEngine()->GetLevelPath();
    char resolvedLevelPath[AZ_MAX_PATH_LEN] = { 0 };
    AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(levelPath.toUtf8().data(), resolvedLevelPath, AZ_MAX_PATH_LEN);

    QString holdPath = QString::fromUtf8(resolvedLevelPath) + "/" + relativeHoldPath + "/";
    QString holdFilename = holdPath + holdName + GetIEditor()->GetGameEngine()->GetLevelExtension();

    // never auto-backup while we're trying to hold.
    bool oldBackup = gSettings.bBackupOnSave;
    gSettings.bBackupOnSave = false;
    SaveLevel(holdFilename);
    gSettings.bBackupOnSave = oldBackup;

    GetIEditor()->GetGameEngine()->SetLevelPath(levelPath);
}

void CCryEditDoc::Fetch(const QString& relativeHoldPath, bool bShowMessages, bool bDelHoldFolder)
{
    Fetch(relativeHoldPath, relativeHoldPath, bShowMessages, bDelHoldFolder ? FetchPolicy::DELETE_FOLDER : FetchPolicy::PRESERVE);
}

void CCryEditDoc::Fetch(const QString& holdName, const QString& relativeHoldPath, bool bShowMessages, FetchPolicy policy)
{
    if (!IsDocumentReady() || GetEditMode() == CCryEditDoc::DocumentEditingMode::SliceEdit)
    {
        return;
    }

    QString levelPath = GetIEditor()->GetGameEngine()->GetLevelPath();
    char resolvedLevelPath[AZ_MAX_PATH_LEN] = { 0 };
    AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(levelPath.toUtf8().data(), resolvedLevelPath, AZ_MAX_PATH_LEN);

    QString holdPath = QString::fromUtf8(resolvedLevelPath) + "/" + relativeHoldPath + "/";
    QString holdFilename = holdPath + holdName + GetIEditor()->GetGameEngine()->GetLevelExtension();

    {
        QFile cFile(holdFilename);
        // Open the file for writing, create it if needed
        if (!cFile.open(QFile::ReadOnly))
        {
            if (bShowMessages)
            {
                QMessageBox::information(QApplication::activeWindow(), QString(), QObject::tr("You have to use 'Hold' before you can fetch!"));
            }
            return;
        }
    }

    // Does the document contain unsaved data ?
    if (bShowMessages && IsModified() &&
        QMessageBox::question(QApplication::activeWindow(), QString(), QObject::tr("The document contains unsaved data, it will be lost if fetched.\r\nReally fetch old state?")) != QMessageBox::Yes)
    {
        return;
    }

    GetIEditor()->FlushUndo();

    TDocMultiArchive arrXmlAr = {};
    if (!LoadXmlArchiveArray(arrXmlAr, holdFilename, holdPath))
    {
        QMessageBox::critical(QApplication::activeWindow(), "Error", "The temporary 'Hold' level failed to load successfully.  Your level might be corrupted, you should restart the Editor.", QMessageBox::Ok);
        AZ_Error("EditDoc", false, "Fetch failed to load the Xml Archive");
        return;
    }

    // Load the state
    LoadLevel(arrXmlAr, holdFilename);

    // Load AZ entities for the editor.
    LoadEntitiesFromLevel(holdFilename);

    GetIEditor()->GetGameEngine()->SetLevelPath(levelPath);

    GetIEditor()->FlushUndo();

    switch (policy)
    {
    case FetchPolicy::DELETE_FOLDER:
        CFileUtil::Deltree(holdPath.toUtf8().data(), true);
        break;

    case FetchPolicy::DELETE_LY_FILE:
        CFileUtil::DeleteFile(holdFilename);
        break;

    default:
        break;
    }
}



//////////////////////////////////////////////////////////////////////////
namespace {
    struct SFolderTime
    {
        QString folder;
        time_t creationTime;
    };

    bool SortByCreationTime(SFolderTime& a, SFolderTime& b)
    {
        return a.creationTime < b.creationTime;
    }

    // This function, given a source folder to scan, returns all folders within that folder
    // non-recursively.  They will be sorted by time, with most recent first, and oldest last.
    void CollectAllFoldersByTime(const char* sourceFolder, std::vector<SFolderTime>& outputFolders)
    {
        QString folderMask(sourceFolder);
        AZ::IO::ArchiveFileIterator handle = gEnv->pCryPak->FindFirst((folderMask + "/*").toUtf8().data());
        if (handle)
        {
            do
            {
                if (handle.m_filename.front() == '.')
                {
                    continue;
                }

                if ((handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory)
                {
                    SFolderTime ft;
                    ft.folder = QString::fromUtf8(handle.m_filename.data(), aznumeric_cast<int>(handle.m_filename.size()));
                    ft.creationTime = handle.m_fileDesc.tCreate;
                    outputFolders.push_back(ft);
                }
            } while (handle = gEnv->pCryPak->FindNext(handle));

            gEnv->pCryPak->FindClose(handle);
        }
        std::sort(outputFolders.begin(), outputFolders.end(), SortByCreationTime);
    }
}

bool CCryEditDoc::BackupBeforeSave(bool force)
{
    // This function will copy the contents of an entire level folder to a backup folder
    // and delete older ones based on user preferences.
    if (!force && !gSettings.bBackupOnSave)
    {
        return true; // not an error
    }

    QString levelPath = GetIEditor()->GetGameEngine()->GetLevelPath();
    if (levelPath.isEmpty())
    {
        return false;
    }

    char resolvedLevelPath[AZ_MAX_PATH_LEN] = { 0 };
    AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(levelPath.toUtf8().data(), resolvedLevelPath, AZ_MAX_PATH_LEN);
    QWaitCursor wait;

    QString saveBackupPath = QString::fromUtf8(resolvedLevelPath) + "/" + kSaveBackupFolder;

    std::vector<SFolderTime> folders;
    CollectAllFoldersByTime(saveBackupPath.toUtf8().data(), folders);

    for (int i = int(folders.size()) - gSettings.backupOnSaveMaxCount; i >= 0; --i)
    {
        CFileUtil::Deltree(QStringLiteral("%1/%2/").arg(saveBackupPath, folders[i].folder).toUtf8().data(), true);
    }

    QDateTime theTime = QDateTime::currentDateTime();
    QString subFolder = theTime.toString("yyyy-MM-dd [HH.mm.ss]");

    QString levelName = GetIEditor()->GetGameEngine()->GetLevelName();
    QString backupPath = saveBackupPath + "/" + subFolder;
    AZ::IO::FileIOBase::GetDirectInstance()->CreatePath(backupPath.toUtf8().data());

    QString sourcePath = QString::fromUtf8(resolvedLevelPath) + "/";

    QString ignoredFiles;

    for (const char* backupOrTempFolderName : kBackupOrTempFolders)
    {
        if (!ignoredFiles.isEmpty())
        {
            ignoredFiles += "|";
        }
        ignoredFiles += QString::fromUtf8(backupOrTempFolderName);
    }

    // copy that whole tree:
    AZ_TracePrintf("Editor", "Saving level backup to '%s'...\n", backupPath.toUtf8().data());
    if (IFileUtil::ETREECOPYOK != CFileUtil::CopyTree(sourcePath, backupPath, true, false, ignoredFiles.toUtf8().data()))
    {
        gEnv->pLog->LogWarning("Attempting to save backup to %s before saving, but could not write all files.", backupPath.toUtf8().data());
        return false;
    }
    return true;
}

void CCryEditDoc::SaveAutoBackup(bool bForce)
{
    if (!bForce && (!gSettings.autoBackupEnabled || GetIEditor()->IsInGameMode()))
    {
        return;
    }

    QString levelPath = GetIEditor()->GetGameEngine()->GetLevelPath();
    if (levelPath.isEmpty())
    {
        return;
    }

    static bool isInProgress = false;
    if (isInProgress)
    {
        return;
    }

    isInProgress = true;

    QWaitCursor wait;

    QString autoBackupPath = levelPath + "/" + kAutoBackupFolder;

    // collect all subfolders
    std::vector<SFolderTime> folders;

    CollectAllFoldersByTime(autoBackupPath.toUtf8().data(), folders);

    for (int i = int(folders.size()) - gSettings.autoBackupMaxCount; i >= 0; --i)
    {
        CFileUtil::Deltree(QStringLiteral("%1/%2/").arg(autoBackupPath, folders[i].folder).toUtf8().data(), true);
    }

    // save new backup
    QDateTime theTime = QDateTime::currentDateTime();
    QString subFolder = theTime.toString(QStringLiteral("yyyy-MM-dd [HH.mm.ss]"));

    QString levelName = GetIEditor()->GetGameEngine()->GetLevelName();
    QString filename = autoBackupPath + "/" + subFolder + "/" + levelName + "/" + levelName + GetIEditor()->GetGameEngine()->GetLevelExtension();
    SaveLevel(filename);
    GetIEditor()->GetGameEngine()->SetLevelPath(levelPath);

    isInProgress = false;
}

bool CCryEditDoc::IsLevelExported() const
{
    return m_boLevelExported;
}

void CCryEditDoc::SetLevelExported(bool boExported)
{
    m_boLevelExported = boExported;
}

void CCryEditDoc::RegisterListener(IDocListener* listener)
{
    if (listener == nullptr)
    {
        return;
    }

    if (std::find(m_listeners.begin(), m_listeners.end(), listener) == m_listeners.end())
    {
        m_listeners.push_back(listener);
    }
}

void CCryEditDoc::UnregisterListener(IDocListener* listener)
{
    m_listeners.remove(listener);
}

void CCryEditDoc::LogLoadTime(int time) const
{
    QString appFilePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    QString exePath = Path::GetPath(appFilePath);
    QString filename = Path::Make(exePath, "LevelLoadTime.log");
    QString level = GetIEditor()->GetGameEngine()->GetLevelPath();

    CLogFile::FormatLine("[LevelLoadTime] Level %s loaded in %d seconds", level.toUtf8().data(), time / 1000);
#if defined(AZ_PLATFORM_WINDOWS)
    SetFileAttributesW(filename.toStdWString().c_str(), FILE_ATTRIBUTE_ARCHIVE);
#endif

    QFile file(filename);
    if (!file.open(QFile::Append | QFile::Text))
    {
        return;
    }

    char version[50];
    GetIEditor()->GetFileVersion().ToShortString(version, AZ_ARRAY_SIZE(version));

    time = time / 1000;
    QString text = QStringLiteral("\n[%1] Level %2 loaded in %3 seconds").arg(version, level).arg(time);
    file.write(text.toUtf8());
}

void CCryEditDoc::SetDocumentReady(bool bReady)
{
    m_bDocumentReady = bReady;
}

void CCryEditDoc::RegisterConsoleVariables()
{
    doc_validate_surface_types = gEnv->pConsole->GetCVar("doc_validate_surface_types");

    if (!doc_validate_surface_types)
    {
        doc_validate_surface_types = REGISTER_INT_CB("doc_validate_surface_types", 0, 0,
            "Flag indicating whether icons are displayed on the animation graph.\n"
            "Default is 1.\n",
            OnValidateSurfaceTypesChanged);
    }
}

void CCryEditDoc::OnValidateSurfaceTypesChanged(ICVar*)
{
    CErrorsRecorder errorsRecorder(GetIEditor());
    CSurfaceTypeValidator().Validate();
}

void CCryEditDoc::OnStartLevelResourceList()
{
    // after loading another level we clear the RFOM_Level list, the first time the list should be empty
    static bool bFirstTime = true;

    if (bFirstTime)
    {
        const char* pResFilename = gEnv->pCryPak->GetResourceList(AZ::IO::IArchive::RFOM_Level)->GetFirst();

        while (pResFilename)
        {
            // This should be fixed because ExecuteCommandLine is executed right after engine init as we assume the
            // engine already has all data loaded an is initialized to process commands. Loading data afterwards means
            // some init was done later which can cause problems when running in the engine batch mode (executing console commands).
            gEnv->pLog->LogError("'%s' was loaded after engine init but before level load/new (should be fixed)", pResFilename);
            pResFilename = gEnv->pCryPak->GetResourceList(AZ::IO::IArchive::RFOM_Level)->GetNext();
        }

        bFirstTime = false;
    }

    gEnv->pCryPak->GetResourceList(AZ::IO::IArchive::RFOM_Level)->Clear();
}

bool CCryEditDoc::DoFileSave()
{
    if (GetEditMode() == CCryEditDoc::DocumentEditingMode::LevelEdit)
    {
        // If the file to save is the temporary level it should 'save as' since temporary levels will get deleted
        const char* temporaryLevelName = GetTemporaryLevelName();
        if (QString::compare(GetIEditor()->GetLevelName(), temporaryLevelName) == 0)
        {
            QString filename;
            if (CCryEditApp::instance()->GetDocManager()->DoPromptFileName(filename, ID_FILE_SAVE_AS, 0, false, nullptr)
                && !filename.isEmpty() && !QFileInfo(filename).exists())
            {
                if (SaveLevel(filename))
                {
                    DeleteTemporaryLevel();
                    QString newLevelPath = filename.left(filename.lastIndexOf('/') + 1);
                    GetIEditor()->GetDocument()->SetPathName(filename);
                    GetIEditor()->GetGameEngine()->SetLevelPath(newLevelPath);
                    return true;
                }
            }
            return false;
        }
    }
    if (!IsDocumentReady())
    {
        return false;
    }

    return Internal::SaveLevel();
}

const char* CCryEditDoc::GetTemporaryLevelName() const
{
    return gEnv->pConsole->GetCVar("g_TemporaryLevelName")->GetString();
}

void CCryEditDoc::DeleteTemporaryLevel()
{
    QString tempLevelPath = (Path::GetEditingGameDataFolder() + "/Levels/" + GetTemporaryLevelName()).c_str();
    GetIEditor()->GetSystem()->GetIPak()->ClosePacks(tempLevelPath.toUtf8().data());
    CFileUtil::Deltree(tempLevelPath.toUtf8().data(), true);
}

void CCryEditDoc::InitEmptyLevel(int /*resolution*/, int /*unitSize*/, bool /*bUseTerrain*/)
{
    GetIEditor()->SetStatusText("Initializing Level...");

    OnStartLevelResourceList();

    GetIEditor()->Notify(eNotify_OnBeginNewScene);
    CLogFile::WriteLine("Preparing new document...");

    //cleanup resources!
    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_POST_UNLOAD, 0, 0);

    //////////////////////////////////////////////////////////////////////////
    // Initialize defaults.
    //////////////////////////////////////////////////////////////////////////
    if (!GetIEditor()->IsInPreviewMode())
    {
        GetIEditor()->ReloadTemplates();
        m_environmentTemplate = GetIEditor()->FindTemplate("Environment");

        GetIEditor()->GetGameEngine()->SetLevelCreated(true);
        GetIEditor()->GetGameEngine()->SetLevelCreated(false);
    }

    {
        // Notify listeners.
        std::list<IDocListener*> listeners = m_listeners;
        for (IDocListener* listener : listeners)
        {
            listener->OnNewDocument();
        }
    }

    // Tell the system that the level has been created/loaded.
    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_LOAD_END, 0, 0);

    GetIEditor()->Notify(eNotify_OnEndNewScene);
    SetModifiedFlag(false);
    SetLevelExported(false);
    SetModifiedModules(eModifiedNothing);

    GetIEditor()->SetStatusText("Ready");
}

void CCryEditDoc::CreateDefaultLevelAssets([[maybe_unused]] int resolution, [[maybe_unused]] int unitSize)
{
    AzToolsFramework::EditorLevelNotificationBus::Broadcast(&AzToolsFramework::EditorLevelNotificationBus::Events::OnNewLevelCreated);
}

void CCryEditDoc::OnEnvironmentPropertyChanged(IVariable* pVar)
{
    if (pVar == nullptr)
    {
        return;
    }

    XmlNodeRef node = GetEnvironmentTemplate();
    if (node == nullptr)
    {
        return;
    }

    // QVariant will not convert a void * to int, so do it manually.
    int nKey = static_cast<int>(reinterpret_cast<intptr_t>(pVar->GetUserData().value<void*>()));

    int nGroup = (nKey & 0xFFFF0000) >> 16;
    int nChild = (nKey & 0x0000FFFF);

    if (nGroup < 0 || nGroup >= node->getChildCount())
    {
        return;
    }

    XmlNodeRef groupNode = node->getChild(nGroup);

    if (groupNode == nullptr)
    {
        return;
    }

    if (nChild < 0 || nChild >= groupNode->getChildCount())
    {
        return;
    }

    XmlNodeRef childNode = groupNode->getChild(nChild);
    if (childNode == nullptr)
    {
        return;
    }
    QString childValue;

    if (pVar->GetDataType() == IVariable::DT_COLOR)
    {
        Vec3 value;
        pVar->Get(value);
        QColor gammaColor = ColorLinearToGamma(ColorF(value.x, value.y, value.z));
        childValue = QStringLiteral("%1,%2,%3").arg(gammaColor.red()).arg(gammaColor.green()).arg(gammaColor.blue());
    }
    else
    {
        pVar->Get(childValue);
    }
    childNode->setAttr("value", childValue.toUtf8().data());
}

QString CCryEditDoc::GetCryIndexPath(const char* levelFilePath) const
{
    QString levelPath = Path::GetPath(levelFilePath);
    QString levelName = Path::GetFileName(levelFilePath);
    return Path::AddPathSlash(levelPath + levelName + "_editor");
}

bool CCryEditDoc::LoadXmlArchiveArray(TDocMultiArchive& arrXmlAr, const QString& absoluteLevelPath, const QString& levelPath)
{
    auto pIPak = GetIEditor()->GetSystem()->GetIPak();

    //if (m_pSWDoc->IsNull())
    {
        CXmlArchive* pXmlAr = new CXmlArchive();
        if (!pXmlAr)
        {
            return false;
        }

        CXmlArchive& xmlAr = *pXmlAr;
        xmlAr.bLoading = true;

        // bound to the level folder, as if it were the assets folder.
        // this mounts (whateverlevelname.ly) as @products@/Levels/whateverlevelname/ and thus it works...
        bool openLevelPakFileSuccess = pIPak->OpenPack(levelPath.toUtf8().data(), absoluteLevelPath.toUtf8().data());
        if (!openLevelPakFileSuccess)
        {
            return false;
        }

        CPakFile pakFile;
        bool loadFromPakSuccess = xmlAr.LoadFromPak(levelPath, pakFile);
        pIPak->ClosePack(absoluteLevelPath.toUtf8().data());
        if (!loadFromPakSuccess)
        {
            return false;
        }

        FillXmlArArray(arrXmlAr, &xmlAr);
    }

    return true;
}

void CCryEditDoc::ReleaseXmlArchiveArray(TDocMultiArchive& arrXmlAr)
{
    SAFE_DELETE(arrXmlAr[0]);
}

//////////////////////////////////////////////////////////////////////////
// AzToolsFramework::EditorEntityContextNotificationBus interface implementation
void CCryEditDoc::OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, AZ::SliceComponent::SliceInstanceAddress& sliceAddress, const AzFramework::SliceInstantiationTicket& /*ticket*/)
{
    if (m_envProbeSliceAssetId == sliceAssetId)
    {
        const AZ::SliceComponent::EntityList& entities = sliceAddress.GetInstance()->GetInstantiated()->m_entities;
        const AZ::Uuid editorEnvProbeComponentId("{8DBD6035-583E-409F-AFD9-F36829A0655D}");
        AzToolsFramework::EntityIdList entityIds;
        entityIds.reserve(entities.size());
        for (const AZ::Entity* entity : entities)
        {
            if (entity->FindComponent(editorEnvProbeComponentId))
            {
                // Update Probe Area size to cover the whole terrain
                LmbrCentral::EditorLightComponentRequestBus::Event(entity->GetId(), &LmbrCentral::EditorLightComponentRequests::SetProbeAreaDimensions, AZ::Vector3(m_terrainSize, m_terrainSize, m_envProbeHeight));

                // Force update the light to apply cubemap
                LmbrCentral::EditorLightComponentRequestBus::Event(entity->GetId(), &LmbrCentral::EditorLightComponentRequests::RefreshLight);
            }
            entityIds.push_back(entity->GetId());
        }

        //Detach instantiated env probe entities from engine slice
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Broadcast(
            &AzToolsFramework::SliceEditorEntityOwnershipServiceRequests::DetachSliceEntities, entityIds);

        sliceAddress.SetInstance(nullptr);
        sliceAddress.SetReference(nullptr);
        SetModifiedFlag(true);
        SetModifiedModules(eModifiedEntities);

        AzToolsFramework::SliceEditorEntityOwnershipServiceNotificationBus::Handler::BusDisconnect();

        //save after level default slice fully instantiated
        Save();
    }
    GetIEditor()->ResumeUndo();
}


void CCryEditDoc::OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId, const AzFramework::SliceInstantiationTicket& /*ticket*/)
{
    if (m_envProbeSliceAssetId == sliceAssetId)
    {
        AzToolsFramework::SliceEditorEntityOwnershipServiceNotificationBus::Handler::BusDisconnect();
        AZ_Warning("Editor", false, "Failed to instantiate default environment probe slice.");
    }
    GetIEditor()->ResumeUndo();
}
//////////////////////////////////////////////////////////////////////////

namespace AzToolsFramework
{
    void CryEditDocFuncsHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // this will put these methods into the 'azlmbr.legacy.general' module
            auto addLegacyGeneral = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Legacy/Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "legacy.general");
            };
            addLegacyGeneral(behaviorContext->Method("save_level", ::Internal::SaveLevel, nullptr, "Saves the current level."));
        }
    }
}

#include <moc_CryEditDoc.cpp>
