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

#include "ThumbnailGenerator.h"

// Editor
#include "Util/Image.h"
#include "Util/ImageUtil.h"             // for CUmageUtil
#include "WaitProgress.h"               // for CWaitProgress

#if defined(AZ_PLATFORM_MAC) || defined(AZ_PLATFORM_LINUX)
#include <sys/types.h>
#include <utime.h>
#endif



CThumbnailGenerator::CThumbnailGenerator(void)
{
}

CThumbnailGenerator::~CThumbnailGenerator(void)
{
}

// Get directory contents.
static bool scan_directory(const QString& root, const QString& path, const QString& file, QStringList& files, bool recursive)
{
    QString fullPath = root + path + file;
    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (recursive)
    {
        flags = QDirIterator::Subdirectories;
    }
    QDirIterator dirIterator(fullPath, {file}, QDir::Files, flags);
    if (!dirIterator.hasNext())
    {
        return false;
    }
    else
    {
        // Find the rest of the .c files.
        while (dirIterator.hasNext())
        {
            files.push_back(dirIterator.next());
            //FileInfo fi;
            //fi.attrib = c_file.attrib;
            //fi.name = path + c_file.name;
            /*
                        // Add . after file name without extension.
                        if (fi.name.find('.') == CString::npos) {
                            fi.name.append( "." );
                        }
            */
            //fi.size = c_file.size;
            //fi.time = c_file.time_write;
            //files.push_back( fi );
        }
    }
    return true;
}

#if defined(AZ_PLATFORM_WINDOWS)
#define FileTimeType FILETIME

inline void GetThumbFileTime(const char* fileName, FILETIME& time)
{
    HANDLE hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        GetFileTime(hFile, NULL, NULL, &time);
        CloseHandle(hFile);
    }
}

inline void SetThumbFileTime(const char* fileName, FILETIME& time)
{
    HANDLE hFile = CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        SetFileTime(hFile, NULL, NULL, &time);
        CloseHandle(hFile);
    }
}

inline bool ThumbFileTimeIsEqual(const FILETIME& ft1, const FILETIME& ft2)
{
    return ft1.dwHighDateTime == ft2.dwHighDateTime && ft1.dwLowDateTime == ft2.dwLowDateTime;
}
#elif defined(AZ_PLATFORM_MAC) || defined(AZ_PLATFORM_LINUX)
#define FileTimeType utimbuf

inline void GetThumbFileTime(const char* fileName, utimbuf& times)
{
    struct stat sb;
    if (stat(fileName, &sb) == 0)
    {
        times.actime = sb.st_atime;
        times.modtime = sb.st_mtime;
    }
}

inline void SetThumbFileTime(const char* fileName, const utimbuf& times)
{
    utime(fileName, &times);
}

inline bool ThumbFileTimeIsEqual(const utimbuf& ft1, const utimbuf& ft2)
{
    return ft1.modtime == ft2.modtime;
}
#endif

void CThumbnailGenerator::GenerateForDirectory(const QString& path)
{
    return;

    //////////////////////////////////////////////////////////////////////////
    QStringList files;
    //CString dir = GetIEditor()->GetPrimaryCDFolder();
    QString dir = path;
    scan_directory(dir, "", "*.*", files, true);

    I3DEngine* engine = GetIEditor()->Get3DEngine();

    int thumbSize = 128;
    CImageEx image;
    image.Allocate(thumbSize, thumbSize);

    char drive[_MAX_DRIVE];
    char fdir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char fext[_MAX_EXT];
    char bmpFile[1024];

    GetIEditor()->ShowConsole(true);
    CWaitProgress wait("Generating CGF Thumbnails");
    for (int i = 0; i < files.size(); i++)
    {
        QString file = dir + files[i];
        _splitpath_s(file.toUtf8().data(), drive, fdir, fname, fext);

        //if (_stricmp(fext,".cgf") != 0 && _stricmp(fext,".bld") != 0)
        if (_stricmp(fext, ".cgf") != 0)
        {
            continue;
        }

        if (!wait.Step(100 * i / files.size()))
        {
            break;
        }

        _makepath_s(bmpFile, drive, fdir, fname, ".tmb");

        FileTimeType ft1, ft2;
        GetThumbFileTime(file.toUtf8().data(), ft1);
        GetThumbFileTime(bmpFile, ft2);
        // Both cgf and bmp have same time stamp.
        if (ThumbFileTimeIsEqual(ft1, ft2))
        {
            continue;
        }

        //CLogFile::FormatLine( "Generating thumbnail for %s...",file );

        _smart_ptr<IStatObj> obj = engine->LoadStatObjAutoRef(file.toUtf8().data(), NULL, NULL, false);
        if (obj)
        {
            assert(!"IStatObj::MakeObjectPicture does not exist anymore");
            //          obj->MakeObjectPicture( (unsigned char*)image.GetData(),thumbSize );

            CImageUtil::SaveBitmap(bmpFile, image);
            SetThumbFileTime(bmpFile, ft1);
#if defined(AZ_PLATFORM_WINDOWS)
            SetFileAttributes(bmpFile, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
#endif
            obj->Release();
        }
    }
    //GetIEditor()->ShowConsole( false );
}

void CThumbnailGenerator::GenerateForFile(const QString& fileName)
{
    return;

    I3DEngine* engine = GetIEditor()->Get3DEngine();

    int thumbSize = 128;
    CImageEx image;
    image.Allocate(thumbSize, thumbSize);

    char drive[_MAX_DRIVE];
    char fdir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char fext[_MAX_EXT];
    char bmpFile[1024];

    _splitpath_s(fileName.toUtf8().data(), drive, fdir, fname, fext);

    _makepath_s(bmpFile, drive, fdir, fname, ".tmb");
    FileTimeType ft1, ft2;
    GetThumbFileTime(fileName.toUtf8().data(), ft1);
    GetThumbFileTime(bmpFile, ft2);
    // Both cgf and bmp have same time stamp.
    if (ThumbFileTimeIsEqual(ft1, ft2))
    {
        return;
    }

    _smart_ptr<IStatObj> obj = engine->LoadStatObjAutoRef(fileName.toUtf8().data(), NULL, NULL, false);
    if (obj)
    {
        assert(!"IStatObj::MakeObjectPicture does not exist anymore");
        //      obj->MakeObjectPicture( (unsigned char*)image.GetData(),thumbSize );

        CImageUtil::SaveBitmap(bmpFile, image);
        SetThumbFileTime(bmpFile, ft1);
#if defined(AZ_PLATFORM_WINDOWS)
        SetFileAttributes(bmpFile, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
#endif
        obj->Release();
    }
}
