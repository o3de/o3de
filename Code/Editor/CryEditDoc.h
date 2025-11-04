/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_CRYEDITDOC_H
#define CRYINCLUDE_EDITOR_CRYEDITDOC_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "DocMultiArchive.h"
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/UI/Prefab/PrefabIntegrationInterface.h>
#include <AzCore/Component/Component.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <TimeValue.h>
#include <IEditor.h>
#endif

struct LightingSettings;
struct IVariable;
struct ICVar;

// Filename of the temporary file used for the hold / fetch operation
// conform to the "$tmp[0-9]_" naming convention
#define HOLD_FETCH_FILE "$tmp_hold"

class CCryEditDoc
    : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool modified READ IsModified WRITE SetModifiedFlag);
    Q_PROPERTY(QString pathName READ GetLevelPathName WRITE SetPathName);
    Q_PROPERTY(QString title READ GetTitle WRITE SetTitle);

public:
    Q_INVOKABLE CCryEditDoc();

    virtual ~CCryEditDoc();

    bool IsModified() const;
    void SetModifiedFlag(bool modified);
    // Currently it's not possible to disable one single flag and eModifiedModule is ignored
    // if bModified is false.
    virtual void SetModifiedModules(EModifiedModule eModifiedModule, bool boSet = true);
    int GetModifiedModule();

    // Return level path.
    QString GetLevelPathName() const;

    // Set path of level being edited.
    void SetPathName(const QString& pathName);

    SANDBOX_API QString GetActivePathName() const;

    QString GetTitle() const;
    void SetTitle(const QString& title);

    virtual bool OnNewDocument();
    void InitEmptyLevel(int resolution = 0, int unitSize = 0, bool bUseTerrain = false);
    void CreateDefaultLevelAssets(int resolution = 0, int unitSize = 0);

    bool DoSave(const QString& pathName, bool replace);
    SANDBOX_API bool Save();
    virtual bool DoFileSave();
    bool SaveModified();

    virtual bool BackupBeforeSave(bool bForce = false);
    void SaveAutoBackup(bool bForce = false);

    // ClassWizard generated virtual function overrides
    virtual bool OnOpenDocument(const QString& lpszPathName);

    bool IsLevelLoadFailed() const { return m_bLoadFailed; }

    //! Marks this document as having errors.
    void SetHasErrors() { m_hasErrors = true; }

    bool IsDocumentReady() const { return m_bDocumentReady; }
    void SetDocumentReady(bool bReady);

    bool IsLevelExported() const;
    void SetLevelExported(bool boExported = true);

    bool CanCloseFrame();

    enum class FetchPolicy
    {
        DELETE_FOLDER,
        DELETE_LY_FILE,
        PRESERVE
    };

    void Hold(const QString& holdName);
    void Hold(const QString& holdName, const QString& relativeHoldPath);
    void Fetch(const QString& relativeHoldPath, bool bShowMessages = true, bool bDelHoldFolder = false);
    void Fetch(const QString& holdName, const QString& relativeHoldPath, bool bShowMessages, FetchPolicy policy);

    const char* GetTemporaryLevelName() const;
    void DeleteTemporaryLevel();

    XmlNodeRef& GetEnvironmentTemplate() { return m_environmentTemplate; }
    void OnEnvironmentPropertyChanged(IVariable* pVar);

    void RegisterListener(IDocListener* listener);
    void UnregisterListener(IDocListener* listener);

    static bool IsBackupOrTempLevelSubdirectory(const QString& folderName);
    virtual void OnFileSaveAs();

protected:
    virtual void DeleteContents();

    struct TOpenDocContext
    {
        CTimeValue loading_start_time;
        QString absoluteLevelPath;
    };
    bool BeforeOpenDocument(const QString& lpszPathName, TOpenDocContext& context);
    bool DoOpenDocument(TOpenDocContext& context);
    virtual bool LoadXmlArchiveArray(TDocMultiArchive& arrXmlAr, const QString& absoluteLevelPath, const QString& levelPath);
    virtual void ReleaseXmlArchiveArray(TDocMultiArchive& arrXmlAr);

    virtual void Load(TDocMultiArchive& arrXmlAr, const QString& szFilename);
    virtual void StartStreamingLoad(){}

    void Load(CXmlArchive& xmlAr, const QString& szFilename);
    bool SaveLevel(const QString& filename);
    bool LoadLevel(TDocMultiArchive& arrXmlAr, const QString& absoluteCryFilePath);
    bool LoadEntitiesFromLevel(const QString& levelPakFile);
    void SerializeFogSettings(CXmlArchive& xmlAr);
    virtual void SerializeViewSettings(CXmlArchive& xmlAr);
    void LogLoadTime(int time) const;

    struct TSaveDocContext
    {
        bool bSaved;
    };
    bool BeforeSaveDocument(const QString& lpszPathName, TSaveDocContext& context);
    bool DoSaveDocument(const QString& lpszPathName, TSaveDocContext& context);
    bool AfterSaveDocument(const QString& lpszPathName, TSaveDocContext& context, bool bShowPrompt = true);

    virtual bool OnSaveDocument(const QString& lpszPathName);
    //! called immediately after saving the level.
    void AfterSave();
    void OnStartLevelResourceList();

    QString GetCryIndexPath(const char* levelFilePath) const;

    bool m_bLoadFailed = false;
    QColor m_waterColor = QColor(0, 0, 255);
    XmlNodeRef m_fogTemplate;
    XmlNodeRef m_environmentTemplate;
    std::list<IDocListener*> m_listeners;
    bool m_bDocumentReady = false;
    int m_modifiedModuleFlags;
    // On construction, it assumes loaded levels have already been exported. Can be a big fat lie, though.
    // The right way would require us to save to the level folder the export status of the level.
    bool m_boLevelExported = true;
    bool m_modified = false;
    QString m_pathName;
    QString m_title;
    const float m_envProbeHeight = 200.0f;
    bool m_hasErrors = false; ///< This is used to warn the user that they may lose work when they go to save.
    AzToolsFramework::Prefab::PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
    AzToolsFramework::PrefabEditorEntityOwnershipInterface* m_prefabEditorEntityOwnershipInterface = nullptr;
    AzToolsFramework::Prefab::PrefabLoaderInterface* m_prefabLoaderInterface = nullptr;
    AzToolsFramework::Prefab::PrefabIntegrationInterface* m_prefabIntegrationInterface = nullptr;
};

class CAutoDocNotReady
{
public:
    CAutoDocNotReady()
    {
        m_prevState = GetIEditor()->GetDocument()->IsDocumentReady();
        GetIEditor()->GetDocument()->SetDocumentReady(false);
    }

    ~CAutoDocNotReady()
    {
        GetIEditor()->GetDocument()->SetDocumentReady(m_prevState);
    }

private:
    bool m_prevState;
};

namespace AzToolsFramework
{
    //! A component to reflect scriptable commands for the Editor
    class CryEditDocFuncsHandler
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(CryEditDocFuncsHandler, "{628CE458-72E7-4B7B-B8A2-62F95F55E738}")

        SANDBOX_API static void Reflect(AZ::ReflectContext* context);

        // AZ::Component ...
        void Activate() override {}
        void Deactivate() override {}
    };

} // namespace AzToolsFramework

#endif // CRYINCLUDE_EDITOR_CRYEDITDOC_H
