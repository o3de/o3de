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
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>
#include <AzToolsFramework/Prefab/PrefabLoaderInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/UI/Prefab/PrefabIntegrationInterface.h>
#include <AzCore/Component/Component.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <TimeValue.h>
#include <IEditor.h>
#endif

class CClouds;
struct LightingSettings;
struct IVariable;
struct ICVar;

// Filename of the temporary file used for the hold / fetch operation
// conform to the "$tmp[0-9]_" naming convention
#define HOLD_FETCH_FILE "$tmp_hold"

class CCryEditDoc
    : public QObject
    , protected AzToolsFramework::SliceEditorEntityOwnershipServiceNotificationBus::Handler
{
    Q_OBJECT
    Q_PROPERTY(bool modified READ IsModified WRITE SetModifiedFlag);
    Q_PROPERTY(QString pathName READ GetLevelPathName WRITE SetPathName);
    Q_PROPERTY(QString title READ GetTitle WRITE SetTitle);

public: // Create from serialization only
    enum DocumentEditingMode
    {
        LevelEdit,
        SliceEdit
    };

    Q_INVOKABLE CCryEditDoc();

    virtual ~CCryEditDoc();

    bool IsModified() const;
    void SetModifiedFlag(bool modified);
    // Currently it's not possible to disable one single flag and eModifiedModule is ignored
    // if bModified is false.
    virtual void SetModifiedModules(EModifiedModule eModifiedModule, bool boSet = true);
    int GetModifiedModule();

    // Return level path.
    // If a slice is being edited then the path to a placeholder level is returned.
    QString GetLevelPathName() const;

    // Set path of slice or level being edited.
    void SetPathName(const QString& pathName);

    // Return slice path, if slice is open, an empty string otherwise. Use GetEditMode() to determine if a slice is open.
    QString GetSlicePathName() const;

    // Return The current type of document being edited, a level or a slice
    // While editing a slice, a placeholder level is opened since the editor requires an open level to function.
    // While editing a slice, saving will only affect the slice and not the placeholder level.
    DocumentEditingMode GetEditMode() const;

    // Return slice path if editing slice, level path if editing level.
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

    CClouds* GetClouds() { return m_pClouds; }
    void SetWaterColor(const QColor& col) { m_waterColor = col; }
    QColor GetWaterColor() const { return m_waterColor; }
    XmlNodeRef& GetFogTemplate() { return m_fogTemplate; }
    XmlNodeRef& GetEnvironmentTemplate() { return m_environmentTemplate; }
    void OnEnvironmentPropertyChanged(IVariable* pVar);

    void RegisterListener(IDocListener* listener);
    void UnregisterListener(IDocListener* listener);

    static bool IsBackupOrTempLevelSubdirectory(const QString& folderName);
protected:

    virtual void DeleteContents();

    struct TOpenDocContext
    {
        CTimeValue loading_start_time;
        QString absoluteLevelPath;
        QString absoluteSlicePath;
    };
    bool BeforeOpenDocument(const QString& lpszPathName, TOpenDocContext& context);
    bool DoOpenDocument(TOpenDocContext& context);
    virtual bool LoadXmlArchiveArray(TDocMultiArchive& arrXmlAr, const QString& absoluteLevelPath, const QString& levelPath);
    virtual void ReleaseXmlArchiveArray(TDocMultiArchive& arrXmlAr);

    virtual void Load(TDocMultiArchive& arrXmlAr, const QString& szFilename);
    virtual void StartStreamingLoad(){}

    void Save(CXmlArchive& xmlAr);
    void Load(CXmlArchive& xmlAr, const QString& szFilename);
    void Save(TDocMultiArchive& arrXmlAr);
    bool SaveLevel(const QString& filename);
    bool SaveSlice(const QString& filename);
    bool LoadLevel(TDocMultiArchive& arrXmlAr, const QString& absoluteCryFilePath);
    bool LoadEntitiesFromLevel(const QString& levelPakFile);
    bool LoadEntitiesFromSlice(const QString& sliceFile);
    void SerializeFogSettings(CXmlArchive& xmlAr);
    virtual void SerializeViewSettings(CXmlArchive& xmlAr);
    void SerializeNameSelection(CXmlArchive& xmlAr);
    void LogLoadTime(int time) const;

    struct TSaveDocContext
    {
        bool bSaved;
    };
    bool BeforeSaveDocument(const QString& lpszPathName, TSaveDocContext& context);
    bool HasLayerNameConflicts() const;
    bool DoSaveDocument(const QString& lpszPathName, TSaveDocContext& context);
    bool AfterSaveDocument(const QString& lpszPathName, TSaveDocContext& context, bool bShowPrompt = true);

    virtual bool OnSaveDocument(const QString& lpszPathName);
    virtual void OnFileSaveAs();
    //! called immediately after saving the level.
    void AfterSave();
    void RegisterConsoleVariables();
    void OnStartLevelResourceList();
    static void OnValidateSurfaceTypesChanged(ICVar*);

    QString GetCryIndexPath(const char* levelFilePath) const;

    //////////////////////////////////////////////////////////////////////////
    // SliceEditorEntityOwnershipServiceNotificationBus::Handler
    void OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, AZ::SliceComponent::SliceInstanceAddress& sliceAddress, const AzFramework::SliceInstantiationTicket& /*ticket*/) override;
    void OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId, const AzFramework::SliceInstantiationTicket& /*ticket*/) override;
    //////////////////////////////////////////////////////////////////////////

    bool m_bLoadFailed = false;
    QColor m_waterColor = QColor(0, 0, 255);
    XmlNodeRef m_fogTemplate;
    XmlNodeRef m_environmentTemplate;
    CClouds* m_pClouds;
    std::list<IDocListener*> m_listeners;
    bool m_bDocumentReady = false;
    ICVar* doc_validate_surface_types = nullptr;
    int m_modifiedModuleFlags;
    // On construction, it assumes loaded levels have already been exported. Can be a big fat lie, though.
    // The right way would require us to save to the level folder the export status of the level.
    bool m_boLevelExported = true;
    bool m_modified = false;
    QString m_pathName;
    QString m_slicePathName;
    QString m_title;
    AZ::Data::AssetId m_envProbeSliceAssetId;
    float m_terrainSize;
    const char* m_envProbeSliceRelativePath = "EngineAssets/Slices/DefaultLevelSetup.slice";
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
