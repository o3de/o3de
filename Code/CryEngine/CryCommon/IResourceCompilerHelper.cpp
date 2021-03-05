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
#include "IResourceCompilerHelper.h"

#include <AzCore/base.h>
// DO NOT USE AZSTD.

#include <string> // std string used here.
#include <memory>
#include <cstring>

// the following block is for _mkdir on windows and mkdir on other platforms.
#if defined(_WIN32)
#   include <direct.h>
#else
#   include <sys/stat.h>
#   include <sys/types.h>
#endif

namespace RCPathUtil
{
    const char* GetExt(const char* filepath)
    {
        const char* str = filepath;
        size_t len = strlen(filepath);
        for (const char* p = str + len - 1; p >= str; --p)
        {
            switch (*p)
            {
            case ':':
            case '/':
            case '\\':
                // we've reached a path separator - it means there's no extension in this name
                return "";
            case '.':
                // there's an extension in this file name
                return p + 1;
            }
        }
        return "";
    }

    const char* GetFile(const char* filepath)
    {
        const size_t len = strlen(filepath);
        for (const char* p = filepath + len - 1; p >= filepath; --p)
        {
            switch (*p)
            {
            case ':':
            case '/':
            case '\\':
                return p + 1;
            }
        }
        return filepath;
    }


    //! Replace extension for given file.
    std::string RemoveExtension(const char* filepath)
    {
        std::string filepathstr = filepath;
        const char* str = filepathstr.c_str();
        for (const char* p = str + filepathstr.length() - 1; p >= str; --p)
        {
            switch (*p)
            {
            case ':':
            case '/':
            case '\\':
                // we've reached a path separator - it means there's no extension in this name
                return filepathstr;
            case '.':
                // there's an extension in this file name
                filepathstr = filepathstr.substr(0, p - str);
                return filepathstr;
            }
        }
        // it seems the file name is a pure name, without path or extension
        return filepathstr;
    }

    std::string ReplaceExtension(const char* filepath, const char* ext)
    {
        std::string str = filepath;
        if (ext != 0)
        {
            str = RemoveExtension(str.c_str());
            if (ext[0] != 0 && ext[0] != '.')
            {
                str += ".";
            }
            str += ext;
        }
        return str;
    }

    std::string GetPath(const char* filepath)
    {
        std::string filepathstr = filepath;
        const char* str = filepathstr.c_str();
        for (const char* p = str + filepathstr.length() - 1; p >= str; --p)
        {
            switch (*p)
            {
            case ':':
            case '/':
            case '\\':
                // we've reached a path separator - it means there's no extension in this name
                return filepathstr.substr(0, p - str);
            }
        }
        // it seems the file name is a pure name, without path
        return "";
    }


    //////////////////////////////////////////////////////////////////////////
    bool IsRelativePath(const char* p)
    {
        if (!p || !p[0])
        {
            return true;
        }
        return p[0] != '/' && p[0] != '\\' && !strchr(p, ':');
    }
}

const char* IResourceCompilerHelper::SourceImageFormatExts[NUM_SOURCE_IMAGE_TYPE] = { "tif", "bmp", "gif", "jpg", "jpeg", "jpe", "tga", "png" };
const char* IResourceCompilerHelper::SourceImageFormatExtsWithDot[NUM_SOURCE_IMAGE_TYPE] = { ".tif", ".bmp", ".gif", ".jpg", ".jpeg", ".jpe", ".tga", ".png" };
const char* IResourceCompilerHelper::EngineImageFormatExts[NUM_ENGINE_IMAGE_TYPE] = { "dds" };
const char* IResourceCompilerHelper::EngineImageFormatExtsWithDot[NUM_ENGINE_IMAGE_TYPE] = { ".dds" };


IResourceCompilerHelper::ERcCallResult IResourceCompilerHelper::ConvertResourceCompilerExitCodeToResultCode(int exitCode)
{
    switch (exitCode)
    {
    case   eRcExitCode_Success:
    case   eRcExitCode_UserFixing:
        return eRcCallResult_success;

    case   eRcExitCode_Error:
        return eRcCallResult_error;

    case   eRcExitCode_FatalError:
        return eRcCallResult_error;
    case   eRcExitCode_Crash:
        return eRcCallResult_crash;
    }
    return eRcCallResult_error;
}


//////////////////////////////////////////////////////////////////////////
const char* IResourceCompilerHelper::GetCallResultDescription(IResourceCompilerHelper::ERcCallResult result)
{
    switch (result)
    {
    case eRcCallResult_success:
        return "Success.";
    case eRcCallResult_notFound:
        return "ResourceCompiler executable was not found.";
    case eRcCallResult_error:
        return "ResourceCompiler exited with an error.";
    case eRcCallResult_crash:
        return "ResourceCompiler crashed! Please report this. Include source asset and this log in the report.";
    default:
        return "Unexpected failure in ResultCompilerHelper.";
    }
}

// Arguments:
//   szFilePath - could be source or destination filename
void IResourceCompilerHelper::GetOutputFilename(const char* szFilePath, char* buffer, size_t bufferSizeInBytes)
{
    if (IResourceCompilerHelper::IsSourceImageFormatSupported(szFilePath))
    {
        std::string newString = RCPathUtil::ReplaceExtension(szFilePath, "dds");
        azstrncpy(buffer, bufferSizeInBytes, newString.c_str(), bufferSizeInBytes - 1);
        return;
    }

    azstrncpy(buffer, bufferSizeInBytes, szFilePath, bufferSizeInBytes - 1);
}

