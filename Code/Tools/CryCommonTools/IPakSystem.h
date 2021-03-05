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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_IPAKSYSTEM_H
#define CRYINCLUDE_CRYCOMMONTOOLS_IPAKSYSTEM_H
#pragma once

#include <platform.h>

struct PakSystemFile;
struct PakSystemArchive;
struct IPakSystem
{
    virtual PakSystemFile* Open(const char* filename, const char* mode) = 0;
    virtual bool ExtractNoOverwrite(const char* filename, const char* extractToFile = 0) = 0;
    virtual void Close(PakSystemFile* file) = 0;
    virtual int GetLength(PakSystemFile* file) const = 0;
    virtual int Read(PakSystemFile* file, void* buffer, int size) = 0;
    virtual bool EoF(PakSystemFile* file) = 0;

    virtual PakSystemArchive* OpenArchive(const char* path, size_t fileAlignment = 1, bool encrypted = false, const uint32 encryptionKey[4] = 0) = 0;
    virtual void CloseArchive(PakSystemArchive* archive) = 0;

    // Summary:
    //   Adds a new file to the pak or update an existing one.
    //   Adds a directory (creates several nested directories if needed)
    // Arguments:
    //   path - relative path inside archive
    //   data, size - file content
    //   modTime - modification timestamp of the file
    //   compressionLevel - level of compression (correnponds to zlib-levels):
    //                      -1 or [0-9] where -1=default compression, 0=no compression, 9=best compression
    virtual void AddToArchive(PakSystemArchive* archive, const char* path, void* data, int size, int64 modTime, int compressionLevel = -1) = 0;

    virtual bool DeleteFromArchive(PakSystemArchive* archive, const char* path) = 0;
    virtual bool CheckIfFileExist(PakSystemArchive* archive, const char* path, int64 modTime) = 0;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_IPAKSYSTEM_H
