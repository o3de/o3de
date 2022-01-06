/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Defines namespace PathUtil for operations on files paths.


#ifndef CRYINCLUDE_CRYCOMMON_CRYPATH_H
#define CRYINCLUDE_CRYCOMMON_CRYPATH_H
#pragma once

#include <ISystem.h>
#include <AzFramework/Archive/IArchive.h>
#include <IConsole.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/StringFunc/StringFunc.h>

#include "platform.h"

#define UNIX_PATH_SEP_STR    "/"
#define UNIX_PATH_SEP_CHR    '/'
#define DOS_PATH_SEP_STR     "\\"
#define DOS_PATH_SEP_CHR     '\\'

#if AZ_LEGACY_CRYCOMMON_TRAIT_USE_UNIX_PATHS
#define CRY_NATIVE_PATH_SEPSTR  UNIX_PATH_SEP_STR
#else
#define CRY_NATIVE_PATH_SEPSTR  DOS_PATH_SEP_STR
#endif

typedef AZStd::fixed_string<512> stack_string;

namespace PathUtil
{
    const static int maxAliasLength = 32;
    inline AZStd::string GetLocalizationFolder()
    {
        return gEnv->pCryPak->GetLocalizationFolder();
    }

    inline AZStd::string GetLocalizationRoot()
    {
        return gEnv->pCryPak->GetLocalizationRoot();
    }

    //! Convert a path to the uniform form.
    inline AZStd::string ToUnixPath(const AZStd::string& strPath)
    {
        if (strPath.find(DOS_PATH_SEP_CHR) != AZStd::string::npos)
        {
            AZStd::string path = strPath;
            AZ::StringFunc::Replace(path, DOS_PATH_SEP_CHR, UNIX_PATH_SEP_CHR);
            return path;
        }
        return strPath;
    }

    //! Convert a path to the uniform form in place on stack
    inline void ToUnixPath(stack_string& rConv)
    {
        const char* const cpEnd = &(rConv.c_str()[rConv.size()]);
        char*   __restrict pC = rConv.begin();
        while (pC != cpEnd)
        {
            char c = *pC;
            if (c == DOS_PATH_SEP_CHR)
            {
                c = UNIX_PATH_SEP_CHR;
            }
            *pC++ = c;
        }
    }

    //! Convert a path to the DOS form.
    inline AZStd::string ToDosPath(const AZStd::string& strPath)
    {
        if (strPath.find(UNIX_PATH_SEP_CHR) != AZStd::string::npos)
        {
            AZStd::string path = strPath;
            AZ::StringFunc::Replace(path, UNIX_PATH_SEP_CHR, DOS_PATH_SEP_CHR);
            return path;
        }
        return strPath;
    }

    //! Convert a path to the Native form.
    inline AZStd::string ToNativePath(const AZStd::string& strPath)
    {
#if AZ_LEGACY_CRYCOMMON_TRAIT_USE_UNIX_PATHS
        return ToUnixPath(strPath);
#else
        return ToDosPath(strPath);
#endif
    }

    //! Convert a path to lowercase form
    inline AZStd::string ToLower(const AZStd::string& strPath)
    {
        AZStd::string path = strPath;
        AZStd::to_lower(path.begin(), path.end());
        return path;
    }


    //! Split full file name to path and filename
    //! @param filepath [IN] Full file name including path.
    //! @param path [OUT] Extracted file path.
    //! @param filename [OUT] Extracted file (without extension).
    //! @param ext [OUT] Extracted files extension.
    inline void Split(const AZStd::string& filepath, AZStd::string& path, AZStd::string& filename, AZStd::string& fext)
    {
        path = filename = fext = AZStd::string();
        if (filepath.empty())
        {
            return;
        }
        const char* str = filepath.c_str();
        const char* pext = str + filepath.length() - 1;
        const char* p;
        for (p = str + filepath.length() - 1; p >= str; --p)
        {
            switch (*p)
            {
            case ':':
            case '/':
            case '\\':
                path = filepath.substr(0, p - str + 1);
                filename = filepath.substr(p - str + 1, pext - p);
                return;
            case '.':
                // there's an extension in this file name
                fext = filepath.substr(p - str + 1);
                pext = p;
                break;
            }
        }
        filename = filepath.substr(p - str + 1, pext - p);
    }

