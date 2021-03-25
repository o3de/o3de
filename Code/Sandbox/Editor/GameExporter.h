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

#ifndef CRYINCLUDE_EDITOR_GAMEEXPORTER_H
#define CRYINCLUDE_EDITOR_GAMEEXPORTER_H
#pragma once

#include "Util/PakFile.h"
#include "Util/Image.h"

enum EGameExport
{
    eExp_SurfaceTexture = BIT(0),
    eExp_CoverSurfaces  = BIT(2),
    eExp_Fast           = BIT(3)
};


class CTerrainLightGen;
class CWaitProgress;
class CUsedResources;

struct SGameExporterSettings
{
    UINT iExportTexWidth;
    int nApplySS;

    SGameExporterSettings();
    void SetLowQuality();
    void SetHiQuality();
};

struct SANDBOX_API SLevelPakHelper
{
    SLevelPakHelper()
        :  m_bPakOpened(false)
        , m_bPakOpenedCryPak(true) {}

    QString m_sPath;
    CPakFile m_pakFile;
    bool m_bPakOpened;
    bool m_bPakOpenedCryPak;
};


/*! CGameExporter implements exporting of data fom Editor to Game format.
        It will produce level.pak file in current level folder, with nececcary exported files.
 */
class SANDBOX_API CGameExporter
{
public:
    CGameExporter();
    ~CGameExporter();
    static const char* GetLevelPakFilename() { return "level.pak"; }
    SGameExporterSettings& GetSettings() { return m_settings; }

    SLevelPakHelper& GetLevelPack() { return m_levelPak; }

    // In auto exporting mode, highest possible settings will be chosen and no UI dialogs will be shown.
    void SetAutoExportMode(bool bAuto) { m_bAutoExportMode = bAuto; }

    bool Export(unsigned int flags = 0, EEndian eExportEndian = GetPlatformEndian(), const char* subdirectory = 0);

    bool OpenLevelPack(SLevelPakHelper& lphelper, bool bCryPak = false);
    bool CloseLevelPack(SLevelPakHelper& lphelper, bool bCryPak = false);
    static CGameExporter* GetCurrentExporter() { return m_pCurrentExporter; }

private:
    void ExportLevelData(const QString& path, bool bExportMission = true);
    void ExportLevelInfo(const QString& path);

    void ExportVisAreas(const char* pszGamePath, EEndian eExportEndian);
    void ExportOcclusionMesh(const char* pszGamePath);
    void ExportMapInfo(XmlNodeRef& node);

    void ExportLevelLensFlares(const QString& path);
    void ExportLevelResourceList(const QString& path);
    void ExportLevelUsedResourceList(const QString& path);
    void ExportLevelShaderCache(const QString& path);
    void ExportMaterials(XmlNodeRef& levelDataNode, const QString& path);
    void ExportGameData(const QString& path);
    void ExportFileList(const QString& path, const QString& levelName);

    void Error(const QString& error);

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QString m_levelPath;
    SLevelPakHelper m_levelPak;
    SGameExporterSettings m_settings;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    bool m_bAutoExportMode;
    int m_numExportedMaterials;

    static CGameExporter* m_pCurrentExporter;
};

// Helper to setup terrain info.
template<typename Func>
void SetupTerrainInfo(const size_t octreeCompiledDataSize, Func&& setupTerrainFn)
{
    // only setup the terrain if we know space has been allocated for the octree
    if (octreeCompiledDataSize > 0)
    {
        setupTerrainFn(octreeCompiledDataSize);
    }
}

#endif // CRYINCLUDE_EDITOR_GAMEEXPORTER_H
