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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_PAKSYSTEM_H
#define CRYINCLUDE_CRYCOMMONTOOLS_PAKSYSTEM_H
#pragma once


#include "IPakSystem.h"
#include "ZipDir/ZipDir.h" // TODO: get rid of thid include

enum PakSystemFileType
{
    PakSystemFileType_Unknown,
    PakSystemFileType_File,
    PakSystemFileType_PakFile
};
struct PakSystemFile
{
    PakSystemFile();
    PakSystemFileType type;

    // PakSystemFileType_File
    FILE* file;

    // PakSystemFileType_PakFile
    ZipDir::CachePtr zip;
    ZipDir::FileEntry* fileEntry;
    void* data;
    int dataPosition;
};

struct PakSystemArchive
{
    ZipDir::CacheRWPtr zip;
};

class PakSystem
    : public IPakSystem
{
public:
    PakSystem();

    // IPakSystem
    virtual PakSystemFile* Open(const char* filename, const char* mode);
    virtual bool ExtractNoOverwrite(const char* filename, const char* extractToFile = 0);
    virtual void Close(PakSystemFile* file);
    virtual int GetLength(PakSystemFile* file) const;
    virtual int Read(PakSystemFile* file, void* buffer, int size);
    virtual bool EoF(PakSystemFile* file);

    virtual PakSystemArchive* OpenArchive(const char* path, size_t fileAlignment, bool encrypted, const uint32 encryptionKey[4]);
    virtual void CloseArchive(PakSystemArchive* archive);
    virtual void AddToArchive(PakSystemArchive* archive, const char* path, void* data, int size, int64 modTime, int compressionLevel);
    virtual bool DeleteFromArchive(PakSystemArchive* archive, const char* path);
    virtual bool CheckIfFileExist(PakSystemArchive* archive, const char* path, int64 modTime);
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_PAKSYSTEM_H