    //! Split full file name to path and filename
    //! @param filepath [IN] Full file name inclusing path.
    //! @param path [OUT] Extracted file path.
    //! @param file [OUT] Extracted file (with extension).
    inline void Split(const AZStd::string& filepath, AZStd::string& path, AZStd::string& file)
    {
        AZStd::string fext;
        Split(filepath, path, file, fext);
        file += fext;
    }

    // Extract extension from full specified file path
    // Returns
    //   pointer to the extension (without .) or pointer to an empty 0-terminated AZStd::string
    inline const char* GetExt(const char* filepath)
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

    //! Extract path from full specified file path.
    inline AZStd::string GetPath(const AZStd::string& filepath)
    {
        const char* str = filepath.c_str();
        for (const char* p = str + filepath.length() - 1; p >= str; --p)
        {
            switch (*p)
            {
            case ':':
            case '/':
            case '\\':
                return filepath.substr(0, p - str + 1);
            }
        }
        return "";
    }

    //! Extract path from full specified file path.
    inline AZStd::string GetPath(const char* filepath)
    {
        return GetPath(AZStd::string(filepath));
    }

    //! Extract path from full specified file path.
    inline stack_string GetPath(const stack_string& filepath)
    {
        const char* str = filepath.c_str();
        for (const char* p = str + filepath.length() - 1; p >= str; --p)
        {
            switch (*p)
            {
            case ':':
            case '/':
            case '\\':
                return filepath.substr(0, p - str + 1);
            }
        }
        return "";
    }

    //! Extract file name with extension from full specified file path.
    inline AZStd::string GetFile(const AZStd::string& filepath)
    {
        const char* str = filepath.c_str();
        for (const char* p = str + filepath.length() - 1; p >= str; --p)
        {
            switch (*p)
            {
            case ':':
            case '/':
            case '\\':
                return filepath.substr(p - str + 1);
            }
        }
        return filepath;
    }

    inline const char* GetFile(const char* filepath)
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
    inline void RemoveExtension(AZStd::string& filepath)
    {
        const char* str = filepath.c_str();
        for (const char* p = str + filepath.length() - 1; p >= str; --p)
        {
            switch (*p)
            {
            case ':':
            case '/':
            case '\\':
                // we've reached a path separator - it means there's no extension in this name
                return;
            case '.':
                // there's an extension in this file name
                filepath = filepath.substr(0, p - str);
                return;
            }
        }
        // it seems the file name is a pure name, without path or extension
    }

    //! Replace extension for given file.
    inline void RemoveExtension(stack_string& filepath)
    {
        const char* str = filepath.c_str();
        for (const char* p = str + filepath.length() - 1; p >= str; --p)
        {
            switch (*p)
            {
            case ':':
            case '/':
            case '\\':
                // we've reached a path separator - it means there's no extension in this name
                return;
            case '.':
                // there's an extension in this file name
                filepath = filepath.substr(0, p - str);
                return;
            }
        }
        // it seems the file name is a pure name, without path or extension
    }

    //! Extract file name without extension from full specified file path.
    inline AZStd::string GetFileName(const AZStd::string& filepath)
    {
        AZStd::string file = filepath;
        RemoveExtension(file);
        return GetFile(file);
    }

    //! Removes the trailing slash or backslash from a given path.
    inline AZStd::string RemoveSlash(const AZStd::string& path)
    {
        if (path.empty() || (path[path.length() - 1] != '/' && path[path.length() - 1] != '\\'))
        {
            return path;
        }
        return path.substr(0, path.length() - 1);
    }

    //! get slash
    inline AZStd::string GetSlash()
    {
        return CRY_NATIVE_PATH_SEPSTR;
    }

    //! add a backslash if needed
    inline AZStd::string AddSlash(const AZStd::string& path)
    {
        if (path.empty() || path[path.length() - 1] == '/')
        {
            return path;
        }
        if (path[path.length() - 1] == '\\')
        {
            return path.substr(0, path.length() - 1) + "/";
        }
        return path + "/";
    }

