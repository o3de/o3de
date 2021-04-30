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

// AzCore
#include <AzCore/IO/IStreamer.h>
#include <AzCore/std/parallel/binary_semaphore.h>

// CryCommon
#include <CryCommon/ILevelSystem.h>

// Editor
#include "GameExporter.h"
#include "GameEngine.h"
#include "CryEditDoc.h"
#include "Mission.h"
#include "ShaderCache.h"
#include "UsedResources.h"
#include "Material/MaterialManager.h"
#include "Material/MaterialLibrary.h"
#include "WaitProgress.h"
#include "Util/CryMemFile.h"
#include "Objects/ObjectManager.h"

#include "Objects/EntityObject.h"

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

//////////////////////////////////////////////////////////////////////////
#define MUSIC_LEVEL_LIBRARY_FILE "Music.xml"
#define MATERIAL_LEVEL_LIBRARY_FILE "Materials.xml"
#define RESOURCE_LIST_FILE "ResourceList.txt"
#define USED_RESOURCE_LIST_FILE "UsedResourceList.txt"
#define SHADER_LIST_FILE "ShadersList.txt"

#define GetAValue(rgb)      ((BYTE)((rgb) >> 24))

//////////////////////////////////////////////////////////////////////////
// SGameExporterSettings
//////////////////////////////////////////////////////////////////////////
SGameExporterSettings::SGameExporterSettings()
    : iExportTexWidth(4096)
    , nApplySS(1)
{
}


//////////////////////////////////////////////////////////////////////////
void SGameExporterSettings::SetLowQuality()
{
    iExportTexWidth = 4096;
    nApplySS = 0;
}


//////////////////////////////////////////////////////////////////////////
void SGameExporterSettings::SetHiQuality()
{
    iExportTexWidth = 16384;
    nApplySS = 1;
}

CGameExporter* CGameExporter::m_pCurrentExporter = NULL;

//////////////////////////////////////////////////////////////////////////
// CGameExporter
//////////////////////////////////////////////////////////////////////////
CGameExporter::CGameExporter()
    : m_bAutoExportMode(false)
{
    m_pCurrentExporter = this;
}

CGameExporter::~CGameExporter()
{
    m_pCurrentExporter = NULL;
}