IResourceCompilerHelper::ERcCallResult IResourceCompilerHelper::InvokeResourceCompiler(const char* szSrcFilePath, const char* szDstFilePath, const bool bUserDialog)
{
    ERcExitCode eRet = eRcExitCode_Pending;

    const char* szDstFileName = RCPathUtil::GetFile(szDstFilePath);
    std::string pathOnly = RCPathUtil::GetPath(szDstFilePath);
    const int maxStringSize = 512;
    char szRemoteCmdLine[maxStringSize] = { 0 };
    char szFullPathToSourceFile[maxStringSize] = { 0 };

    if (RCPathUtil::IsRelativePath(szSrcFilePath))
    {
        azstrcat(szFullPathToSourceFile, maxStringSize, "#ENGINEROOT#");
        azstrcat(szFullPathToSourceFile, maxStringSize, "\\");
    }
    azstrcat(szFullPathToSourceFile, maxStringSize, szSrcFilePath);

    azstrcat(szRemoteCmdLine, maxStringSize, " /targetroot=\"");
    azstrcat(szRemoteCmdLine, maxStringSize, pathOnly.c_str());
    azstrcat(szRemoteCmdLine, maxStringSize, "\"");

    azstrcat(szRemoteCmdLine, maxStringSize, " /overwritefilename=\"");
    azstrcat(szRemoteCmdLine, maxStringSize, szDstFileName);
    azstrcat(szRemoteCmdLine, maxStringSize, "\"");

    return CallResourceCompiler(szFullPathToSourceFile, szRemoteCmdLine, nullptr, true, false, !bUserDialog);
}

unsigned int IResourceCompilerHelper::GetNumSourceImageFormats()
{
    return NUM_SOURCE_IMAGE_TYPE;
}

const char* IResourceCompilerHelper::GetSourceImageFormat(unsigned int index, bool bWithDot)
{
    if (index >= GetNumSourceImageFormats())
    {
        return nullptr;
    }

    if (bWithDot)
    {
        return SourceImageFormatExtsWithDot[index];
    }
    else
    {
        return SourceImageFormatExts[index];
    }
}

unsigned int IResourceCompilerHelper::GetNumEngineImageFormats()
{
    return NUM_ENGINE_IMAGE_TYPE;
}

const char* IResourceCompilerHelper::GetEngineImageFormat(unsigned int index, bool bWithDot)
{
    if (index >= GetNumEngineImageFormats())
    {
        return nullptr;
    }

    if (bWithDot)
    {
        return EngineImageFormatExtsWithDot[index];
    }
    else
    {
        return EngineImageFormatExts[index];
    }
}

bool IResourceCompilerHelper::IsSourceImageFormatSupported(const char* szFileNameOrExtension)
{
    if (!szFileNameOrExtension) // if this hits, might want to check the call site
    {
        return false;
    }

    //check the string length
    size_t len = strlen(szFileNameOrExtension);
    if (len < 3)//no point in going on if the smallest valid ext is 3 characters
    {
        return false;
    }

    //find the ext by starting at the last character and moving backward to first he first '.'
    const char* szExtension = nullptr;
    size_t cur = len - 1;
    while (cur && !szExtension)
    {
        if (szFileNameOrExtension[cur] == '.')
        {
            szExtension = &szFileNameOrExtension[cur];
        }
        cur--;
    }
    if (len - cur < 3)//no point in going on if the smallest valid ext is 3 characters
    {
        return false;
    }

    //if we didn't find a '.' it could still be valid, they may not have
    //passed it in. i.e. "dds" instead of ".dds" which is still valid
    if (!szExtension)
    {
        //with no '.' the largest ext is currently 4 characters
        //no point in going on if it is larger
        if (len > 4)
        {
            return false;
        }

        szExtension = szFileNameOrExtension;
    }

    //loop over all the valid exts and see if it is one of them
    for (unsigned int i = 0; i < GetNumSourceImageFormats(); ++i)
    {
        if (!azstricmp(szExtension, GetSourceImageFormat(i, szExtension[0] == '.')))
        {
            return true;
        }
    }

    return false;
}

bool IResourceCompilerHelper::IsGameImageFormatSupported(const char* szFileNameOrExtension)
{
    if (!szFileNameOrExtension) // if this hits, might want to check the call site
    {
        return false;
    }

    //check the string length
    size_t len = strlen(szFileNameOrExtension);
    if (len < 3)//no point in going on if the smallest valid ext is 3 characters
    {
        return false;
    }

    //find the ext by starting at the last character and moving backward to first he first '.'
    const char* szExtension = nullptr;
    size_t cur = len - 1;
    while (cur && !szExtension)
    {
        if (szFileNameOrExtension[cur] == '.')
        {
            szExtension = &szFileNameOrExtension[cur];
        }
        cur--;
    }
    if (len - cur < 3)//no point in going on if the smallest valid ext is 3 characters
    {
        return false;
    }

    //if we didn't find a '.' it could still be valid, they may not have
    //passed it in. i.e. "dds" instead of ".dds" which is still valid
    if (!szExtension)
    {
        //with no '.' the largest ext is currently 4 characters
        //no point in going on if it is larger
        if (len > 4)
        {
            return false;
        }

        szExtension = szFileNameOrExtension;
    }

    //loop over all the valid exts and see if it is one of them
    for (unsigned int i = 0; i < GetNumEngineImageFormats(); ++i)
    {
        if (!azstricmp(szExtension, GetEngineImageFormat(i, szExtension[0] == '.')))
        {
            return true;
        }
    }

    return false;
}