    //! add a backslash if needed
    inline stack_string AddSlash(const stack_string& path)
    {
        if (path.empty() || path[path.length() - 1] == '/')
        {
            return path;
        }
        if (path[path.length() - 1] == '\\')
        {
            return path.substr(0, path.length() - 1) + "/";
        }
        return path + "/";
    }

    //! add a backslash if needed
    inline AZStd::string AddSlash(const char* path)
    {
        return AddSlash(AZStd::string(path));
    }

    inline stack_string ReplaceExtension(const stack_string& filepath, const char* ext)
    {
        stack_string str = filepath;
        if (ext != 0)
        {
            RemoveExtension(str);
            if (ext[0] != 0 && ext[0] != '.')
            {
                str += ".";
            }
            str += ext;
        }
        return str;
    }

    //! Replace extension for given file.
    inline AZStd::string ReplaceExtension(const AZStd::string& filepath, const char* ext)
    {
        AZStd::string str = filepath;
        if (ext != 0)
        {
            RemoveExtension(str);
            if (ext[0] != 0 && ext[0] != '.')
            {
                str += ".";
            }
            str += ext;
        }
        return str;
    }

    //! Replace extension for given file.
    inline AZStd::string ReplaceExtension(const char* filepath, const char* ext)
    {
        return ReplaceExtension(AZStd::string(filepath), ext);
    }

    //! Makes a fully specified file path from path and file name.
    inline AZStd::string Make(const AZStd::string& path, const AZStd::string& file)
    {
        return AddSlash(path) + file;
    }

    //! Makes a fully specified file path from path and file name.
    inline AZStd::string Make(const AZStd::string& dir, const AZStd::string& filename, const AZStd::string& ext)
    {
        AZStd::string path = filename;
        AZ::StringFunc::Path::ReplaceExtension(path, ext.c_str());
        path = AddSlash(dir) + path;
        return path;
    }

    //! Makes a fully specified file path from path and file name.
    inline AZStd::string Make(const AZStd::string& dir, const AZStd::string& filename, const char* ext)
    {
        return Make(dir, filename, AZStd::string(ext));
    }

    //! Makes a fully specified file path from path and file name.
    inline stack_string Make(const stack_string& path, const stack_string& file)
    {
        return AddSlash(path) + file;
    }

    //! Makes a fully specified file path from path and file name.
    inline stack_string Make(const stack_string& dir, const stack_string& filename, const stack_string& ext)
    {
        AZStd::string path = filename.c_str();
        AZ::StringFunc::Path::ReplaceExtension(path, ext.c_str());
        path = AddSlash(dir.c_str()) + path;
        return stack_string(path.c_str());
    }

    //! Makes a fully specified file path from path and file name.
    inline AZStd::string Make(const char* path, const AZStd::string& file)
    {
        return Make(AZStd::string(path), file);
    }

    //! Makes a fully specified file path from path and file name.
    inline AZStd::string Make(const AZStd::string& path, const char* file)
    {
        return Make(path, AZStd::string(file));
    }

    //! Makes a fully specified file path from path and file name.
    inline AZStd::string Make(const char path[], const char file[])
    {
        return Make(AZStd::string(path), AZStd::string(file));
    }

    //! Makes a fully specified file path from path and file name.
    inline AZStd::string Make(const char* path, const char* file, const char* ext)
    {
        return Make(AZStd::string(path), AZStd::string(file), AZStd::string(ext));
    }

    //! Makes a fully specified file path from path and file name.
    inline AZStd::string MakeFullPath(const AZStd::string& relativePath)
    {
        return relativePath;
    }

    inline AZStd::string GetParentDirectory (const AZStd::string& strFilePath, int nGeneration = 1)
    {
        for (const char* p = strFilePath.c_str() + strFilePath.length() - 2; // -2 is for the possible trailing slash: there always must be some trailing symbol which is the file/directory name for which we should get the parent
             p >= strFilePath.c_str();
             --p)
        {
            switch (*p)
            {
            case ':':
                return AZStd::string(strFilePath.c_str(), p);
            case '/':
            case '\\':
                // we've reached a path separator - return everything before it.
                if (!--nGeneration)
                {
                    return AZStd::string(strFilePath.c_str(), p);
                }
                break;
            }
        }
        // it seems the file name is a pure name, without path or extension
        return AZStd::string();
    }

