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

#include "EditorDefs.h"

#include "MaterialManager.h"

// Qt
#include <QMessageBox>

// AzCore
#include <AzCore/IO/Path/Path.h>

// AzFramework
#include <AzFramework/Render/RenderSystemBus.h>

// AzToolsFramework
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

// Editor
#include "MainWindow.h"
#include "MaterialLibrary.h"
#include "MaterialSender.h"
#include "MaterialUtils.h"
#include "ModelViewport.h"
#include "ISourceControl.h"
#include "UsedResources.h"
#include "Viewport.h"
#include "Commands/CommandManager.h"
#include "Include/IObjectManager.h"
#include "Objects/BaseObject.h"
#include "Objects/SelectionGroup.h"


static const char* MATERIALS_LIBS_PATH = "Materials/";
static unsigned int s_highlightUpdateCounter = 0;

// Convert a material name into a material identifier (no extension, no gamename, etc) so that it can be compared
// in the hash.
static QString UnifyMaterialName(const QString& source)
{
    char tempBuffer[AZ_MAX_PATH_LEN];
    azstrncpy(tempBuffer, AZ_ARRAY_SIZE(tempBuffer), source.toUtf8().data(), AZ_ARRAY_SIZE(tempBuffer) - 1);
    MaterialUtils::UnifyMaterialName(tempBuffer);
    return QString(tempBuffer);
}

struct SHighlightMode
{
    float m_colorHue;
    float m_period;
    bool m_continuous;
};

static SHighlightMode g_highlightModes[] = {
    { 0.70f, 0.8f, true }, // purple
    { 0.25f, 0.75f, false }, // green
    { 0.0, 0.75f, true } // red
};

class CMaterialHighlighter
{
public:
    void Start(CMaterial* pMaterial, int modeFlag);
    void Stop(CMaterial* pMaterial, int modeFlag);
    void GetHighlightColor(ColorF* color, float* intensity, int flags);

    void ClearMaterials() { m_materials.clear(); };
    void RestoreMaterials();
    void Update();
private:
    struct SHighlightOptions
    {
        int m_modeFlags;
    };

    typedef std::map<CMaterial*, SHighlightOptions> Materials;
    Materials m_materials;
};

AZStd::string DccMaterialToSourcePath(const AZStd::string& relativeDccMaterialPath)
{
    AZStd::string fullSourcePath;
    bool sourcePathFound = false;

    // Get source path using relative .dccmtl path
    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(sourcePathFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, relativeDccMaterialPath, fullSourcePath);

    if (sourcePathFound)
    {
        // Set source path extension to ".mtl"
        AzFramework::StringFunc::Path::ReplaceExtension(fullSourcePath, MATERIAL_FILE_EXT);
    }
    return fullSourcePath;
}


void CMaterialHighlighter::Start(CMaterial* pMaterial, int modeFlag)
{
    Materials::iterator it = m_materials.find(pMaterial);
    if (it == m_materials.end())
    {
        SHighlightOptions& options = m_materials[pMaterial];
        options.m_modeFlags = modeFlag;
    }
    else
    {
        SHighlightOptions& options = it->second;
        options.m_modeFlags |= modeFlag;
    }
}

void CMaterialHighlighter::Stop(CMaterial* pMaterial, int modeFlag)
{
    if (pMaterial)
    {
        pMaterial->SetHighlightFlags(0);
    }

    Materials::iterator it = m_materials.find(pMaterial);
    if (it == m_materials.end())
    {
        return;
    }

    SHighlightOptions& options = it->second;
    MAKE_SURE((options.m_modeFlags & modeFlag) != 0, return );

    options.m_modeFlags &= ~modeFlag;
    if (options.m_modeFlags == 0)
    {
        m_materials.erase(it);
    }
}

void CMaterialHighlighter::RestoreMaterials()
{
    for (Materials::iterator it = m_materials.begin(); it != m_materials.end(); ++it)
    {
        if (it->first)
        {
            it->first->SetHighlightFlags(0);
        }
    }
}

void CMaterialHighlighter::Update()
{
    unsigned int counter = s_highlightUpdateCounter;

    Materials::iterator it;
    for (it = m_materials.begin(); it != m_materials.end(); ++it)
    {
        // Only update each material every 4 frames
        if (counter++ % 4 == 0)
        {
            it->first->SetHighlightFlags(it->second.m_modeFlags);
        }
    }

    s_highlightUpdateCounter = (s_highlightUpdateCounter + 1) % 4;
}

void CMaterialHighlighter::GetHighlightColor(ColorF* color, float* intensity, int flags)
{
    MAKE_SURE(color != 0, return );
    MAKE_SURE(intensity != 0, return );

    *intensity = 0.0f;

    if (flags == 0)
    {
        return;
    }

    int flagIndex = 0;
    while (flags)
    {
        if ((flags & 1) != 0)
        {
            break;
        }
        flags = flags >> 1;
        ++flagIndex;
    }

    MAKE_SURE(flagIndex < sizeof(g_highlightModes) / sizeof(g_highlightModes[0]), return );

    const SHighlightMode& mode = g_highlightModes[flagIndex];
    float t = GetTickCount() / 1000.0f;
    float h = mode.m_colorHue;
    float s = 1.0f;
    float v = 1.0f;

    color->fromHSV(h + sinf(t * g_PI2 * 5.0f) * 0.025f, s, v);
    color->a = 1.0f;

    if (mode.m_continuous)
    {
        *intensity = fabsf(sinf(t * g_PI2 / mode.m_period));
    }
    else
    {
        *intensity = max(0.0f, sinf(t * g_PI2 / mode.m_period));
    }
}


//////////////////////////////////////////////////////////////////////////
// CMaterialManager implementation.
//////////////////////////////////////////////////////////////////////////
CMaterialManager::CMaterialManager(CRegistrationContext& regCtx)
    : m_pHighlighter(new CMaterialHighlighter)
    , m_highlightMask(eHighlight_All)
    , m_currentFolder("")
    , m_joinThreads(false)
{
    m_bUniqGuidMap = false;
    m_bUniqNameMap = true;

    m_bEditorUiReady = false;
    m_bSourceControlErrorReported = false;
    m_sourceControlFunctionQueued = false;
    m_pLevelLibrary = (CBaseLibrary*)AddLibrary("Level", true);

    m_MatSender = new CMaterialSender(true);

    EBusFindAssetTypeByName materialResult("Material"); //from MaterialAssetTypeInfo.cpp, case insensitive
    AZ::AssetTypeInfoBus::BroadcastResult(materialResult, &AZ::AssetTypeInfo::GetAssetType);
    m_materialAssetType = materialResult.GetAssetType();

    EBusFindAssetTypeByName dccMaterialResult("DccMaterial"); //from MaterialAssetTypeInfo.cpp, case insensitive
    AZ::AssetTypeInfoBus::BroadcastResult(dccMaterialResult, &AZ::AssetTypeInfo::GetAssetType);
    m_dccMaterialAssetType = dccMaterialResult.GetAssetType();

    RegisterCommands(regCtx);
    AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
    AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationBus::Handler::BusConnect();
    AzFramework::AssetCatalogEventBus::Handler::BusConnect();
    AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
}

