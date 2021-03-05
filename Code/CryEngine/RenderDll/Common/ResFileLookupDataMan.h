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

#ifndef __RESFILELOOKUPDATAMAN_H__
#define __RESFILELOOKUPDATAMAN_H__

#include "ResFile.h"

typedef std::vector<CCryNameTSCRC> TResDirNames;

struct SResFileLookupDataDisk
{
    int m_NumOfFilesUnique;
    int m_NumOfFilesRef;
    uint32 m_OffsetDir;
    uint32 m_CRC32;
    uint16 m_CacheMajorVer;
    uint16 m_CacheMinorVer;

    SResFileLookupDataDisk() {}
    SResFileLookupDataDisk(const struct SResFileLookupData& inLookup);

    AUTO_STRUCT_INFO
};
struct SResFileLookupData
{
    int m_NumOfFilesUnique;
    int m_NumOfFilesRef;
    uint32 m_OffsetDir;
    uint32 m_CRC32;
    uint16 m_CacheMajorVer;
    uint16 m_CacheMinorVer;

    SResFileLookupData() {}
    SResFileLookupData(const SResFileLookupDataDisk& inLookup)
    {
        m_NumOfFilesUnique = inLookup.m_NumOfFilesUnique;
        m_NumOfFilesRef = inLookup.m_NumOfFilesRef;
        m_OffsetDir = inLookup.m_OffsetDir;
        m_CRC32 = inLookup.m_CRC32;
        m_CacheMajorVer = inLookup.m_CacheMajorVer;
        m_CacheMinorVer = inLookup.m_CacheMinorVer;
    }

#ifdef USE_PARTIAL_ACTIVATION
    bool m_ContainsResDir;

    TResDirNames m_resdirlookup;
    ResDir m_resdir;

    unsigned int GetDirOffset(const CCryNameTSCRC& dirEntryName) const;
#endif
};

struct SCFXLookupData
{
    uint32 m_CRC32;
    SCFXLookupData() {}
    AUTO_STRUCT_INFO
};

#define MAX_DIR_BUFFER_SIZE 300000

typedef std::map<CCryNameTSCRC, SResFileLookupData> TFileResDirDataMap;
typedef std::map<CCryNameTSCRC, SCFXLookupData> TFileCFXDataMap;

class CResFile;

struct SVersionInfo
{
    SVersionInfo()
        : m_ResVersion(0)
    { memset(m_szCacheVer, 0, sizeof(m_szCacheVer)); }

    int m_ResVersion;
    char m_szCacheVer[16];

    AUTO_STRUCT_INFO
};

class CResFileLookupDataMan
{
public:
    CResFileLookupDataMan();
    ~CResFileLookupDataMan();

    void Clear();
    void Flush();

    CCryNameTSCRC AdjustName(const char* szName);
    int GetResVersion() const { return m_VersionInfo.m_ResVersion; }

    bool LoadData(const char* acFilename, bool bSwapEndianRead, bool bReadOnly);
    void SaveData(const char* acFilename, bool bSwapEndianWrite) const;

    bool IsReadOnly() { return m_bReadOnly; }

    void AddData(const CResFile* pResFile, uint32 CRC);
    void AddDataCFX(const char* szPath, uint32 CRC);
    void RemoveData(uint32 CRC);

    SResFileLookupData* GetData(const CCryNameTSCRC& name);
    SCFXLookupData* GetDataCFX(const char* szPath);
    void MarkDirty(bool bDirty) { m_bDirty = bDirty; }


protected:

    string m_Path;
    SVersionInfo m_VersionInfo;
    TFileResDirDataMap m_Data;
    TFileCFXDataMap m_CFXData;
    unsigned int m_TotalDirStored;
    byte m_bDirty : 1;
    byte m_bReadOnly : 1;
};

#endif //  __RESFILELOOKUPDATAMAN_H__