    template<typename T, size_t SIZE>
    inline AZStd::basic_fixed_string<T, SIZE> GetParentDirectoryStackString(const AZStd::basic_fixed_string<T, SIZE>& strFilePath, int nGeneration = 1)
    {
        for (const char* p = strFilePath.c_str() + strFilePath.length() - 2; // -2 is for the possible trailing slash: there always must be some trailing symbol which is the file/directory name for which we should get the parent
             p >= strFilePath.c_str();
             --p)
        {
            switch (*p)
            {
            case ':':
                return AZStd::basic_fixed_string<T, SIZE> (strFilePath.c_str(), p);
            case '/':
            case '\\':
                // we've reached a path separator - return everything before it.
                if (!--nGeneration)
                {
                    return AZStd::basic_fixed_string<T, SIZE>(strFilePath.c_str(), p);
                }
                break;
            }
        }
        // it seems the file name is a pure name, without path or extension
        return AZStd::basic_fixed_string<T, SIZE>();
    }

    //////////////////////////////////////////////////////////////////////////
    // Description:
    //    Make a game correct path out of any input path.
    inline stack_string MakeGamePath(const stack_string& path)
    {
        stack_string relativePath = path;
        ToUnixPath(relativePath);

        if ((!gEnv) || (!gEnv->pFileIO))
        {
            return relativePath;
        }
        unsigned int index = 0;
        if (relativePath.length() && relativePath[index] == '@') // already aliased
        {
            if (AZ::StringFunc::Equal(relativePath.c_str(), "@products@/", false, 9))
            {
                return relativePath.substr(9); // assets is assumed.
            }
            return relativePath;
        }

        const char* rootValue = gEnv->pFileIO->GetAlias("@products@");
        if (rootValue)
        {
            stack_string rootPath(ToUnixPath(rootValue));
            if (
                (rootPath.size() > 0) &&
                (rootPath.size() < relativePath.size()) &&
                (AZ::StringFunc::Equal(relativePath.c_str(), rootPath.c_str(), false, rootPath.size()))
                )
            {
                stack_string chopped_string = relativePath.substr(rootPath.size());
                stack_string rooted = stack_string("@products@") + chopped_string;
                return rooted;
            }
        }



        return relativePath;
    }

    //////////////////////////////////////////////////////////////////////////
    // Description:
    //    Make a game correct path out of any input path.
    inline AZStd::string MakeGamePath(const AZStd::string& path)
    {
        stack_string stackPath(path.c_str());
        return MakeGamePath(stackPath).c_str();
    }

    // returns true if the AZStd::string matches the wildcard
    inline bool MatchWildcard (const char* szString, const char* szWildcard)
    {
        const char* pString = szString, * pWildcard = szWildcard;
        // skip the obviously the same starting substring
        while (*pWildcard && *pWildcard != '*' && *pWildcard != '?')
        {
            if (*pString != *pWildcard)
            {
                return false; // must be exact match unless there's a wildcard character in the wildcard string
            }
            else
            {
                ++pString, ++pWildcard;
            }
        }

        if (!*pString)
        {
            // this will only match if there are no non-wild characters in the wildcard
            for (; *pWildcard; ++pWildcard)
            {
                if (*pWildcard != '*' && *pWildcard != '?')
                {
                    return false;
                }
            }
            return true;
        }

        switch (*pWildcard)
        {
        case '\0':
            return false; // the only way to match them after the leading non-wildcard characters is !*pString, which was already checked

        // we have a wildcard with wild character at the start.
        case '*':
        {
            // merge consecutive ? and *, since they are equivalent to a single *
            while (*pWildcard == '*' || *pWildcard == '?')
            {
                ++pWildcard;
            }

            if (!*pWildcard)
            {
                return true;     // the rest of the AZStd::string doesn't matter: the wildcard ends with *
            }
            for (; *pString; ++pString)
            {
                if (MatchWildcard(pString, pWildcard))
                {
                    return true;
                }
            }

            return false;
        }
        case '?':
            return MatchWildcard(pString + 1, pWildcard + 1) || MatchWildcard(pString, pWildcard + 1);
        default:
            assert (0);
            return false;
        }
    }
};

#endif // CRYINCLUDE_CRYCOMMON_CRYPATH_H