//////////////////////////////////////////////////////////////////////////
bool CGameExporter::Export(unsigned int flags, [[maybe_unused]] EEndian eExportEndian, const char* subdirectory)
{
    CAutoDocNotReady autoDocNotReady;
    CObjectManagerLevelIsExporting levelIsExportingFlag;
    QWaitCursor waitCursor;

    IEditor* pEditor = GetIEditor();
    CGameEngine* pGameEngine = pEditor->GetGameEngine();
    if (pGameEngine->GetLevelPath().isEmpty())
    {
        return false;
    }

    bool exportSuccessful = true;

    CrySystemEventBus::Broadcast(&CrySystemEventBus::Events::OnCryEditorBeginLevelExport);

    bool usePrefabSystemForLevels = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);

    if (usePrefabSystemForLevels)
    {
        // Level.pak and all the data contained within it is unused when using the prefab system for levels, so there's nothing
        // to do here.

        CCryEditDoc* pDocument = pEditor->GetDocument();
        pDocument->SetLevelExported(true);
    }
    else
    {
        QDir::setCurrent(pEditor->GetPrimaryCDFolder());

        QString sLevelPath = Path::AddSlash(pGameEngine->GetLevelPath());
        if (subdirectory && subdirectory[0] && strcmp(subdirectory, ".") != 0)
        {
            sLevelPath = Path::AddSlash(sLevelPath + subdirectory);
            QDir().mkpath(sLevelPath);
        }

        m_levelPak.m_sPath = QString(sLevelPath) + GetLevelPakFilename();

        m_levelPath = Path::RemoveBackslash(sLevelPath);
        QString rootLevelPath = Path::AddSlash(pGameEngine->GetLevelPath());

        // Make sure we unload any unused CGFs before exporting so that they don't end up in
        // the level data.
        if (pEditor->Get3DEngine())
        {
            pEditor->Get3DEngine()->FreeUnusedCGFResources();
        }

        CCryEditDoc* pDocument = pEditor->GetDocument();

        if (flags & eExp_Fast)
        {
            m_settings.SetLowQuality();
        }
        else if (m_bAutoExportMode)
        {
            m_settings.SetHiQuality();
        }

        CryAutoLock<CryMutex> autoLock(CGameEngine::GetPakModifyMutex());

        // Close this pak file.
        if (!CloseLevelPack(m_levelPak, true))
        {
            Error("Cannot close Pak file " + m_levelPak.m_sPath);
            exportSuccessful = false;
        }

        if (exportSuccessful)
        {
            if (m_bAutoExportMode)
            {
                // Remove read-only flags.
                CrySetFileAttributes(m_levelPak.m_sPath.toUtf8().data(), FILE_ATTRIBUTE_NORMAL);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        if (exportSuccessful)
        {
            if (!CFileUtil::OverwriteFile(m_levelPak.m_sPath))
            {
                Error("Cannot overwrite Pak file " + m_levelPak.m_sPath);
                exportSuccessful = false;
            }
        }

        if (exportSuccessful)
        {
            if (!OpenLevelPack(m_levelPak, false))
            {
                Error("Cannot open Pak file " + m_levelPak.m_sPath + " for writing.");
                exportSuccessful = false;
            }
        }

        ////////////////////////////////////////////////////////////////////////
        // Export all data to the game
        ////////////////////////////////////////////////////////////////////////
        if (exportSuccessful)
        {
            ExportVisAreas(sLevelPath.toUtf8().data(), eExportEndian);

            ////////////////////////////////////////////////////////////////////////
            // Exporting map setttings
            ////////////////////////////////////////////////////////////////////////
            ExportOcclusionMesh(sLevelPath.toUtf8().data());

            //! Export Level data.
            CLogFile::WriteLine("Exporting LevelData.xml");
            ExportLevelData(sLevelPath);
            CLogFile::WriteLine("Exporting LevelData.xml done.");

            ExportLevelInfo(sLevelPath);

            ExportLevelResourceList(sLevelPath);
            ExportLevelUsedResourceList(sLevelPath);
            ExportLevelShaderCache(sLevelPath);

            //////////////////////////////////////////////////////////////////////////
            // End Exporting Game data.
            //////////////////////////////////////////////////////////////////////////

            // Close all packs.
            CloseLevelPack(m_levelPak, false);
            //  m_texturePakFile.Close();

            pEditor->SetStatusText(QObject::tr("Ready"));

            // Reopen this pak file.
            if (!OpenLevelPack(m_levelPak, true))
            {
                Error("Cannot open Pak file " + m_levelPak.m_sPath);
                exportSuccessful = false;
            }
        }

        if (exportSuccessful)
        {
            // Commit changes to the disk.
            _flushall();

            // finally create filelist.xml
            QString levelName = Path::GetFileName(pGameEngine->GetLevelPath());
            ExportFileList(sLevelPath, levelName);

            pDocument->SetLevelExported(true);
        }
    }

    // Always notify that we've finished exporting, whether it was successful or not.
    CrySystemEventBus::Broadcast(&CrySystemEventBus::Events::OnCryEditorEndLevelExport, exportSuccessful);

    if (exportSuccessful)
    {
        // Notify the level system that there's a new level, so that the level info is populated.
        gEnv->pSystem->GetILevelSystem()->Rescan(ILevelSystem::GetLevelsDirectoryName());

        CLogFile::WriteLine("Exporting was successful.");
    }

    return exportSuccessful;
}


//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportVisAreas(const char* pszGamePath, EEndian eExportEndian)
{
    char szFileOutputPath[_MAX_PATH];

    // export visareas
    IEditor* pEditor = GetIEditor();

    // remove old files
    sprintf_s(szFileOutputPath, "%s%s", pszGamePath, COMPILED_VISAREA_MAP_FILE_NAME);
    m_levelPak.m_pakFile.RemoveFile(szFileOutputPath);

    SHotUpdateInfo exportInfo;
    I3DEngine* p3DEngine = pEditor->Get3DEngine();

    if (p3DEngine && (eExportEndian == GetPlatformEndian())) // skip second export, this data is common for PC and consoles
    {
        std::vector<struct IStatObj*>* pTempBrushTable = NULL;
        std::vector<_smart_ptr<IMaterial>>* pTempMatsTable = NULL;
        std::vector<struct IStatInstGroup*>* pTempVegGroupTable = NULL;

        // export visareas
        CLogFile::WriteLine("Exporting indoors...");
        pEditor->SetStatusText("Exporting indoors...");
        if (IVisAreaManager* pVisAreaManager = p3DEngine->GetIVisAreaManager())
        {
            if (int nSize = pVisAreaManager->GetCompiledDataSize())
            { // get visareas data from 3dengine and save it into file
                uint8* pData = new uint8[nSize];
                pVisAreaManager->GetCompiledData(pData, nSize, &pTempBrushTable, &pTempMatsTable, &pTempVegGroupTable, eExportEndian);
                sprintf_s(szFileOutputPath, "%s%s", pszGamePath, COMPILED_VISAREA_MAP_FILE_NAME);
                CCryMemFile visareasCompiledFile;
                visareasCompiledFile.Write(pData, nSize);
                m_levelPak.m_pakFile.UpdateFile(szFileOutputPath, visareasCompiledFile);
                delete[] pData;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportOcclusionMesh(const char* pszGamePath)
{
    IEditor* pEditor = GetIEditor();
    pEditor->SetStatusText(QObject::tr("including Occluder Mesh \"occluder.ocm\" if available"));

    char resolvedLevelPath[AZ_MAX_PATH_LEN] = { 0 };
    AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(pszGamePath, resolvedLevelPath, AZ_MAX_PATH_LEN);
    QString levelDataFile = QString(resolvedLevelPath) + "occluder.ocm";
    QFile FileIn(levelDataFile);
    if (FileIn.open(QFile::ReadOnly))
    {
        CMemoryBlock Temp;
        const size_t Size   =   FileIn.size();
        Temp.Allocate(Size);
        FileIn.read(reinterpret_cast<char*>(Temp.GetBuffer()), Size);
        FileIn.close();
        CCryMemFile FileOut;
        FileOut.Write(Temp.GetBuffer(), Size);
        m_levelPak.m_pakFile.UpdateFile(levelDataFile.toUtf8().data(), FileOut);
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportLevelData(const QString& path, bool bExportMission)
{
    IEditor* pEditor = GetIEditor();
    pEditor->SetStatusText(QObject::tr("Exporting LevelData.xml..."));

    char versionString[256];
    pEditor->GetFileVersion().ToString(versionString);

    XmlNodeRef root = XmlHelpers::CreateXmlNode("LevelData");
    root->setAttr("SandboxVersion", versionString);
    XmlNodeRef rootAction = XmlHelpers::CreateXmlNode("LevelDataAction");
    rootAction->setAttr("SandboxVersion", versionString);

    ExportMapInfo(root);

    //////////////////////////////////////////////////////////////////////////
    // Export materials.
    ExportMaterials(root, path);
    //////////////////////////////////////////////////////////////////////////

    CCryEditDoc* pDocument = pEditor->GetDocument();
    CMission* pCurrentMission = 0;

    if (bExportMission)
    {
        pCurrentMission = pDocument->GetCurrentMission();
        // Save contents of current mission.
    }

    //////////////////////////////////////////////////////////////////////////
    // Export missions tag.
    //////////////////////////////////////////////////////////////////////////
    XmlNodeRef missionsNode = rootAction->newChild("Missions");
    QString missionFileName;
    QString currentMissionFileName;
    I3DEngine* p3DEngine = pEditor->Get3DEngine();
    if (p3DEngine)
    {
        for (int i = 0; i < pDocument->GetMissionCount(); i++)
        {
            CMission* pMission = pDocument->GetMission(i);

            QString name = pMission->GetName();
            name.replace(' ', '_');
            missionFileName = QStringLiteral("Mission_%1.xml").arg(name);

            XmlNodeRef missionDescNode = missionsNode->newChild("Mission");
            missionDescNode->setAttr("Name", pMission->GetName().toUtf8().data());
            missionDescNode->setAttr("File", missionFileName.toUtf8().data());
            missionDescNode->setAttr("CGFCount", p3DEngine->GetLoadedObjectCount());

            int nProgressBarRange = m_numExportedMaterials / 10 + p3DEngine->GetLoadedObjectCount();
            missionDescNode->setAttr("ProgressBarRange", nProgressBarRange);

            if (pMission == pCurrentMission)
            {
                currentMissionFileName = missionFileName;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Save Level Data XML
    //////////////////////////////////////////////////////////////////////////
    QString levelDataFile = path + "LevelData.xml";
    XmlString xmlData = root->getXML();
    CCryMemFile file;
    file.Write(xmlData.c_str(), xmlData.length());
    m_levelPak.m_pakFile.UpdateFile(levelDataFile.toUtf8().data(), file);

    QString levelDataActionFile = path + "LevelDataAction.xml";
    XmlString xmlDataAction = rootAction->getXML();
    CCryMemFile fileAction;
    fileAction.Write(xmlDataAction.c_str(), xmlDataAction.length());
    m_levelPak.m_pakFile.UpdateFile(levelDataActionFile.toUtf8().data(), fileAction);

    if (bExportMission)
    {
        XmlNodeRef objectsNode = NULL;
        //////////////////////////////////////////////////////////////////////////
        // Export current mission file.
        //////////////////////////////////////////////////////////////////////////
        XmlNodeRef missionNode = rootAction->createNode("Mission");
        pCurrentMission->Export(missionNode, objectsNode);

        if (p3DEngine)
        {
            missionNode->setAttr("CGFCount", p3DEngine->GetLoadedObjectCount());
        }

        //if (!CFileUtil::OverwriteFile( path+currentMissionFileName ))
        //          return;

        AZStd::vector<char> entitySaveBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
        bool savedEntities = false;
        EBUS_EVENT_RESULT(savedEntities, AzToolsFramework::EditorEntityContextRequestBus, SaveToStreamForGame, entitySaveStream, AZ::DataStream::ST_BINARY);
        if (savedEntities)
        {
            QString entitiesFile;
            entitiesFile = QStringLiteral("%1%2.entities_xml").arg(path, pCurrentMission ? pCurrentMission->GetName() : "");
            m_levelPak.m_pakFile.UpdateFile(entitiesFile.toUtf8().data(), entitySaveBuffer.begin(), entitySaveBuffer.size());
        }

        _smart_ptr<IXmlStringData> pXmlStrData = missionNode->getXMLData(5000000);

        CCryMemFile fileMission;
        fileMission.Write(pXmlStrData->GetString(), pXmlStrData->GetStringLength());
        m_levelPak.m_pakFile.UpdateFile((path + currentMissionFileName).toUtf8().data(), fileMission);
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportLevelInfo(const QString& path)
{
    //////////////////////////////////////////////////////////////////////////
    // Export short level info xml.
    //////////////////////////////////////////////////////////////////////////
    IEditor* pEditor = GetIEditor();
    XmlNodeRef root = XmlHelpers::CreateXmlNode("LevelInfo");
    char versionString[256];
    pEditor->GetFileVersion().ToString(versionString);
    root->setAttr("SandboxVersion", versionString);

    QString levelName = pEditor->GetGameEngine()->GetLevelPath();
    root->setAttr("Name", levelName.toUtf8().data());
    auto terrain = AzFramework::Terrain::TerrainDataRequestBus::FindFirstHandler();
    const AZ::Aabb terrainAabb = terrain ? terrain->GetTerrainAabb() : AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
    const AZ::Vector2 terrainGridResolution = terrain ? terrain->GetTerrainGridResolution() : AZ::Vector2::CreateOne();
    const int compiledHeightmapSize = static_cast<int>(terrainAabb.GetXExtent() / terrainGridResolution.GetX());
    root->setAttr("HeightmapSize", compiledHeightmapSize);

    // Save all missions in this level.
    XmlNodeRef missionsNode = root->newChild("Missions");
    int numMissions = pEditor->GetDocument()->GetMissionCount();
    for (int i = 0; i < numMissions; i++)
    {
        CMission* pMission = pEditor->GetDocument()->GetMission(i);
        XmlNodeRef missionNode = missionsNode->newChild("Mission");
        missionNode->setAttr("Name", pMission->GetName().toUtf8().data());
        missionNode->setAttr("Description", pMission->GetDescription().toUtf8().data());
    }


    //////////////////////////////////////////////////////////////////////////
    // Save LevelInfo file.
    //////////////////////////////////////////////////////////////////////////
    QString filename = path + "LevelInfo.xml";
    XmlString xmlData = root->getXML();

    CCryMemFile file;
    file.Write(xmlData.c_str(), xmlData.length());
    m_levelPak.m_pakFile.UpdateFile(filename.toUtf8().data(), file);
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportMapInfo(XmlNodeRef& node)
{
    if (!GetIEditor()->Get3DEngine())
    {
        return;
    }

    XmlNodeRef info = node->newChild("LevelInfo");

    IEditor* pEditor = GetIEditor();
    info->setAttr("Name", QFileInfo(pEditor->GetDocument()->GetTitle()).completeBaseName());

    auto terrain = AzFramework::Terrain::TerrainDataRequestBus::FindFirstHandler();
    const AZ::Aabb terrainAabb = terrain ? terrain->GetTerrainAabb() : AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
    const AZ::Vector2 terrainGridResolution = terrain ? terrain->GetTerrainGridResolution() : AZ::Vector2::CreateOne();

    const int terrainSizeInMeters = static_cast<int>(terrainAabb.GetXExtent());
    const int terrainUnitSizeInMeters = static_cast<int>(terrainGridResolution.GetX());
    info->setAttr("HeightmapSize", terrainSizeInMeters / terrainUnitSizeInMeters);
    info->setAttr("HeightmapUnitSize", terrainUnitSizeInMeters);
    //! Default Max Height value.
    constexpr int HEIGHTMAP_MAX_HEIGHT = 150; //This is the default max height in CHeightmap
    info->setAttr("HeightmapMaxHeight", HEIGHTMAP_MAX_HEIGHT);
    info->setAttr("WaterLevel", pEditor->Get3DEngine()->GetWaterLevel());

    // Serialize surface types.
    CXmlArchive xmlAr;
    xmlAr.bLoading = false;
    xmlAr.root = node;
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportMaterials(XmlNodeRef& levelDataNode, const QString& path)
{
    //////////////////////////////////////////////////////////////////////////
    // Export materials manager.
    CMaterialManager* pManager = GetIEditor()->GetMaterialManager();
    pManager->Export(levelDataNode);

    QString filename = Path::Make(path, MATERIAL_LEVEL_LIBRARY_FILE);

    bool bHaveItems = true;

    int numMtls = 0;

    XmlNodeRef nodeMaterials = XmlHelpers::CreateXmlNode("MaterialsLibrary");
    // Export Materials local level library.
    for (int i = 0; i < pManager->GetLibraryCount(); i++)
    {
        XmlNodeRef nodeLib = nodeMaterials->newChild("Library");
        CMaterialLibrary* pLib = (CMaterialLibrary*)pManager->GetLibrary(i);
        if (pLib->GetItemCount() > 0)
        {
            bHaveItems = false;
            // Export this library.
            numMtls += pManager->ExportLib(pLib, nodeLib);
        }
    }
    if (!bHaveItems)
    {
        XmlString xmlData = nodeMaterials->getXML();

        CCryMemFile file;
        file.Write(xmlData.c_str(), xmlData.length());
        m_levelPak.m_pakFile.UpdateFile(filename.toUtf8().data(), file);
    }
    else
    {
        m_levelPak.m_pakFile.RemoveFile(filename.toUtf8().data());
    }
    m_numExportedMaterials = numMtls;
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportLevelResourceList(const QString& path)
{
    auto pResList = gEnv->pCryPak->GetResourceList(AZ::IO::IArchive::RFOM_Level);

    // Write resource list to file.
    CCryMemFile memFile;
    for (const char* filename = pResList->GetFirst(); filename; filename = pResList->GetNext())
    {
        memFile.Write(filename, strlen(filename));
        memFile.Write("\n", 1);
    }

    QString resFile = Path::Make(path, RESOURCE_LIST_FILE);

    m_levelPak.m_pakFile.UpdateFile(resFile.toUtf8().data(), memFile, true);
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportLevelUsedResourceList(const QString& path)
{
    // Write used resource list to file.
    CCryMemFile memFile;

    CUsedResources resources;
    GetIEditor()->GetObjectManager()->GatherUsedResources(resources);

    for (CUsedResources::TResourceFiles::const_iterator it = resources.files.begin(); it != resources.files.end(); it++)
    {
        QString filePath = Path::MakeGamePath(*it).toLower();

        memFile.Write(filePath.toUtf8().data(), filePath.toUtf8().length());
        memFile.Write("\n", 1);
    }

    QString resFile = Path::Make(path, USED_RESOURCE_LIST_FILE);

    m_levelPak.m_pakFile.UpdateFile(resFile.toUtf8().data(), memFile, true);
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportLevelShaderCache(const QString& path)
{
    QString buf;
    GetIEditor()->GetDocument()->GetShaderCache()->SaveBuffer(buf);
    CCryMemFile memFile;
    memFile.Write(buf.toUtf8().data(), buf.toUtf8().length());

    QString filename = Path::Make(path, SHADER_LIST_FILE);
    m_levelPak.m_pakFile.UpdateFile(filename.toUtf8().data(), memFile, true);
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportFileList(const QString& path, const QString& levelName)
{
    // process the folder of the specified map name, producing a filelist.xml file
    //  that can later be used for map downloads
    string newpath;

    QString filename = levelName;
    string mapname = (filename + ".dds").toUtf8().data();
    string metaname = (filename + ".xml").toUtf8().data();

    XmlNodeRef rootNode = gEnv->pSystem->CreateXmlNode("download");
    rootNode->setAttr("name", filename.toUtf8().data());
    rootNode->setAttr("type", "Map");
    XmlNodeRef indexNode = rootNode->newChild("index");
    if (indexNode)
    {
        indexNode->setAttr("src", "filelist.xml");
        indexNode->setAttr("dest", "filelist.xml");
    }
    XmlNodeRef filesNode = rootNode->newChild("files");
    if (filesNode)
    {
        newpath = GetIEditor()->GetGameEngine()->GetLevelPath().toUtf8().data();
        newpath += "/*";
        AZ::IO::ArchiveFileIterator handle = gEnv->pCryPak->FindFirst(newpath.c_str());
        if (!handle)
        {
            return;
        }
        do
        {
            // ignore "." and ".."
            if (handle.m_filename.front() == '.')
            {
                continue;
            }
            // do we need any files from sub directories?
            if ((handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory)
            {
                continue;
            }

            // only need the following files for multiplayer downloads
            if (!_stricmp(handle.m_filename.data(), GetLevelPakFilename())
                || !_stricmp(handle.m_filename.data(), mapname.c_str())
                || !_stricmp(handle.m_filename.data(), metaname.c_str()))
            {
                XmlNodeRef newFileNode = filesNode->newChild("file");
                if (newFileNode)
                {
                    // TEMP: this is just for testing. src probably needs to be blank.
                    //      string src = "http://server41/updater/";
                    //      src += m_levelName;
                    //      src += "/";
                    //      src += fileinfo.name;
                    newFileNode->setAttr("src", handle.m_filename.data());
                    newFileNode->setAttr("dest", handle.m_filename.data());
                    newFileNode->setAttr("size", handle.m_fileDesc.nSize);

                    unsigned char md5[16];
                    string filenameToHash = GetIEditor()->GetGameEngine()->GetLevelPath().toUtf8().data();
                    filenameToHash += "/";
                    filenameToHash += string{ handle.m_filename.data(), handle.m_filename.size() };
                    if (gEnv->pCryPak->ComputeMD5(filenameToHash.data(), md5))
                    {
                        char md5string[33];
                        sprintf_s(md5string, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                            md5[0], md5[1], md5[2], md5[3],
                            md5[4], md5[5], md5[6], md5[7],
                            md5[8], md5[9], md5[10], md5[11],
                            md5[12], md5[13], md5[14], md5[15]);
                        newFileNode->setAttr("md5", md5string);
                    }
                    else
                    {
                        newFileNode->setAttr("md5", "");
                    }
                }
            }
        } while (handle = gEnv->pCryPak->FindNext(handle));

        gEnv->pCryPak->FindClose (handle);
    }

    // save filelist.xml
    newpath = path.toUtf8().data();
    newpath += "/filelist.xml";
    rootNode->saveToFile(newpath.c_str());
}

void CGameExporter::Error(const QString& error)
{
    if (m_bAutoExportMode)
    {
        CLogFile::WriteLine((QString("Export failed! ") + error).toUtf8().data());
    }
    else
    {
        Warning((QString("Export failed! ") + error).toUtf8().data());
    }
}


bool CGameExporter::OpenLevelPack(SLevelPakHelper& lphelper, bool bCryPak)
{
    bool bRet = false;

    assert(lphelper.m_bPakOpened == false);
    assert(lphelper.m_bPakOpenedCryPak == false);

    if (bCryPak)
    {
        assert(!lphelper.m_sPath.isEmpty());
        bRet = gEnv->pCryPak->OpenPack(lphelper.m_sPath.toUtf8().data());
        assert(bRet);
        lphelper.m_bPakOpenedCryPak = true;
    }
    else
    {
        bRet = lphelper.m_pakFile.Open(lphelper.m_sPath.toUtf8().data());
        assert(bRet);
        lphelper.m_bPakOpened = true;
    }
    return bRet;
}


bool CGameExporter::CloseLevelPack(SLevelPakHelper& lphelper, bool bCryPak)
{
    bool bRet = false;

    if (bCryPak)
    {
        assert(lphelper.m_bPakOpenedCryPak == true);
        assert(!lphelper.m_sPath.isEmpty());
        bRet = gEnv->pCryPak->ClosePack(lphelper.m_sPath.toUtf8().data());
        assert(bRet);
        lphelper.m_bPakOpenedCryPak = false;
    }
    else
    {
        assert(lphelper.m_bPakOpened == true);
        lphelper.m_pakFile.Close();
        bRet = true;
        lphelper.m_bPakOpened = false;
    }

    assert(lphelper.m_bPakOpened == false);
    assert(lphelper.m_bPakOpenedCryPak == false);
    return bRet;
}
