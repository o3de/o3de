/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_MATERIAL_MATERIALMANAGER_H
#define CRYINCLUDE_EDITOR_MATERIAL_MATERIALMANAGER_H
#pragma once

#include "BaseLibraryManager.h"
#include "Material.h"

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <Include/IEditorMaterialManager.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/string/wildcard.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/parallel/thread.h>


class CMaterial;
class CMaterialLibrary;
class CMaterialHighlighter;

namespace AZ
{
    struct Uuid;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        struct SourceFileOpenerDetails;
        typedef AZStd::vector<SourceFileOpenerDetails> SourceFileOpenerList;
    }
}

enum EHighlightMode
{
    eHighlight_Pick = BIT(0),
    eHighlight_Breakable = BIT(1),
    eHighlight_NoSurfaceType = BIT(2),
    eHighlight_All = 0xFFFFFFFF
};

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
/** Manages all entity prototypes and material libraries.
*/
class CRYEDIT_API CMaterialManager
    : public IEditorMaterialManager
    , public CBaseLibraryManager
    , public IMaterialManagerListener
    , protected AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
    , public AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationBus::Handler
    , public AzFramework::AssetCatalogEventBus::Handler
    , public AzToolsFramework::EditorEvents::Bus::Handler
{
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:

    //! Notification callback.
    typedef AZStd::function<void()> NotifyCallback;

    CMaterialManager(CRegistrationContext& regCtx);
    ~CMaterialManager();

    void Set3DEngine();

    // Clear all prototypes and material libraries.
    void ClearAll();

    //////////////////////////////////////////////////////////////////////////
    // Materials.
    //////////////////////////////////////////////////////////////////////////

    // Loads material.
    CMaterial* LoadMaterial(const QString& sMaterialName, bool bMakeIfNotFound = true);
    XmlNodeRef LoadXmlNode(const QString &fullSourcePath, const QString &relativeFilePath);
    //! Loads a material, avoiding a call to CMaterialManager::MaterialToFilename if the full path is already known
    CMaterial* LoadMaterialWithFullSourcePath(const QString& relativeFilePath, const QString& fullSourcePath, bool makeIfNotFound = true);
    virtual CMaterial* LoadMaterial(const char* sMaterialName, bool bMakeIfNotFound = true);
    virtual void OnRequestMaterial(_smart_ptr<IMaterial> pMaterial);
    // Creates a new material from a xml node.
    CMaterial* CreateMaterial(const QString& sMaterialName, const XmlNodeRef& node = XmlNodeRef(), int nMtlFlags = 0, unsigned long nLoadingFlags = 0);
    virtual CMaterial* CreateMaterial(const char* sMaterialName, const XmlNodeRef& node = XmlNodeRef(), int nMtlFlags = 0, unsigned long nLoadingFlags = 0);

    // Duplicate material and do nothing more.
    CMaterial* DuplicateMaterial(const char* newName, CMaterial* pOriginal);

    // Delete specified material, erases material file, and unassigns from all objects.
    virtual void DeleteMaterial(CMaterial* pMtl);
    virtual void RemoveMaterialFromDisk(const char* fileName);


    //! Export property manager to game.
    void Export(XmlNodeRef& node);
    int ExportLib(CMaterialLibrary* pLib, XmlNodeRef& libNode);

    virtual void SetSelectedItem(IDataBaseItem* pItem);
    void SetCurrentMaterial(CMaterial* pMtl);
    //! Get currently active material.
    CMaterial* GetCurrentMaterial() const;

    void SetCurrentFolder(const QString& folder);

    // This material will be highlighted
    void SetHighlightedMaterial(CMaterial* pMtl);
    void GetHighlightColor(ColorF* color, float* intensity, int flags);
    void HighlightedMaterialChanged(CMaterial* pMtl);
    // highlightMask is a combination of EHighlightMode flags
    void SetHighlightMask(int highlightMask);
    int GetHighlightMask() const{ return m_highlightMask; }
    void SetMarkedMaterials(const std::vector<_smart_ptr<CMaterial> >& markedMaterials);
    void OnLoadShader(CMaterial* pMaterial);

    //! Serialize property manager.
    virtual void Serialize(XmlNodeRef& node, bool bLoading);

    virtual void SaveAllLibs();

    //////////////////////////////////////////////////////////////////////////
    // IMaterialManagerListener implementation
    //////////////////////////////////////////////////////////////////////////
    // Called when material manager tries to load a material.
    virtual void OnCreateMaterial(_smart_ptr<IMaterial> pMaterial);
    virtual void OnDeleteMaterial(_smart_ptr<IMaterial> pMaterial);
    virtual bool IsCurrentMaterial(_smart_ptr<IMaterial> pMaterial) const;
    //////////////////////////////////////////////////////////////////////////

    // Convert filename of material file into the name of the material.
    QString FilenameToMaterial(const QString& filename);

    // Convert name of the material to the filename.
    QString MaterialToFilename(const QString& sMaterialName);

    //! Get the full file path of the source material
    const AZ::Data::AssetType& GetMaterialAssetType();

    //////////////////////////////////////////////////////////////////////////
    // Convert 3DEngine IMaterial to Editor's CMaterial pointer.
    CMaterial* FromIMaterial(_smart_ptr<IMaterial> pMaterial);

    // Open File selection dialog to create a new material.
    CMaterial* SelectNewMaterial(int nMtlFlags, const char* sStartPath = NULL);

    // Synchronize material between 3dsMax and editor.
    void SyncMaterialEditor();

    //////////////////////////////////////////////////////////////////////////
    void GotoMaterial(CMaterial* pMaterial);
    void GotoMaterial(_smart_ptr<IMaterial> pMaterial);

    // Gather resources from the game material.
    static void GatherResources(_smart_ptr<IMaterial> pMaterial, CUsedResources& resources);

    void Command_Create();
    void Command_CreateMulti();
    void Command_ConvertToMulti();
    void Command_Duplicate();
    void Command_Merge();
    void Command_Delete();
    void Command_AssignToSelection();
    void Command_ResetSelection();
    void Command_SelectAssignedObjects();
    void Command_SelectFromObject();

protected:
    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // protected AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
    void AddSourceFileOpeners(const char* fullSourceFileName, const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers) override;
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // public AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationsBus::Handler
    void EntryAdded(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry);
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // AssetCatalogEventBus Functions
    void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////////////////////////////////
    // EditorEventsBus Functions
    bool SkipEditorStartupUI() override;
    //////////////////////////////////////////////////////////////////////////////////////////////////////

    void SaveDccMaterial(const AZStd::string& relativeDccMaterialPath);
    bool DccMaterialRequiresSave(const AZStd::string& relativeDccMaterialPath, const AZStd::string& fullSourcePath);
    void DccMaterialSourceControlCheck(const AZStd::string& relativeDccMaterialPath);
    void AddDccMaterialPath(const AZStd::string relativeDccMaterialPath);
    void TickSourceControl();
    void QueueSourceControlTick();

    // Duplicate the source material and set it as a submaterial of the target material at subMaterialIndex. Returns true if successful.
    bool DuplicateAsSubMaterialAtIndex(CMaterial* pSourceMaterial, CMaterial* pTargetMaterial, int subMaterialIndex);
    // Generates a unique variant of the name of the source material to resolve name collisions with the names of the submaterials in the target material.
    void GenerateUniqueSubmaterialName(const CMaterial* pSourceMaterial, const CMaterial* pTargetMaterial, QString& uniqueSubmaterialName) const;

    // Open save as dialog for saving materials.
    bool SelectSaveMaterial(QString& itemName, QString& fullPath, const char* defaultStartPath);

    void OnEditorNotifyEvent(EEditorNotifyEvent event);

    virtual CBaseLibraryItem* MakeNewItem();
    virtual CBaseLibrary* MakeNewLibrary();
    //! Root node where this library will be saved.
    virtual QString GetRootNodeName();
    //! Path to libraries in this manager.
    virtual QString GetLibsPath();
    virtual void ReportDuplicateItem(CBaseLibraryItem* pItem, CBaseLibraryItem* pOldItem);

    void RegisterCommands(CRegistrationContext& regCtx);

    void UpdateHighlightedMaterials();
    void AddForHighlighting(CMaterial* pMaterial);
    void RemoveFromHighlighting(CMaterial* pMaterial, int mask);
    int GetHighlightFlags(CMaterial* pMaterial) const;

    // For material syncing with 3dsMax.
    void PickPreviewMaterial();
    void InitMatSender();

    //! Reloads any registered materials that have been modified by the runtime.
    void ReloadDirtyMaterials();

protected:
    QString m_libsPath;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    // Currently selected (focused) material, in material browser.
    _smart_ptr<CMaterial> m_pCurrentMaterial;
    // current selected folder
    QString m_currentFolder;
    // List of materials selected in material browser tree.
    std::vector<_smart_ptr<CMaterial> > m_markedMaterials;
    // IMaterial is needed to not let 3Dengine release selected IMaterial
    _smart_ptr<IMaterial> m_pCurrentEngineMaterial;

    // Paths of .dccmtl that might require saving. They will be fed to
    // DccMaterialSourceControlCheck() and if saving is required, they
    // will be added to m_dccMaterialSaveBuffer
    AZStd::vector<AZStd::string> m_sourceControlBuffer;
    AZStd::mutex m_sourceControlBufferMutex;
    AZStd::atomic_bool m_sourceControlFunctionQueued;

    // Paths of .dccmtl that require saving. These paths have
    // gone through DccMaterialSourceControlCheck() and will
    // be saved by the dcc material save thread
    AZStd::vector<AZStd::string> m_dccMaterialSaveBuffer;
    AZStd::mutex m_dccMaterialSaveMutex;

    // Material highlighting
    _smart_ptr<CMaterial> m_pHighlightMaterial;
    CMaterialHighlighter* m_pHighlighter;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    int m_highlightMask;

    bool m_bHighlightingMaterial;

    // Only begin processing .dccmtl filepaths once editor is ready
    // and we can display appropriate error messages
    bool m_bEditorUiReady;

    // Only report source control error to the user once,
    // no need to spam them for every material
    bool m_bSourceControlErrorReported;

    class CMaterialSender* m_MatSender;

private:
    CMaterial* LoadMaterialInternal(const QString &materialNameClear, const QString &fullSourcePath, const QString &relativeFilePath, bool bMakeIfNotFound);

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    AZ::Data::AssetType m_materialAssetType;
    AZ::Data::AssetType m_dccMaterialAssetType;

    void StartDccMaterialSaveThread();
    void DccMaterialSaveThreadFunc();
    AZStd::thread m_dccMaterialSaveThread;
    AZStd::atomic_bool m_joinThreads;
    AZStd::binary_semaphore m_dccMaterialSaveSemaphore;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

#endif // CRYINCLUDE_EDITOR_MATERIAL_MATERIALMANAGER_H