//////////////////////////////////////////////////////////////////////////
CMaterialManager::~CMaterialManager()
{
    AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationBus::Handler::BusDisconnect();
    AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
    AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();

    delete m_pHighlighter;
    m_pHighlighter = 0;

    if (gEnv->p3DEngine)
    {
        gEnv->p3DEngine->GetMaterialManager()->SetListener(NULL);
    }

    if (m_MatSender)
    {
        delete m_MatSender;
        m_MatSender = 0;
    }

    // Terminate thread that saves dcc materials.
    m_joinThreads = true;
    if (m_bEditorUiReady)
    {
        m_dccMaterialSaveSemaphore.release();
        m_dccMaterialSaveThread.join();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::Set3DEngine()
{
    if (gEnv->p3DEngine)
    {
        gEnv->p3DEngine->GetMaterialManager()->SetListener(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::ClearAll()
{
    SetCurrentMaterial(NULL);
    CBaseLibraryManager::ClearAll();

    m_pLevelLibrary = (CBaseLibrary*)AddLibrary("Level", true);
}

//////////////////////////////////////////////////////////////////////////
CMaterial* CMaterialManager::CreateMaterial(const QString& sMaterialName,const XmlNodeRef& node, int nMtlFlags, [[maybe_unused]] unsigned long nLoadingFlags)
{
    CMaterial* pMaterial = new CMaterial(sMaterialName, nMtlFlags);

    if (node)
    {
        CBaseLibraryItem::SerializeContext serCtx(node, true);
        serCtx.bUniqName = true;
        pMaterial->Serialize(serCtx);
    }
    if (!pMaterial->IsPureChild() && !(pMaterial->GetFlags() & MTL_FLAG_UIMATERIAL))
    {
        RegisterItem(pMaterial);
    }

    return pMaterial;
}

//////////////////////////////////////////////////////////////////////////
CMaterial* CMaterialManager::CreateMaterial(const char* sMaterialName,const XmlNodeRef& node, int nMtlFlags, unsigned long nLoadingFlags)
{
    return CreateMaterial(QString(sMaterialName), node, nMtlFlags, nLoadingFlags);
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::Export(XmlNodeRef& node)
{
    XmlNodeRef libs = node->newChild("MaterialsLibrary");
    for (int i = 0; i < GetLibraryCount(); i++)
    {
        IDataBaseLibrary* pLib = GetLibrary(i);
        // Level libraries are saved in in level.
        XmlNodeRef libNode = libs->newChild("Library");

        // Export library.
        libNode->setAttr("Name", pLib->GetName().toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
int CMaterialManager::ExportLib(CMaterialLibrary* pLib, XmlNodeRef& libNode)
{
    int num = 0;
    // Export library.
    libNode->setAttr("Name", pLib->GetName().toUtf8().data());
    libNode->setAttr("File", pLib->GetFilename().toUtf8().data());
    char version[50];
    GetIEditor()->GetFileVersion().ToString(version, AZ_ARRAY_SIZE(version));
    libNode->setAttr("SandboxVersion", version);

    // Serialize prototypes.
    for (int j = 0; j < pLib->GetItemCount(); j++)
    {
        CMaterial* pMtl = (CMaterial*)pLib->GetItem(j);

        // Only export real used materials.
        if (pMtl->IsDummy() || !pMtl->IsUsed() || pMtl->IsPureChild())
        {
            continue;
        }

        XmlNodeRef itemNode = libNode->newChild("Material");
        itemNode->setAttr("Name", pMtl->GetName().toUtf8().data());
        num++;
    }
    return num;
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::SetSelectedItem(IDataBaseItem* pItem)
{
    m_pSelectedItem = (CBaseLibraryItem*)pItem;
    SetCurrentMaterial((CMaterial*)pItem);
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::SetCurrentMaterial(CMaterial* pMtl)
{
    if (m_pCurrentMaterial)
    {
        // Changing current material. save old one.
        if (m_pCurrentMaterial->IsModified())
        {
            m_pCurrentMaterial->Save();
        }
    }

    m_pCurrentMaterial = pMtl;
    if (m_pCurrentMaterial)
    {
        m_pCurrentMaterial->OnMakeCurrent();
        m_pCurrentEngineMaterial = m_pCurrentMaterial->GetMatInfo();
    }
    else
    {
        m_pCurrentEngineMaterial = 0;
    }

    m_pSelectedItem = pMtl;
    m_pSelectedParent = pMtl ? pMtl->GetParent() : NULL;

    NotifyItemEvent(m_pCurrentMaterial, EDB_ITEM_EVENT_SELECTED);
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::SetCurrentFolder(const QString& folder)
{
    m_currentFolder = folder;
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::SetMarkedMaterials(const std::vector<_smart_ptr<CMaterial> >& markedMaterials)
{
    m_markedMaterials = markedMaterials;
}

void CMaterialManager::OnLoadShader(CMaterial* pMaterial)
{
    RemoveFromHighlighting(pMaterial, eHighlight_All);
    AddForHighlighting(pMaterial);
}

//////////////////////////////////////////////////////////////////////////
CMaterial* CMaterialManager::GetCurrentMaterial() const
{
    return m_pCurrentMaterial;
}

//////////////////////////////////////////////////////////////////////////
CBaseLibraryItem* CMaterialManager::MakeNewItem()
{
    CMaterial* pMaterial = new CMaterial("", 0);
    return pMaterial;
}
//////////////////////////////////////////////////////////////////////////
CBaseLibrary* CMaterialManager::MakeNewLibrary()
{
    return new CMaterialLibrary(this);
}
//////////////////////////////////////////////////////////////////////////
QString CMaterialManager::GetRootNodeName()
{
    return "MaterialsLibs";
}
//////////////////////////////////////////////////////////////////////////
QString CMaterialManager::GetLibsPath()
{
    if (m_libsPath.isEmpty())
    {
        m_libsPath = MATERIALS_LIBS_PATH;
    }
    return m_libsPath;
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::ReportDuplicateItem(CBaseLibraryItem* pItem, CBaseLibraryItem* pOldItem)
{
    QString sLibName;
    if (pOldItem->GetLibrary())
    {
        sLibName = pOldItem->GetLibrary()->GetName();
    }
    CErrorRecord err;
    err.pItem = (CMaterial*)pOldItem;
    err.error = QObject::tr("Material %1 with the duplicate name to the loaded material %2 ignored").arg(pItem->GetName(), pOldItem->GetName());
    GetIEditor()->GetErrorReport()->ReportError(err);
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::Serialize([[maybe_unused]] XmlNodeRef& node, bool bLoading)
{
    //CBaseLibraryManager::Serialize( node,bLoading );
    if (bLoading)
    {
    }
    else
    {
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    CBaseLibraryManager::OnEditorNotifyEvent(event);
    switch (event)
    {
    case eNotify_OnInit:
        InitMatSender();
        break;
    case eNotify_OnIdleUpdate:
        m_pHighlighter->Update();
        break;
    case eNotify_OnBeginGameMode:
        m_pHighlighter->RestoreMaterials();
        break;
    case eNotify_OnEndGameMode:
        ReloadDirtyMaterials();
        break;
    case eNotify_OnBeginNewScene:
        SetCurrentMaterial(0);
        break;
    case eNotify_OnBeginSceneOpen:
        SetCurrentMaterial(0);
        break;
    case eNotify_OnMissionChange:
        SetCurrentMaterial(0);
        break;
    case eNotify_OnCloseScene:
        SetCurrentMaterial(0);
        m_pHighlighter->ClearMaterials();
        break;
    case eNotify_OnQuit:
        SetCurrentMaterial(0);
        if (gEnv->p3DEngine)
        {
            gEnv->p3DEngine->GetMaterialManager()->SetListener(NULL);
        }
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::ReloadDirtyMaterials()
{
    if (!GetIEditor()->Get3DEngine())
    {
        return;
    }

    IMaterialManager* runtimeMaterialManager = GetIEditor()->Get3DEngine()->GetMaterialManager();

    uint32 mtlCount = 0;

    runtimeMaterialManager->GetLoadedMaterials(NULL, mtlCount);

    if (mtlCount > 0)
    {
        AZStd::vector<_smart_ptr<IMaterial>> allMaterials;

        allMaterials.reserve(mtlCount);

        [[maybe_unused]] uint32 mtlCountPrev = mtlCount;
        runtimeMaterialManager->GetLoadedMaterials(&allMaterials, mtlCount);
        AZ_Assert(mtlCountPrev == mtlCount && mtlCount == allMaterials.size(), "It appears GetLoadedMaterials was not used correctly.");

        for (size_t i = 0; i < mtlCount; ++i)
        {
            _smart_ptr<IMaterial> pMtl = allMaterials[i];
            if (pMtl && pMtl->IsDirty())
            {
                runtimeMaterialManager->ReloadMaterial(pMtl);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CMaterial* CMaterialManager::LoadMaterial(const QString& sMaterialName, bool bMakeIfNotFound)
{
    LOADING_TIME_PROFILE_SECTION(GetISystem());

    QString sMaterialNameClear = UnifyMaterialName(sMaterialName);
    QString fullSourcePath = MaterialToFilename(sMaterialNameClear);
    QString relativePath = PathUtil::ReplaceExtension(sMaterialNameClear.toUtf8().data(), MATERIAL_FILE_EXT).c_str();

    return LoadMaterialInternal(sMaterialNameClear, fullSourcePath, relativePath, bMakeIfNotFound);
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CMaterialManager::LoadXmlNode(const QString &fullSourcePath, const QString &relativeFilePath)
{
    XmlNodeRef materialNode = GetISystem()->LoadXmlFromFile(fullSourcePath.toUtf8().data());
    if (!materialNode)
    {
        // try again with the product file in case its present
        materialNode = GetISystem()->LoadXmlFromFile(relativeFilePath.toUtf8().data());
    }
    return materialNode;
}

//////////////////////////////////////////////////////////////////////////
CMaterial* CMaterialManager::LoadMaterialWithFullSourcePath(const QString& relativeFilePath, const QString& fullSourcePath, bool makeIfNotFound /*= true*/)
{
    QString materialNameClear = UnifyMaterialName(relativeFilePath);
    return LoadMaterialInternal(materialNameClear, fullSourcePath, relativeFilePath, makeIfNotFound);
}

//////////////////////////////////////////////////////////////////////////
CMaterial* CMaterialManager::LoadMaterialInternal(const QString &materialNameClear, const QString &fullSourcePath, const QString &relativeFilePath, bool makeIfNotFound)
{
    // Note:  We are loading from source files here, not from compiled assets, so there is no need to query the asset system for compilation status, etc.

    // Load material with this name if not yet loaded.
    CMaterial* pMaterial = (CMaterial*)FindItemByName(materialNameClear);
    if (pMaterial)
    {
        // If this is a dummy material that was created before for not found mtl file
        // try reload the mtl file again to get valid material data.
        if (pMaterial->IsDummy())
        {
            XmlNodeRef mtlNode = GetISystem()->LoadXmlFromFile(fullSourcePath.toUtf8().data());
            if (mtlNode)
            {
                DeleteMaterial(pMaterial);
                pMaterial = CreateMaterial(materialNameClear, mtlNode);
            }
        }
        return pMaterial;
    }

    XmlNodeRef mtlNode = LoadXmlNode(fullSourcePath, relativeFilePath);

    if (mtlNode)
    {
        pMaterial = CreateMaterial(materialNameClear, mtlNode);
    }
    else
    {
        if (makeIfNotFound)
        {
            pMaterial = new CMaterial(materialNameClear);
            pMaterial->SetDummy(true);
            RegisterItem(pMaterial);

            CErrorRecord err;
            err.error = QObject::tr("Material %1 not found").arg(materialNameClear);
            GetIEditor()->GetErrorReport()->ReportError(err);
        }
    }

    return pMaterial;
}

//////////////////////////////////////////////////////////////////////////
CMaterial* CMaterialManager::LoadMaterial(const char* sMaterialName, bool bMakeIfNotFound)
{
    return LoadMaterial(QString(sMaterialName), bMakeIfNotFound);
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::AddSourceFileOpeners(const char* fullSourceFileName, [[maybe_unused]] const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers)
{
    using namespace AzToolsFramework;
    using namespace AzToolsFramework::AssetBrowser;

    // disable if other project is active
    if (AzFramework::Render::RenderSystemRequestBus::HasHandlers())
    {
        return;
    }

    if (AZStd::wildcard_match("*.mtl", fullSourceFileName))
    {
        // we can handle these!
        auto materialCallback = [this](const char* fullSourceFileNameInCall, const AZ::Uuid& sourceUUIDInCall)
        {
            const SourceAssetBrowserEntry* fullDetails = SourceAssetBrowserEntry::GetSourceByUuid(sourceUUIDInCall);
            if (fullDetails)
            {
                CMaterial* materialFile = LoadMaterialWithFullSourcePath(QString::fromUtf8(fullDetails->GetRelativePath().c_str()), QString::fromUtf8(fullSourceFileNameInCall), false);
                if (materialFile)
                {
                    OpenViewPane("Material Editor");
                    SetCurrentMaterial(materialFile); // the material browser pane should be able to deal with this.
                }
            }
        };

        openers.push_back({ "O3DE_MaterialEditor", "Open In Material Editor...", QIcon(), materialCallback });
    }
}

//////////////////////////////////////////////////////////////////////////
static bool MaterialRequiresSurfaceType(CMaterial* pMaterial)
{
    // Do not enforce Surface Type...

    // ...over editor UI materials
    if ((pMaterial->GetFlags() & MTL_FLAG_UIMATERIAL) != 0)
    {
        return false;
    }

    // ...over SKY
    if (pMaterial->GetShaderName() == "DistanceCloud" ||
        pMaterial->GetShaderName() == "Sky" ||
        pMaterial->GetShaderName() == "SkyHDR")
    {
        return false;
    }
    // ...over terrain materials
    if (pMaterial->GetShaderName() == "Terrain.Layer")
    {
        return false;
    }
    // ...over vegetation
    if (pMaterial->GetShaderName() == "Vegetation")
    {
        return false;
    }

    // ...over decals
    bool requiresSurfaceType = true;
    CVarBlock* pShaderGenParams = pMaterial->GetShaderGenParamsVars();
    if (pShaderGenParams)
    {
        if (IVariable* pVar = pShaderGenParams->FindVariable("Decal"))
        {
            int value = 0;
            pVar->Get(value);
            if (value)
            {
                requiresSurfaceType = false;
            }
        }
        // The function GetShaderGenParamsVars allocates a new CVarBlock object, so let's clean it up here
        delete pShaderGenParams;
    }
    return requiresSurfaceType;
}

//////////////////////////////////////////////////////////////////////////
int CMaterialManager::GetHighlightFlags(CMaterial* pMaterial) const
{
    if (pMaterial == NULL)
    {
        return 0;
    }

    if ((pMaterial->GetFlags() & MTL_FLAG_NODRAW) != 0)
    {
        return 0;
    }

    int result = 0;

    if (pMaterial == m_pHighlightMaterial)
    {
        result |= eHighlight_Pick;
    }

    const QString& surfaceTypeName = pMaterial->GetSurfaceTypeName();
    if (surfaceTypeName.isEmpty() && MaterialRequiresSurfaceType(pMaterial))
    {
        result |= eHighlight_NoSurfaceType;
    }

    if (GetIEditor()->Get3DEngine())
    {
        if (ISurfaceTypeManager* pSurfaceManager = GetIEditor()->Get3DEngine()->GetMaterialManager()->GetSurfaceTypeManager())
        {
            const ISurfaceType* pSurfaceType = pSurfaceManager->GetSurfaceTypeByName(surfaceTypeName.toUtf8().data());
            if (pSurfaceType && pSurfaceType->GetBreakability() != 0)
            {
                result |= eHighlight_Breakable;
            }
        }
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::AddForHighlighting(CMaterial* pMaterial)
{
    if (pMaterial == NULL)
    {
        return;
    }

    int highlightFlags = (GetHighlightFlags(pMaterial) & m_highlightMask);
    if (highlightFlags != 0)
    {
        m_pHighlighter->Start(pMaterial, highlightFlags);
    }

    int count = pMaterial->GetSubMaterialCount();
    for (int i = 0; i < count; ++i)
    {
        CMaterial* pChild = pMaterial->GetSubMaterial(i);
        if (!pChild)
        {
            continue;
        }

        AddForHighlighting(pChild);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::RemoveFromHighlighting(CMaterial* pMaterial, int mask)
{
    if (pMaterial == NULL)
    {
        return;
    }

    m_pHighlighter->Stop(pMaterial, mask);

    int count = pMaterial->GetSubMaterialCount();
    for (int i = 0; i < count; ++i)
    {
        CMaterial* pChild = pMaterial->GetSubMaterial(i);
        if (!pChild)
        {
            continue;
        }

        RemoveFromHighlighting(pChild, mask);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::UpdateHighlightedMaterials()
{
    IDataBaseItemEnumerator* pEnum = CBaseLibraryManager::GetItemEnumerator();
    if (!pEnum)
    {
        return;
    }

    CMaterial* pMaterial = (CMaterial*)pEnum->GetFirst();
    while (pMaterial)
    {
        RemoveFromHighlighting(pMaterial, eHighlight_All);
        AddForHighlighting(pMaterial);
        pMaterial =  (CMaterial*)pEnum->GetNext();
    }

    pEnum->Release();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::OnRequestMaterial(_smart_ptr<IMaterial> pMatInfo)
{
    const char* pcName = pMatInfo->GetName();
    CMaterial* pMaterial = (CMaterial*) pMatInfo->GetUserData();

    if (!pMaterial && pcName && *pcName)
    {
        pMaterial = LoadMaterial(pcName, false);
    }

    if (pMaterial)
    {
        _smart_ptr<IMaterial> pNewMatInfo = pMaterial->GetMatInfo(true);
        assert(pNewMatInfo == pMatInfo);
        //Only register if the material is not registered
        if (!pMaterial->IsRegistered())
        {
            RegisterItem(pMaterial);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::OnCreateMaterial(_smart_ptr<IMaterial> pMatInfo)
{
    CMaterial* existingMaterial = static_cast<CMaterial*>(FindItemByName(UnifyMaterialName(pMatInfo->GetName())));
    bool materialAlreadyExists = existingMaterial != nullptr;

    // If its not a sub-material or a UI material
    if (!(pMatInfo->GetFlags() & MTL_FLAG_PURE_CHILD) && !(pMatInfo->GetFlags() & MTL_FLAG_UIMATERIAL))
    {
        // Create a new editor material if it doesn't exist
        if (!materialAlreadyExists)
        {
            CMaterial* pMaterial = new CMaterial(pMatInfo->GetName());
            pMaterial->SetFromMatInfo(pMatInfo);
            RegisterItem(pMaterial);

            AddForHighlighting(pMaterial);
        }
        else
        {
            // If the material already exists, re-set its values from the engine material that was just re-loaded
            existingMaterial->SetFromMatInfo(pMatInfo);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::OnDeleteMaterial(_smart_ptr<IMaterial> pMaterial)
{
    CMaterial* pMtl = (CMaterial*)pMaterial->GetUserData();
    if (pMtl)
    {
        RemoveFromHighlighting(pMtl, eHighlight_All);
        DeleteMaterial(pMtl);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CMaterialManager::IsCurrentMaterial(_smart_ptr<IMaterial> pMaterial) const
{
    if (!pMaterial)
    {
        return false;
    }

    CMaterial* pMtl = static_cast<CMaterial*>(pMaterial->GetUserData());
    bool currentMaterial = (pMtl == m_pCurrentMaterial);

    if (pMtl->GetParent())
    {
        currentMaterial |= (pMtl->GetParent() == m_pCurrentMaterial);
    }

    for (size_t subMatIdx = 0; subMatIdx < pMtl->GetMatInfo()->GetSubMtlCount(); ++subMatIdx)
    {
        if (static_cast<CMaterial*>(pMtl->GetMatInfo()->GetSubMtl(subMatIdx)->GetUserData()) == m_pCurrentMaterial)
        {
            currentMaterial = true;
            break;
        }
    }

    return currentMaterial;
}


//////////////////////////////////////////////////////////////////////////
CMaterial* CMaterialManager::FromIMaterial(_smart_ptr<IMaterial> engineMaterial)
{
    if (!engineMaterial)
    {
        return nullptr;
    }
    CMaterial* editorMaterial = (CMaterial*)engineMaterial->GetUserData();
    if (!editorMaterial)
    {
        // If the user data isn't set, check for an existing material with the same name
        editorMaterial = static_cast<CMaterial*>(FindItemByName(UnifyMaterialName(engineMaterial->GetName())));
    }
    return editorMaterial;
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::SaveAllLibs()
{
}

//////////////////////////////////////////////////////////////////////////
QString CMaterialManager::FilenameToMaterial(const QString& filename)
{
    // Convert a full or relative path to a normalized name that can be used in a hash (so lowercase, relative path, correct slashes, remove extension)
    // note that it may already be an asset path, if so, don't add the overhead of calling into the AP and convert it.
    // if it starts with an alias (@) or if its an absolute file path, we need to convert it.  Otherwise we really don't...
    QString name = filename;
    if (name.startsWith(QChar('@')) || AZ::IO::PathView(name.toUtf8().data()).IsAbsolute())
    {
        name = Path::FullPathToGamePath(filename); // convert any full path to a relative path instead.
    }
    QByteArray n = name.toUtf8();
    MaterialUtils::UnifyMaterialName(n.data());  // Utility function used by all other parts of the code to unify slashes, lowercase, and remove extension

    return QString::fromUtf8(n);
}

//////////////////////////////////////////////////////////////////////////
QString CMaterialManager::MaterialToFilename(const QString& sMaterialName)
{
    QString materialWithExtension = Path::ReplaceExtension(sMaterialName, MATERIAL_FILE_EXT);
    QString fileName = Path::GamePathToFullPath(materialWithExtension);
    const int mtlExtensionLength = strlen(MATERIAL_FILE_EXT);
    if (fileName.right(mtlExtensionLength).toLower() != MATERIAL_FILE_EXT)
    {
        // we got something back which is not a mtl, fall back heuristic:
        AZStd::string pathName(fileName.toUtf8().data());
        AZStd::string fileNameOfMaterial;
        AzFramework::StringFunc::Path::StripFullName(pathName); // remove the filename of the path to the FBX file so now it just contains the folder of the fbx file.
        AzFramework::StringFunc::Path::GetFullFileName(materialWithExtension.toUtf8().data(), fileNameOfMaterial); // remove the path part of the material so it only contains the file name
        AZStd::string finalName;
        AzFramework::StringFunc::Path::Join(pathName.c_str(), fileNameOfMaterial.c_str(), finalName);
        fileName = finalName.c_str();
    }
    return fileName;
}

//////////////////////////////////////////////////////////////////////////
const AZ::Data::AssetType& CMaterialManager::GetMaterialAssetType()
{
    return m_materialAssetType;
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::DeleteMaterial(CMaterial* pMtl)
{
    assert(pMtl);
    _smart_ptr<CMaterial> _ref(pMtl);
    if (pMtl == GetCurrentMaterial())
    {
        SetCurrentMaterial(NULL);
    }

    DeleteItem(pMtl);

    // Delete it from all sub materials.
    for (int i = 0; i < m_pLevelLibrary->GetItemCount(); i++)
    {
        CMaterial* pMultiMtl = (CMaterial*)m_pLevelLibrary->GetItem(i);
        if (pMultiMtl->IsMultiSubMaterial())
        {
            for (int slot = 0; slot < pMultiMtl->GetSubMaterialCount(); slot++)
            {
                if (pMultiMtl->GetSubMaterial(slot) == pMultiMtl)
                {
                    // Clear this sub material slot.
                    pMultiMtl->SetSubMaterial(slot, 0);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::RemoveMaterialFromDisk(const char * fileName)
{
    using namespace AzToolsFramework;
    if (fileName)
    {
        SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::RequestDelete, fileName,
            [](bool success, const SourceControlFileInfo& info)
        {
            //If the file is not managed by source control, delete it locally
            if (!success && !info.IsManaged())
            {
                QFile::remove(info.m_filePath.c_str());
            }
        }
        );
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::RegisterCommands(CRegistrationContext& regCtx)
{
    CommandManagerHelper::RegisterCommand(regCtx.pCommandManager, "material", "duplicate", "", "", AZStd::bind(&CMaterialManager::Command_Duplicate, this));
    CommandManagerHelper::RegisterCommand(regCtx.pCommandManager, "material", "merge", "", "", AZStd::bind(&CMaterialManager::Command_Merge, this));
    CommandManagerHelper::RegisterCommand(regCtx.pCommandManager, "material", "delete", "", "", AZStd::bind(&CMaterialManager::Command_Delete, this));
    CommandManagerHelper::RegisterCommand(regCtx.pCommandManager, "material", "assign_to_selection", "", "", AZStd::bind(&CMaterialManager::Command_AssignToSelection, this));
    CommandManagerHelper::RegisterCommand(regCtx.pCommandManager, "material", "select_assigned_objects", "", "", AZStd::bind(&CMaterialManager::Command_SelectAssignedObjects, this));
    CommandManagerHelper::RegisterCommand(regCtx.pCommandManager, "material", "select_from_object", "", "", AZStd::bind(&CMaterialManager::Command_SelectFromObject, this));
}

//////////////////////////////////////////////////////////////////////////
bool CMaterialManager::SelectSaveMaterial(QString& itemName, QString& fullSourcePath, const char* defaultStartPath)
{
    QString startPath;
    if (defaultStartPath && defaultStartPath[0] != '\0')
    {
        startPath = defaultStartPath;
    }
    else
    {
        startPath = GetIEditor()->GetSearchPath(EDITOR_PATH_MATERIALS);
    }

    if (!CFileUtil::SelectSaveFile("Material Files (*.mtl)", "mtl", startPath, fullSourcePath))
    {
        return false;
    }

    itemName = FilenameToMaterial(fullSourcePath);
    if (itemName.isEmpty())
    {
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
CMaterial* CMaterialManager::SelectNewMaterial(int nMtlFlags, [[maybe_unused]] const char* sStartPath)
{
    QString path = m_pCurrentMaterial ? Path::GetPath(m_pCurrentMaterial->GetFilename()) : m_currentFolder;
    QString itemName;
    QString fullPath;
    if (!SelectSaveMaterial(itemName, fullPath, path.toUtf8().data()))
    {
        return 0;
    }

    if (FindItemByName(itemName))
    {
        Warning("Material with name %s already exist", itemName.toUtf8().data());
        return 0;
    }

    _smart_ptr<CMaterial> mtl = CreateMaterial(itemName, XmlNodeRef(), nMtlFlags);
    mtl->Update();
    bool skipReadOnly = true;
    mtl->Save(skipReadOnly, fullPath);
    SetCurrentMaterial(mtl);
    return mtl;
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::Command_Create()
{
    SelectNewMaterial(0);
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::Command_CreateMulti()
{
    SelectNewMaterial(MTL_FLAG_MULTI_SUBMTL);
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::Command_ConvertToMulti()
{
    CMaterial* pMaterial = GetCurrentMaterial();

    if (pMaterial && pMaterial->GetSubMaterialCount() == 0)
    {
        CMaterial* pSubMat = new CMaterial(*pMaterial);
        pSubMat->SetName(pSubMat->GetShortName());
        pSubMat->SetFlags(pSubMat->GetFlags() | MTL_FLAG_PURE_CHILD);

        pMaterial->SetFlags(MTL_FLAG_MULTI_SUBMTL);
        pMaterial->SetSubMaterialCount(1);
        pMaterial->SetSubMaterial(0, pSubMat);

        pMaterial->Save();
        pMaterial->Reload();
        SetSelectedItem(pSubMat);
    }
    else
    {
        Warning(pMaterial ? "azlmbr.legacy.material.convert_to_multi called on invalid material setup" : "azlmbr.legacy.material.convert_to_multi called while no material selected");
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::Command_Duplicate()
{
    CMaterial* pSrcMtl = GetCurrentMaterial();

    if (!pSrcMtl)
    {
        CErrorRecord err;
        err.error = "azlmbr.legacy.material.duplicate called while no materials selected";
        GetIEditor()->GetErrorReport()->ReportError(err);
        return;
    }

    if (GetIEditor()->IsSourceControlAvailable())
    {
        uint32 attrib = pSrcMtl->GetFileAttributes();

        if ((attrib & SCC_FILE_ATTRIBUTE_INPAK) &&  (attrib & SCC_FILE_ATTRIBUTE_MANAGED) && !(attrib & SCC_FILE_ATTRIBUTE_NORMAL))
        {
            // Get latest for making folders with right case
            CFileUtil::GetLatestFromSourceControl(pSrcMtl->GetFilename().toUtf8().data());
        }
    }

    if (pSrcMtl != 0 && !pSrcMtl->IsPureChild())
    {
        QString newUniqueRelativePath = MakeUniqueItemName(pSrcMtl->GetName());

        // Create a new material.
        _smart_ptr<CMaterial> pMtl = DuplicateMaterial(newUniqueRelativePath.toUtf8().data(), pSrcMtl);
        if (pMtl)
        {
            // Get the new filename from the relative path
            AZStd::string newFileName;
            AzFramework::StringFunc::Path::GetFileName(newUniqueRelativePath.toUtf8().data(), newFileName);

            // Get the full path to the original material, so we know which folder to put the new material in
            AZStd::string newFullFilePath = pSrcMtl->GetFilename().toUtf8().data();

            // Replace the original material filename with the filename from the new relative path + the material file extension to get the new full file path
            AzFramework::StringFunc::Path::ReplaceFullName(newFullFilePath, newFileName.c_str(), MATERIAL_FILE_EXT);

            AzFramework::StringFunc::Path::Normalize(newFullFilePath);

            const bool skipReadOnly = true;
            pMtl->Save(skipReadOnly, newFullFilePath.c_str());
            SetSelectedItem(pMtl);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CMaterial* CMaterialManager::DuplicateMaterial(const char* newName, CMaterial* pOriginal)
{
    if (!newName)
    {
        assert(0 && "NULL newName passed into CMaterialManager::DuplicateMaterial");
        return 0;
    }
    if (!pOriginal)
    {
        assert(0 && "NULL pOriginal passed into CMaterialManager::DuplicateMaterial");
        return 0;
    }


    XmlNodeRef node = GetISystem()->CreateXmlNode("Material");
    CBaseLibraryItem::SerializeContext ctx(node, false);
    ctx.bCopyPaste = true;
    pOriginal->Serialize(ctx);

    return CreateMaterial(newName, node, pOriginal->GetFlags());
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::GenerateUniqueSubmaterialName(const CMaterial* pSourceMaterial, const CMaterial* pTargetMaterial, QString& uniqueSubmaterialName) const
{
    QString sourceMaterialName = pSourceMaterial->GetName();

    // We don't need the whole path to the material, just the base name
    QFileInfo filename(sourceMaterialName);
    sourceMaterialName = filename.baseName();

    uniqueSubmaterialName = sourceMaterialName;
    size_t nameIndex = 0;

    bool nameUpdated = true;
    while (nameUpdated)
    {
        nameUpdated = false;
        for (size_t k = 0; k < pTargetMaterial->GetSubMaterialCount(); ++k)
        {
            CMaterial* pSubMaterial = pTargetMaterial->GetSubMaterial(k);
            if (pSubMaterial && pSubMaterial->GetName() == uniqueSubmaterialName)
            {
                ++nameIndex;
                uniqueSubmaterialName = QStringLiteral("%1%2").arg(sourceMaterialName).arg(nameIndex, 2, 10, QLatin1Char('0'));
                nameUpdated = true;
                break;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CMaterialManager::DuplicateAsSubMaterialAtIndex(CMaterial* pSourceMaterial, CMaterial* pTargetMaterial, int subMaterialIndex)
{
    if (pSourceMaterial && pTargetMaterial && pTargetMaterial->GetSubMaterialCount() > subMaterialIndex)
    {
        // Resolve name collisions between the source material and the submaterials in the target material
        QString newSubMaterialName;
        GenerateUniqueSubmaterialName(pSourceMaterial, pTargetMaterial, newSubMaterialName);

        // Mark the material to be duplicated as a PURE_CHILD since it is being duplicated as a submaterial
        int sourceMaterialFlags = pSourceMaterial->GetFlags();
        pSourceMaterial->SetFlags(sourceMaterialFlags | MTL_FLAG_PURE_CHILD);

        CMaterial* pNewSubMaterial = DuplicateMaterial(newSubMaterialName.toUtf8().data(), pSourceMaterial);
        pTargetMaterial->SetSubMaterial(subMaterialIndex, pNewSubMaterial);

        // Reset the flags of the source material to their original values
        pSourceMaterial->SetFlags(sourceMaterialFlags);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::Command_Merge()
{
    QString itemName;
    QString fullPath;
    QString defaultMaterialPath;
    if (m_pCurrentMaterial)
    {
        defaultMaterialPath = Path::GetPath(m_pCurrentMaterial->GetFilename());
    }
    if (!SelectSaveMaterial(itemName, fullPath, defaultMaterialPath.toUtf8().data()))
    {
        return;
    }

    _smart_ptr<CMaterial> pNewMaterial = CreateMaterial(itemName, XmlNodeRef(), MTL_FLAG_MULTI_SUBMTL);

    size_t totalSubMaterialCount = 0;
    for (_smart_ptr<CMaterial> pMaterial : m_markedMaterials)
    {
        if (pMaterial->IsMultiSubMaterial())
        {
            totalSubMaterialCount += pMaterial->GetSubMaterialCount();
        }
        else
        {
            totalSubMaterialCount++;
        }
    }
    pNewMaterial->SetSubMaterialCount(totalSubMaterialCount);

    size_t subMaterialIndex = 0;
    for (_smart_ptr<CMaterial> pMaterial : m_markedMaterials)
    {
        if (pMaterial->IsMultiSubMaterial())
        {
            // Loop through each submaterial and duplicate it as a submaterial in the new material
            for (size_t j = 0; j < pMaterial->GetSubMaterialCount(); ++j)
            {
                CMaterial* pSubMaterial = pMaterial->GetSubMaterial(j);
                if (DuplicateAsSubMaterialAtIndex(pSubMaterial, pNewMaterial, subMaterialIndex))
                {
                    ++subMaterialIndex;
                }
            }
        }
        else
        {
            // Duplicate the material as a submaterial in the new material
            if (DuplicateAsSubMaterialAtIndex(pMaterial, pNewMaterial, subMaterialIndex))
            {
                ++subMaterialIndex;
            }
        }
    }

    pNewMaterial->Update();
    const bool skipReadOnly = true;
    pNewMaterial->Save(skipReadOnly, fullPath);
    SetCurrentMaterial(pNewMaterial);
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::Command_Delete()
{
    CMaterial* pMtl = GetCurrentMaterial();
    if (pMtl)
    {
        CUndo undo("Delete Material");
        QString str = QObject::tr("Delete Material %1?\r\nNote: Material file %2 will also be deleted.")
            .arg(pMtl->GetName(), pMtl->GetFilename());
        if (QMessageBox::question(QApplication::activeWindow(), QObject::tr("Delete Confirmation"), str) == QMessageBox::Yes)
        {
            AZStd::string matName = pMtl->GetFilename().toUtf8().data();
            DeleteMaterial(pMtl);
            RemoveMaterialFromDisk(matName.c_str());
            SetCurrentMaterial(0);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::Command_AssignToSelection()
{
    CMaterial* pMtl = GetCurrentMaterial();
    if (pMtl)
    {
        CUndo undo("Assign Material");
        CSelectionGroup* pSel = GetIEditor()->GetSelection();
        if (pMtl->IsPureChild())
        {
            const QString title = QObject::tr("Assign Submaterial");
            const QString message = QObject::tr("You can assign submaterials to objects only for preview purpose. This assignment will not be saved with the level and will not be exported to the game.");
            if (QMessageBox::information(QApplication::activeWindow(), title, message, QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel)
            {
                return;
            }
        }
        if (!pSel->IsEmpty())
        {
            for (int i = 0; i < pSel->GetCount(); i++)
            {
                pSel->GetObject(i)->SetMaterial(pMtl);
            }
        }
    }
    CViewport* pViewport = GetIEditor()->GetActiveView();
    if (pViewport)
    {
        pViewport->Drop(QPoint(-1, -1), pMtl);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::Command_ResetSelection()
{
    CSelectionGroup* pSel = GetIEditor()->GetSelection();
    if (!pSel->IsEmpty())
    {
        CUndo undo("Reset Material");
        for (int i = 0; i < pSel->GetCount(); i++)
        {
            pSel->GetObject(i)->SetMaterial(0);
        }
    }
    CViewport* pViewport = GetIEditor()->GetActiveView();
    if (pViewport)
    {
        pViewport->Drop(QPoint(-1, -1), 0);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::Command_SelectAssignedObjects()
{
    CMaterial* pMtl = GetCurrentMaterial();
    if (pMtl)
    {
        CUndo undo("Select Object(s)");
        CBaseObjectsArray objects;
        GetIEditor()->GetObjectManager()->GetObjects(objects);
        for (int i = 0; i < objects.size(); i++)
        {
            CBaseObject* pObject = objects[i];
            if (pObject->GetMaterial() == pMtl || pObject->GetRenderMaterial() == pMtl)
            {
                if (pObject->IsHidden() || pObject->IsFrozen())
                {
                    continue;
                }
                GetIEditor()->GetObjectManager()->SelectObject(pObject);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::Command_SelectFromObject()
{
    if (GetIEditor()->IsInPreviewMode())
    {
        CViewport* pViewport = GetIEditor()->GetActiveView();
        if (CModelViewport* p = viewport_cast<CModelViewport*>(pViewport))
        {
            CMaterial* pMtl = p->GetMaterial();
            SetCurrentMaterial(pMtl);
        }
        return;
    }

    CSelectionGroup* pSel = GetIEditor()->GetSelection();
    if (pSel->IsEmpty())
    {
        return;
    }

    for (int i = 0; i < pSel->GetCount(); i++)
    {
        CMaterial* pMtl = pSel->GetObject(i)->GetRenderMaterial();
        if (pMtl)
        {
            SetCurrentMaterial(pMtl);
            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::PickPreviewMaterial()
{
    XmlNodeRef data = XmlHelpers::CreateXmlNode("ExportMaterial");
    CMaterial* pMtl = GetCurrentMaterial();
    if (!pMtl)
    {
        return;
    }

    if (pMtl->IsPureChild() && pMtl->GetParent())
    {
        pMtl = pMtl->GetParent();
    }

    if (pMtl->GetFlags() & MTL_FLAG_WIRE)
    {
        data->setAttr("Flag_Wire", 1);
    }
    if (pMtl->GetFlags() & MTL_FLAG_2SIDED)
    {
        data->setAttr("Flag_2Sided", 1);
    }

    data->setAttr("Name", pMtl->GetName().toUtf8().data());
    data->setAttr("FileName", pMtl->GetFilename().toUtf8().data());

    XmlNodeRef node = data->newChild("Material");

    CBaseLibraryItem::SerializeContext serCtx(node, false);
    pMtl->Serialize(serCtx);


    if (!pMtl->IsMultiSubMaterial())
    {
        XmlNodeRef texturesNode = node->findChild("Textures");
        if (texturesNode)
        {
            for (int i = 0; i < texturesNode->getChildCount(); i++)
            {
                XmlNodeRef texNode = texturesNode->getChild(i);
                QString file;
                if (texNode->getAttr("File", file))
                {
                    texNode->setAttr("File", Path::GamePathToFullPath(file).toUtf8().data());
                }
            }
        }
    }
    else
    {
        XmlNodeRef childsNode = node->findChild("SubMaterials");
        if (childsNode)
        {
            int nSubMtls = childsNode->getChildCount();
            for (int i = 0; i < nSubMtls; i++)
            {
                XmlNodeRef node2 = childsNode->getChild(i);
                XmlNodeRef texturesNode = node2->findChild("Textures");
                if (texturesNode)
                {
                    for (int ii = 0; ii < texturesNode->getChildCount(); ii++)
                    {
                        XmlNodeRef texNode = texturesNode->getChild(ii);
                        QString file;
                        if (texNode->getAttr("File", file))
                        {
                            texNode->setAttr("File", Path::GamePathToFullPath(file).toUtf8().data());
                        }
                    }
                }
            }
        }
    }


    m_MatSender->SendMessage(eMSM_GetSelectedMaterial, data);
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::SyncMaterialEditor()
{
#if defined(AZ_PLATFORM_WINDOWS)
    if (!m_MatSender)
    {
        return;
    }

    if (!m_MatSender->GetMessage())
    {
        return;
    }

    if (m_MatSender->m_h.msg == eMSM_Create)
    {
        XmlNodeRef node = m_MatSender->m_node->findChild("Material");
        if (!node)
        {
            return;
        }

        QString sMtlName;
        QString sMaxFile;

        XmlNodeRef root = m_MatSender->m_node;
        root->getAttr("Name", sMtlName);
        root->getAttr("MaxFile", sMaxFile);

        int IsMulti = 0;
        root->getAttr("IsMulti", IsMulti);

        int nMtlFlags = 0;
        if (IsMulti)
        {
            nMtlFlags |= MTL_FLAG_MULTI_SUBMTL;
        }

        if (root->haveAttr("Flag_Wire"))
        {
            nMtlFlags |= MTL_FLAG_WIRE;
        }
        if (root->haveAttr("Flag_2Sided"))
        {
            nMtlFlags |= MTL_FLAG_2SIDED;
        }

        _smart_ptr<CMaterial> pMtl = SelectNewMaterial(nMtlFlags, Path::GetPath(sMaxFile).toUtf8().data());

        if (!pMtl)
        {
            return;
        }

        if (!IsMulti)
        {
            node->delAttr("Shader");   // Remove shader attribute.
            XmlNodeRef texturesNode = node->findChild("Textures");
            if (texturesNode)
            {
                for (int i = 0; i < texturesNode->getChildCount(); i++)
                {
                    XmlNodeRef texNode = texturesNode->getChild(i);
                    QString file;
                    if (texNode->getAttr("File", file))
                    {
                        //make path relative to the project specific game folder
                        QString newfile = Path::MakeGamePath(file);
                        if (!newfile.isEmpty())
                        {
                            file = newfile;
                        }
                        texNode->setAttr("File", file.toUtf8().data());
                    }
                }
            }
        }
        else
        {
            XmlNodeRef childsNode = node->findChild("SubMaterials");
            if (childsNode)
            {
                int nSubMtls = childsNode->getChildCount();
                for (int i = 0; i < nSubMtls; i++)
                {
                    XmlNodeRef node2 = childsNode->getChild(i);
                    node2->delAttr("Shader");   // Remove shader attribute.
                    XmlNodeRef texturesNode = node2->findChild("Textures");
                    if (texturesNode)
                    {
                        for (int ii = 0; ii < texturesNode->getChildCount(); ii++)
                        {
                            XmlNodeRef texNode = texturesNode->getChild(ii);
                            QString file;
                            if (texNode->getAttr("File", file))
                            {
                                //make path relative to the project specific game folder
                                QString newfile = Path::MakeGamePath(file);
                                if (!newfile.isEmpty())
                                {
                                    file = newfile;
                                }
                                texNode->setAttr("File", file.toUtf8().data());
                            }
                        }
                    }
                }
            }
        }

        CBaseLibraryItem::SerializeContext ctx(node, true);
        ctx.bUndo = true;
        pMtl->Serialize(ctx);

        pMtl->Update();

        SetCurrentMaterial(0);
        SetCurrentMaterial(pMtl);
    }

    if (m_MatSender->m_h.msg == eMSM_GetSelectedMaterial)
    {
        PickPreviewMaterial();
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::InitMatSender()
{
    //MatSend(true);
    m_MatSender->Create();
    QWidget* mainWindow = MainWindow::instance();
    m_MatSender->SetupWindows(mainWindow, mainWindow);
    XmlNodeRef node = XmlHelpers::CreateXmlNode("Temp");
    m_MatSender->SendMessage(eMSM_Init, node);
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::GotoMaterial(CMaterial* pMaterial)
{
    if (pMaterial)
    {
        GetIEditor()->OpenMaterialLibrary(pMaterial);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::GotoMaterial(_smart_ptr<IMaterial> pMtl)
{
    if (pMtl)
    {
        CMaterial* pEdMaterial = FromIMaterial(pMtl);
        if (pEdMaterial)
        {
            GetIEditor()->OpenMaterialLibrary(pEdMaterial);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::SetHighlightedMaterial(CMaterial* pMtl)
{
    if (m_pHighlightMaterial)
    {
        RemoveFromHighlighting(m_pHighlightMaterial, eHighlight_Pick);
    }

    m_pHighlightMaterial = pMtl;
    if (m_pHighlightMaterial)
    {
        AddForHighlighting(m_pHighlightMaterial);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::HighlightedMaterialChanged(CMaterial* pMtl)
{
    if (!pMtl)
    {
        return;
    }

    RemoveFromHighlighting(pMtl, eHighlight_All);
    AddForHighlighting(pMtl);
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::SetHighlightMask(int highlightMask)
{
    if (m_highlightMask != highlightMask)
    {
        m_highlightMask = highlightMask;

        UpdateHighlightedMaterials();
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialManager::GatherResources(_smart_ptr<IMaterial> pMaterial, CUsedResources& resources)
{
    if (!pMaterial)
    {
        return;
    }

    int nSubMtlCount = pMaterial->GetSubMtlCount();
    if (nSubMtlCount > 0)
    {
        for (int i = 0; i < nSubMtlCount; i++)
        {
            GatherResources(pMaterial->GetSubMtl(i), resources);
        }
    }
    else
    {
        SShaderItem& shItem = pMaterial->GetShaderItem();
        if (shItem.m_pShaderResources)
        {
            SInputShaderResources   res;
            shItem.m_pShaderResources->ConvertToInputResource(&res);

            for (auto& iter : res.m_TexturesResourcesMap )
            {
                SEfResTexture* pTexture = &(iter.second);
                if (!pTexture->m_Name.empty())
                {
                    resources.Add(pTexture->m_Name.c_str());
                }
            }
            gEnv->pRenderer->EF_ReleaseInputShaderResource(&res);
        }
    }
}

///////////////////////////////////////////////////////////////////////////
void CMaterialManager::GetHighlightColor(ColorF* color, float* intensity, int flags)
{
    MAKE_SURE(m_pHighlighter, return );
    m_pHighlighter->GetHighlightColor(color, intensity, flags);
}

///////////////////////////////////////////////////////////////////////////
// This will be called when the editor welcome screen is displayed.
// At this point the editor is ready for UI events, which means we can
// process .dccmtl paths and display error to the user if necessary
bool CMaterialManager::SkipEditorStartupUI()
{
    // Editor started
    m_bEditorUiReady = true;

    // If we have any file paths buffered
    if (m_sourceControlBuffer.size() > 0)
    {
        // Start queuing
        QueueSourceControlTick();
    }

    // Launch thread responsible for saving cached
    // .dccmtl files as source .mtl files
    StartDccMaterialSaveThread();

    // Never want to skip Startup UI
    return false;
}

///////////////////////////////////////////////////////////////////////////
// Queues the function TickSourceControl() to be exectued next frame
void CMaterialManager::QueueSourceControlTick()
{
    // If TickSourceControl is not currently queued
    if (!m_sourceControlFunctionQueued)
    {
        // Queue it
        AZStd::function<void()> tickFunction = [this]()
        {
            TickSourceControl();
        };
        AZ::SystemTickBus::QueueFunction(tickFunction);

        // Stop further queues as TickSourceControl will queue itself
        // until there are no more paths in the buffer to process
        m_sourceControlFunctionQueued = true;
    }
}

///////////////////////////////////////////////////////////////////////////
// Takes a single path from m_sourceControlBuffer and passes it to
// DccMaterialSourceControlCheck(). Then if there are more paths
// remaining in the buffer, it will queue itself for execution next
// frame. The reason for doing only one material every tick is to avoid
// flooding source control with too many requests and stalling the editor
void CMaterialManager::TickSourceControl()
{
    m_sourceControlFunctionQueued = false;
    AZStd::string filePath;
    bool moreRemaining = false;

    {
        AZStd::lock_guard<AZStd::mutex> lock(m_sourceControlBufferMutex);

        if (m_sourceControlBuffer.size() < 1)
        {
            return;
        }

        filePath = m_sourceControlBuffer.back();
        m_sourceControlBuffer.pop_back();
        moreRemaining = !m_sourceControlBuffer.empty();
    }

    // Process it
    DccMaterialSourceControlCheck(filePath);

    // If there are more paths to check
    if (moreRemaining)
    {
        // Queue again
        QueueSourceControlTick();
    }
}

///////////////////////////////////////////////////////////////////////////
// Launches new thread running the DccMaterialSaveThreadFunc() function
void CMaterialManager::StartDccMaterialSaveThread()
{
    AZStd::thread_desc threadDesc;
    threadDesc.m_name = "Dcc Material Save Thread";

    m_dccMaterialSaveThread = AZStd::thread(
        [this]()
        {
            DccMaterialSaveThreadFunc();
        },
        &threadDesc);
}

///////////////////////////////////////////////////////////////////////////
// Will save all the .dccmtl file paths in the buffer to source .mtl
// Runs on a separate thread so as not to stall the main thread
void CMaterialManager::DccMaterialSaveThreadFunc()
{
    while (true)
    {
        m_dccMaterialSaveSemaphore.acquire();

        // Exit condition, set to true in destructor
        if (m_joinThreads)
        {
            return;
        }

        AZStd::vector<AZStd::string> dccMaterialPaths;

        // Lock the buffer and copy file paths locally
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_dccMaterialSaveMutex);
            dccMaterialPaths.reserve(m_dccMaterialSaveBuffer.size());
            for (AZStd::string& fileName : m_dccMaterialSaveBuffer)
            {
                dccMaterialPaths.push_back(fileName);
            }
            m_dccMaterialSaveBuffer.clear();
        }

        // Save all the buffered .dccmtl files
        for (AZStd::string& fileName : dccMaterialPaths)
        {
            SaveDccMaterial(fileName);
        }

        // Clear local strings
        dccMaterialPaths.clear();
    }
}

///////////////////////////////////////////////////////////////////////////
// Async source control request. If successful, the callback will add the
// file name to the buffer for processing by the Dcc Material Save Thread
void CMaterialManager::DccMaterialSourceControlCheck(const AZStd::string& relativeDccMaterialPath)
{
    AZStd::string fullSourcePath = DccMaterialToSourcePath(relativeDccMaterialPath);

    if (!DccMaterialRequiresSave(relativeDccMaterialPath, fullSourcePath))
    {
        // Source .mtl update not required, early out
        return;
    }

    // Create callback for source control operation (see SCCommandBus::Broadcast below)
    AzToolsFramework::SourceControlResponseCallback callback =
        [this, relativeDccMaterialPath, fullSourcePath](bool success, const AzToolsFramework::SourceControlFileInfo& info)
    {
        if (success || !info.IsReadOnly())
        {
            // File needs saving, add it to the buffer for processing by the dcc material thread

            // Lock access to the buffer
            AZStd::lock_guard<AZStd::mutex> lock(m_dccMaterialSaveMutex);

            // Add file path
            m_dccMaterialSaveBuffer.push_back(relativeDccMaterialPath);

            // Notify thread there's work to do
            m_dccMaterialSaveSemaphore.release();
        }
        else
        {
            QString errorMessage = QObject::tr("Could not check out read-only file %1 in source control. Either check your source control configuration or disable source control.").arg(QString::fromUtf8(fullSourcePath.c_str()));

            // Alter error message slightly if source control is disabled
            bool isSourceControlActive = false;
            AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(isSourceControlActive, &AzToolsFramework::SourceControlConnectionRequestBus::Events::IsActive);

            if (!isSourceControlActive)
            {
                errorMessage = QObject::tr("Could not check out read-only file %1 because source control is disabled. Either enable source control or check out the file manually to make it writable.").arg(QString::fromUtf8(fullSourcePath.c_str()));
            }

            // Pop open an error message box if this is the first error we encounter
            if (!m_bSourceControlErrorReported)
            {
                // Report warning in message box
                QString errorTitle = QStringLiteral("Dcc Material Error");
                QMessageBox::warning(QApplication::activeWindow(), errorTitle, errorMessage, QMessageBox::Cancel);

                // Only report source control error box to the user once,
                // no need to spam them for every material
                m_bSourceControlErrorReported = true;
            }

            AZ_Error("Rendering", false, errorMessage.toUtf8().data());
        }
    };

    // Request edit from source control (happens asynchronously)
    using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
    SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, fullSourcePath.c_str(), true, callback);
}

///////////////////////////////////////////////////////////////////////////
// Handles when .dccmtl is created
void CMaterialManager::EntryAdded(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* assetEntry)
{
    if (assetEntry->GetEntryType() != AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Product)
    {
        // Ignore non-product entries
        return;
    }
    const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* productAssetEntry = azrtti_cast<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>(assetEntry);
    if (productAssetEntry && productAssetEntry->GetAssetType() != m_dccMaterialAssetType)
    {
        // Ignore types that aren't .dccmtl
        return;
    }

    AddDccMaterialPath(productAssetEntry->GetRelativePath());
}

///////////////////////////////////////////////////////////////////////////
// Handles when .dccmtl is edited
void CMaterialManager::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
{
    AZ::Data::AssetInfo assetInfo;
    EBUS_EVENT_RESULT(assetInfo, AZ::Data::AssetCatalogRequestBus, GetAssetInfoById, assetId);

    if (assetInfo.m_assetType != m_dccMaterialAssetType)
    {
        // Ignore types that aren't .dccmtl
        return;
    }

    AddDccMaterialPath(assetInfo.m_relativePath);
}

///////////////////////////////////////////////////////////////////////////
void CMaterialManager::AddDccMaterialPath(const AZStd::string relativeDccMaterialPath)
{
    if (relativeDccMaterialPath.empty())
    {
        return;
    }

    // Lock access to the buffer
    AZStd::lock_guard<AZStd::mutex> lock(m_sourceControlBufferMutex);

    // Add file path
    m_sourceControlBuffer.push_back(relativeDccMaterialPath);

    if (m_bEditorUiReady)
    {
        QueueSourceControlTick();
    }
}

///////////////////////////////////////////////////////////////////////////
// Given the path of a .dccmtl in cache, save it as a source .mtl
void CMaterialManager::SaveDccMaterial(const AZStd::string& relativeDccMaterialPath)
{
    // __________________________________________________
    // Load .dccmtl

    XmlNodeRef dccNode = GetISystem()->LoadXmlFromFile(relativeDccMaterialPath.c_str());

    if (!dccNode)
    {
        AZ_Error("MaterialManager", false, "CMaterialManager::SaveDccMaterial: Failed to load XML node from .dccmtl file: %s", relativeDccMaterialPath.c_str());
        return;
    }

    // __________________________________________________
    // Save as source .mtl file

    AZStd::string fullSourcePath = DccMaterialToSourcePath(relativeDccMaterialPath);
    bool saveSuccessful = dccNode->saveToFile(fullSourcePath.c_str());

    if (!saveSuccessful)
    {
        AZ_Error("MaterialManager", false, "CMaterialManager::SaveDccMaterial: Failed to save source .mtl from .dccmtl file: %s", relativeDccMaterialPath.c_str());
    }
}

///////////////////////////////////////////////////////////////////////////
// Compares the hash values from .dccmtl and source .mtl to determine if
// .dccmtl has changed and needs to be saved.
bool CMaterialManager::DccMaterialRequiresSave(const AZStd::string& relativeDccMaterialPath, const AZStd::string& fullSourcePath)
{
    // __________________________________________________
    // Get Source Hash

    AZ::u32 sourceHash = 0;

    // Check if material is already loaded
    QString unifiedName = UnifyMaterialName(QString(relativeDccMaterialPath.c_str()));
    CMaterial* sourceMaterial = (CMaterial*)FindItemByName(unifiedName);

    if (sourceMaterial && !sourceMaterial->IsDummy())
    {
        sourceHash = sourceMaterial->GetDccMaterialHash();
    }
    else
    {
        XmlNodeRef sourceNode = GetISystem()->LoadXmlFromFile(fullSourcePath.c_str());
        if (sourceNode)
        {
            sourceNode->getAttr("DccMaterialHash", sourceHash);
        }
        else
        {
            // Couldn't find source node or material, so we need to save the dcc material as a source material
            // No need to check the dcc material hash, just return true
            return true;
        }
    }

    // __________________________________________________
    // Get DCC material Hash

    AZ::u32 dccHash = 0;
    XmlNodeRef dccNode = GetISystem()->LoadXmlFromFile(relativeDccMaterialPath.c_str());

    if (!dccNode)
    {
        AZ_Error("MaterialManager", false, "CMaterialManager::DccMaterialRequiresSave: Failed to load XML node from .dccmtl file: %s", relativeDccMaterialPath.c_str());
        return false;
    }

    dccNode->getAttr("DccMaterialHash", dccHash);

    // __________________________________________________
    // Compare hash values

    // Only update if .dccmtl hash is different from the source hash
    return (dccHash != sourceHash);
}

