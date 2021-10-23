/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
#include "UsedResources.h"
#include "WaitProgress.h"
#include "Util/CryMemFile.h"
#include "Objects/ObjectManager.h"

#include "Objects/EntityObject.h"

#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

//////////////////////////////////////////////////////////////////////////
#define MUSIC_LEVEL_LIBRARY_FILE "music.xml"
#define MATERIAL_LEVEL_LIBRARY_FILE "materials.xml"
#define RESOURCE_LIST_FILE "resourcelist.txt"
#define USED_RESOURCE_LIST_FILE "usedresourcelist.txt"
#define SHADER_LIST_FILE "shaderslist.txt"

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

CGameExporter* CGameExporter::m_pCurrentExporter = nullptr;

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
    m_pCurrentExporter = nullptr;
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

        CCryEditDoc* pDocument = pEditor->GetDocument();

        if (flags & eExp_Fast)
        {
            m_settings.SetLowQuality();
        }
        else if (m_bAutoExportMode)
        {
            m_settings.SetHiQuality();
        }

        AZStd::scoped_lock autoLock(CGameEngine::GetPakModifyMutex());

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
            ////////////////////////////////////////////////////////////////////////
            // Exporting map setttings
            ////////////////////////////////////////////////////////////////////////
            ExportOcclusionMesh(sLevelPath.toUtf8().data());

            //! Export Level data.
            CLogFile::WriteLine("Exporting leveldata.xml");
            ExportLevelData(sLevelPath);
            CLogFile::WriteLine("Exporting leveldata.xml done.");

            ExportLevelInfo(sLevelPath);

            ExportLevelResourceList(sLevelPath);
            ExportLevelUsedResourceList(sLevelPath);

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
        Temp.Allocate(static_cast<int>(Size));
        FileIn.read(reinterpret_cast<char*>(Temp.GetBuffer()), Size);
        FileIn.close();
        CCryMemFile FileOut;
        FileOut.Write(Temp.GetBuffer(), static_cast<int>(Size));
        m_levelPak.m_pakFile.UpdateFile(levelDataFile.toUtf8().data(), FileOut);
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportLevelData(const QString& path, bool /*bExportMission*/)
{
    IEditor* pEditor = GetIEditor();
    pEditor->SetStatusText(QObject::tr("Exporting leveldata.xml..."));

    char versionString[256];
    pEditor->GetFileVersion().ToString(versionString);

    XmlNodeRef root = XmlHelpers::CreateXmlNode("leveldata");
    root->setAttr("SandboxVersion", versionString);
    XmlNodeRef rootAction = XmlHelpers::CreateXmlNode("leveldataaction");
    rootAction->setAttr("SandboxVersion", versionString);

    //////////////////////////////////////////////////////////////////////////
    // Save Level Data XML
    //////////////////////////////////////////////////////////////////////////
    QString levelDataFile = path + "leveldata.xml";
    XmlString xmlData = root->getXML();
    CCryMemFile file;
    file.Write(xmlData.c_str(), static_cast<unsigned int>(xmlData.length()));
    m_levelPak.m_pakFile.UpdateFile(levelDataFile.toUtf8().data(), file);

    QString levelDataActionFile = path + "leveldataaction.xml";
    XmlString xmlDataAction = rootAction->getXML();
    CCryMemFile fileAction;
    fileAction.Write(xmlDataAction.c_str(), static_cast<unsigned int>(xmlDataAction.length()));
    m_levelPak.m_pakFile.UpdateFile(levelDataActionFile.toUtf8().data(), fileAction);

    AZStd::vector<char> entitySaveBuffer;
    AZ::IO::ByteContainerStream<AZStd::vector<char> > entitySaveStream(&entitySaveBuffer);
    bool savedEntities = false;
    EBUS_EVENT_RESULT(savedEntities, AzToolsFramework::EditorEntityContextRequestBus, SaveToStreamForGame, entitySaveStream, AZ::DataStream::ST_BINARY);
    if (savedEntities)
    {
        QString entitiesFile;
        entitiesFile = QStringLiteral("%1%2.entities_xml").arg(path, "mission0");
        m_levelPak.m_pakFile.UpdateFile(entitiesFile.toUtf8().data(), entitySaveBuffer.begin(), static_cast<int>(entitySaveBuffer.size()));
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
    const AZ::Vector2 terrainGridResolution = terrain ? terrain->GetTerrainHeightQueryResolution() : AZ::Vector2::CreateOne();
    const int compiledHeightmapSize = static_cast<int>(terrainAabb.GetXExtent() / terrainGridResolution.GetX());
    root->setAttr("HeightmapSize", compiledHeightmapSize);

    //////////////////////////////////////////////////////////////////////////
    // Save LevelInfo file.
    //////////////////////////////////////////////////////////////////////////
    QString filename = path + "levelinfo.xml";
    XmlString xmlData = root->getXML();

    CCryMemFile file;
    file.Write(xmlData.c_str(), static_cast<unsigned int>(xmlData.length()));
    m_levelPak.m_pakFile.UpdateFile(filename.toUtf8().data(), file);
}

//////////////////////////////////////////////////////////////////////////
void CGameExporter::ExportLevelResourceList(const QString& path)
{
    auto pResList = gEnv->pCryPak->GetResourceList(AZ::IO::IArchive::RFOM_Level);

    // Write resource list to file.
    CCryMemFile memFile;
    for (const char* filename = pResList->GetFirst(); filename; filename = pResList->GetNext())
    {
        memFile.Write(filename, static_cast<unsigned int>(strlen(filename)));
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
void CGameExporter::ExportFileList(const QString& path, const QString& levelName)
{
    // process the folder of the specified map name, producing a filelist.xml file
    //  that can later be used for map downloads
    AZStd::string newpath;

    AZStd::string filename = levelName.toUtf8().data();
    AZStd::string mapname = (filename + ".dds");
    AZStd::string metaname = (filename + ".xml");

    XmlNodeRef rootNode = gEnv->pSystem->CreateXmlNode("download");
    rootNode->setAttr("name", filename.c_str());
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
