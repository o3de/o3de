/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "PathUtil.h"

#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN
#include <AzToolsFramework/API/EditorAssetSystemAPI.h> // for ebus events
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>

#include <QRegularExpression>

namespace
{
    string g_currentModName; // folder name only!
}

namespace Path
{
    //////////////////////////////////////////////////////////////////////////
    void SplitPath(const QString& rstrFullPathFilename, QString& rstrDriveLetter, QString& rstrDirectory, QString& rstrFilename, QString& rstrExtension)
    {
        string          strFullPathString(rstrFullPathFilename.toUtf8().data());
        string          strDriveLetter;
        string          strDirectory;
        string          strFilename;
        string          strExtension;

        char*           szPath((char*)strFullPathString.c_str());
        char*           pchLastPosition(szPath);
        char*           pchCurrentPosition(szPath);
        char*           pchAuxPosition(szPath);

        // Directory named filenames containing ":" are invalid, so we can assume if there is a :
        // it will be the drive name.
        pchCurrentPosition = strchr(pchLastPosition, ':');
        if (pchCurrentPosition == NULL)
        {
            rstrDriveLetter = "";
        }
        else
        {
            strDriveLetter.assign(pchLastPosition, pchCurrentPosition + 1);
            pchLastPosition = pchCurrentPosition + 1;
        }

        pchCurrentPosition = strrchr(pchLastPosition, '\\');
        pchAuxPosition = strrchr(pchLastPosition, '/');
        if ((pchCurrentPosition == NULL) && (pchAuxPosition == NULL))
        {
            rstrDirectory = "";
        }
        else
        {
            // Since NULL is < valid pointer, so this will work.
            if (pchAuxPosition > pchCurrentPosition)
            {
                pchCurrentPosition = pchAuxPosition;
            }
            strDirectory.assign(pchLastPosition, pchCurrentPosition + 1);
            pchLastPosition = pchCurrentPosition + 1;
        }

        pchCurrentPosition = strrchr(pchLastPosition, '.');
        if (pchCurrentPosition == NULL)
        {
            rstrExtension = "";
            strFilename.assign(pchLastPosition);
        }
        else
        {
            strExtension.assign(pchCurrentPosition);
            strFilename.assign(pchLastPosition, pchCurrentPosition);
        }

        rstrDriveLetter = strDriveLetter;
        rstrDirectory = strDirectory;
        rstrFilename = strFilename;
        rstrExtension = strExtension;
    }
    //////////////////////////////////////////////////////////////////////////
    void GetDirectoryQueue(const QString& rstrSourceDirectory, QStringList& rcstrDirectoryTree)
    {
        string                      strCurrentDirectoryName;
        string                      strSourceDirectory(rstrSourceDirectory.toUtf8().data());
        const char*             szSourceDirectory(strSourceDirectory.c_str());
        const char*             pchCurrentPosition(szSourceDirectory);
        const char*             pchLastPosition(szSourceDirectory);

        rcstrDirectoryTree.clear();

        if (strSourceDirectory.empty())
        {
            return;
        }

        // It removes as many slashes the path has in its start...
        // MAYBE and just maybe we should consider paths starting with
        // more than 2 slashes invalid paths...
        while ((*pchLastPosition == '\\') || (*pchLastPosition == '/'))
        {
            ++pchLastPosition;
            ++pchCurrentPosition;
        }

        do
        {
            pchCurrentPosition = strpbrk(pchLastPosition, "\\/");
            if (pchCurrentPosition == NULL)
            {
                break;
            }
            strCurrentDirectoryName.assign(pchLastPosition, pchCurrentPosition);
            pchLastPosition = pchCurrentPosition + 1;
            // Again, here we are skipping as many consecutive slashes.
            while ((*pchLastPosition == '\\') || (*pchLastPosition == '/'))
            {
                ++pchLastPosition;
            }


            rcstrDirectoryTree.push_back(strCurrentDirectoryName.c_str());
        } while (true);
    }
    //////////////////////////////////////////////////////////////////////////
    void ConvertSlashToBackSlash(QString& rstrStringToConvert)
    {
        rstrStringToConvert.replace('/', '\\');
        rstrStringToConvert = CaselessPaths(rstrStringToConvert);
    }
    //////////////////////////////////////////////////////////////////////////
    void ConvertBackSlashToSlash(QString& rstrStringToConvert)
    {
        rstrStringToConvert.replace('\\', '/');
        rstrStringToConvert = CaselessPaths(rstrStringToConvert);
    }
    //////////////////////////////////////////////////////////////////////////
    void SurroundWithQuotes(QString& rstrSurroundString)
    {
        QString strSurroundString(rstrSurroundString);

        if (!strSurroundString.isEmpty())
        {
            if (strSurroundString[0] != '\"')
            {
                strSurroundString.insert(0, "\"");
            }
            if (strSurroundString[strSurroundString.size() - 1] != '\"')
            {
                strSurroundString.insert(strSurroundString.size(), "\"");
            }
        }
        else
        {
            strSurroundString.insert(0, "\"");
            strSurroundString.insert(strSurroundString.size(), "\"");
        }
        rstrSurroundString = strSurroundString;
    }
    //////////////////////////////////////////////////////////////////////////
    QString GetExecutableFullPath()
    {
        return QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    }
    //////////////////////////////////////////////////////////////////////////
    QString GetWindowsTempDirectory()
    {
        return QDir::tempPath();
    }
    //////////////////////////////////////////////////////////////////////////
    QString GetEngineRootPath()
    {
        const char* engineRoot;
        EBUS_EVENT_RESULT(engineRoot, AzFramework::ApplicationRequests::Bus, GetEngineRoot);
        return QString(engineRoot);
    }
    
    //////////////////////////////////////////////////////////////////////////
    QString& ReplaceFilename(const QString& strFilepath, const QString& strFilename, QString& strOutputFilename, bool bCallCaselessPath)
    {
        QString strDriveLetter;
        QString strDirectory;
        QString strOriginalFilename;
        QString strExtension;

        SplitPath(strFilepath, strDriveLetter, strDirectory, strOriginalFilename, strExtension);

        strOutputFilename = strDriveLetter;
        strOutputFilename += strDirectory;
        strOutputFilename += strFilename;
        strOutputFilename += strExtension;

        if (bCallCaselessPath)
        {
            strOutputFilename = CaselessPaths(strOutputFilename);
        }
        return strOutputFilename;
    }

    bool IsFolder(const char* pPath)
    {
        DWORD attrs = GetFileAttributes(pPath);

        if (attrs == FILE_ATTRIBUTE_DIRECTORY)
        {
            return true;
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    QString GetUserSandboxFolder()
    {
        return QString::fromUtf8("@user@/Sandbox/");
    }

    //////////////////////////////////////////////////////////////////////////
    QString GetResolvedUserSandboxFolder()
    {
        char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
        gEnv->pFileIO->ResolvePath(GetUserSandboxFolder().toUtf8().data(), resolvedPath, AZ_MAX_PATH_LEN);
        return QString::fromLatin1(resolvedPath);
    }

    // internal function, you should use GetEditingGameDataFolder instead.
    AZStd::string GetGameAssetsFolder()
    {
        const char* resultValue = nullptr;
        EBUS_EVENT_RESULT(resultValue, AzToolsFramework::AssetSystemRequestBus, GetAbsoluteDevGameFolderPath);
        if (!resultValue)
        {
            if ((gEnv) && (gEnv->pFileIO))
            {
                resultValue = gEnv->pFileIO->GetAlias("@devassets@");
            }
        }

        if (!resultValue)
        {
            resultValue = ".";
        }

        return resultValue;
    }

    /// Get the data folder
    AZStd::string GetEditingGameDataFolder()
    {
        // query the editor root.  The bus exists in case we want tools to be able to override this.


        if (g_currentModName.empty())
        {
            return GetGameAssetsFolder();
        }
        AZStd::string str(GetGameAssetsFolder());
        str += "Mods\\";
        str += g_currentModName;
        return str;
    }

    //! Get the root folder (in source control or other writable assets) where you should save root data.
    AZStd::string GetEditingRootFolder()
    {
        const char* resultValue = nullptr;
        EBUS_EVENT_RESULT(resultValue, AzToolsFramework::AssetSystemRequestBus, GetAbsoluteDevRootFolderPath);

        if (!resultValue)
        {
            if ((gEnv) && (gEnv->pFileIO))
            {
                resultValue = gEnv->pFileIO->GetAlias("@devassets@");
            }
        }
        if (!resultValue)
        {
            resultValue = ".";
        }
        return resultValue;
    }


    AZStd::string MakeModPathFromGamePath(const char* relGamePath)
    {
        return GetEditingGameDataFolder() + "\\" + relGamePath;
    }

    QString FullPathToLevelPath(const QString& path)
    {
        if (path.isEmpty())
        {
            return "";
        }

        QString relGamePath;

        if (!QFileInfo(path).isRelative())
        {
            relGamePath = GetRelativePath(path);
        }
        else
        {
            relGamePath = path;
        }

        QString levelpath = GetIEditor()->GetLevelFolder();
        QString str = levelpath;
        str.replace('/', '\\');
        levelpath = CaselessPaths(str);

        // Create relative path
        QString relLevelPath = QDir(levelpath).relativeFilePath(relGamePath);
        if (relLevelPath.isEmpty())
        {
            assert(0);
            return path;
        }

        relLevelPath.remove(QRegularExpression(QStringLiteral(R"(^[\\/.]*)")));
        return relLevelPath;
    }

    QString Make(const QString& path, const QString& file)
    {
        if (gEnv->pCryPak->IsAbsPath(file.toUtf8().data()))
        {
            return file;
        }
        return CaselessPaths(AddPathSlash(path) + file);
    }

    QString GetRelativePath(const QString& fullPath, bool bRelativeToGameFolder /*= false*/)
    {
        if (fullPath.isEmpty())
        {
            return "";
        }

        bool relPathfound = false;
        AZStd::string relativePath;
        AZStd::string fullAssetPath(fullPath.toUtf8().data());
        EBUS_EVENT_RESULT(relPathfound, AzToolsFramework::AssetSystemRequestBus, GetRelativeProductPathFromFullSourceOrProductPath, fullAssetPath, relativePath);

        if (relPathfound)
        {
            // do not normalize this path, it will already be an appropriate asset ID.
            return CaselessPaths(relativePath.c_str());
        }

        char rootpath[_MAX_PATH] = { 0 };
        azstrcpy(rootpath, _MAX_PATH, Path::GetEditingRootFolder().c_str());

        if (bRelativeToGameFolder)
        {
            azstrcpy(rootpath, _MAX_PATH, Path::GetEditingGameDataFolder().c_str());
        }

        QString rootPathNormalized(rootpath);
        QString srcPathNormalized(fullPath);

#if defined(AZ_PLATFORM_WINDOWS)
        // avoid confusing PathRelativePathTo
        rootPathNormalized.replace('/', '\\');
        srcPathNormalized.replace('/', '\\');
#endif

        // Create relative path
        char resolvedSrcPath[AZ_MAX_PATH_LEN] = { 0 };
        AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(srcPathNormalized.toUtf8().data(), resolvedSrcPath, AZ_MAX_PATH_LEN);
        QByteArray path = QDir(rootPathNormalized).relativeFilePath(resolvedSrcPath).toUtf8();
        if (path.isEmpty())
        {
            return fullPath;
        }

        // The following code is required because the windows PathRelativePathTo function will always return "./SomePath" instead of just "SomePath"
        // Only remove single dot (.) and slash parts of a path, never the double dot (..)
        const char* pBuffer = path.data();
        bool bHasDot = false;
        while (*pBuffer && pBuffer != path.end())
        {
            switch (*pBuffer)
            {
            case '.':
                if (bHasDot)
                {
                    // Found a double dot, rewind and stop removing
                    pBuffer--;
                    break;
                }
            // Fall through intended
            case '/':
            case '\\':
                bHasDot = (*pBuffer == '.');
                pBuffer++;
                continue;
            }
            break;
        }

        QString relPath = pBuffer;
        return CaselessPaths(relPath);
    }

    QString GamePathToFullPath(const QString& path)
    {
        using namespace AzToolsFramework;
        AZ_Warning("GamePathToFullPath", path.size() <= AZ_MAX_PATH_LEN, "Path exceeds maximum path length of %d", AZ_MAX_PATH_LEN);
        if ((gEnv) && (gEnv->pFileIO) && gEnv->pCryPak && path.size() <= AZ_MAX_PATH_LEN)
        {
            // first, adjust the file name for mods:
            bool fullPathfound = false;
            AZStd::string assetFullPath;
            AZStd::string adjustedFilePath = path.toUtf8().data();
            AssetSystemRequestBus::BroadcastResult(fullPathfound, &AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, adjustedFilePath, assetFullPath);
            if (fullPathfound)
            {
                //if the bus message succeeds than normalize and lowercase the path
                AzFramework::StringFunc::Path::Normalize(assetFullPath);
                return assetFullPath.c_str();
            }
            // if the bus message didn't succeed, 'guess' the source assets:
            else
            {
                // Not all systems have been converted to use local paths. Some editor files save XML files directly, and a full or correctly aliased path is already passed in.
                // If the path passed in exists already, then return the resolved filepath
                if (AZ::IO::FileIOBase::GetDirectInstance()->Exists(adjustedFilePath.c_str()))
                {
                    char resolvedPath[AZ_MAX_PATH_LEN + PathUtil::maxAliasLength] = { 0 };
                    AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(adjustedFilePath.c_str(), resolvedPath, AZ_MAX_PATH_LEN + PathUtil::maxAliasLength);
                    return QString::fromUtf8(resolvedPath);
                }
                // if we get here it means that the Asset Processor does not know about this file.  most of the time we should never get here
                // the rest of this code just does a bunch of heuristic guesses in case of missing files or if the user has hand-edited
                // the asset cache by moving files in via some other means or external process.
                if (adjustedFilePath[0] != '@')
                {
                    const char* prefix = (adjustedFilePath[0] == '/' || adjustedFilePath[0] == '\\') ? "@devassets@" : "@devassets@/";
                    adjustedFilePath = prefix + adjustedFilePath;
                }

                char szAdjustedFile[AZ_MAX_PATH_LEN + PathUtil::maxAliasLength] = { 0 };
                gEnv->pFileIO->ResolvePath(adjustedFilePath.c_str(), szAdjustedFile, AZ_ARRAY_SIZE(szAdjustedFile));

                if ((azstrnicmp(szAdjustedFile, "@devassets@", 11) == 0) && ((szAdjustedFile[11] == '/') || (szAdjustedFile[11] == '\\')))
                {
                    if (!gEnv->pCryPak->IsFileExist(szAdjustedFile))
                    {
                        AZStd::string newName(szAdjustedFile);
                        AzFramework::StringFunc::Replace(newName, "@devassets@", "@devroot@/engine", false);
                        
                        if (gEnv->pCryPak->IsFileExist(newName.c_str()))
                        {
                            azstrcpy(szAdjustedFile, AZ_ARRAY_SIZE(szAdjustedFile), newName.c_str());
                        }
                        else
                        {
                            // getting tricky here, try @devroot@ alone, in case its 'editor'
                            AzFramework::StringFunc::Replace(newName, "@devassets@", "@devroot@", false);
                            if (gEnv->pCryPak->IsFileExist(szAdjustedFile))
                            {
                                azstrcpy(szAdjustedFile, AZ_ARRAY_SIZE(szAdjustedFile), newName.c_str());
                            }
                            // give up, best guess is just @devassets@
                        }
                    }
                }

                // we should very rarely actually get to this point in the code.

                // szAdjustedFile may contain an alias at this point. (@assets@/blah.whatever)
                // there is a case in which the loose asset exists only within a pak file for some reason
                // this is not recommended but it is possible.in that case, we want to return the original szAdjustedFile
                // without touching it or resolving it so that crypak can open it successfully.
                char adjustedPath[AZ_MAX_PATH_LEN + PathUtil::maxAliasLength] = { 0 };
                if (gEnv->pFileIO->ResolvePath(szAdjustedFile, adjustedPath, AZ_MAX_PATH_LEN + PathUtil::maxAliasLength)) // resolve to full path
                {
                    if ((gEnv->pCryPak->IsFileExist(adjustedPath)) || (!gEnv->pCryPak->IsFileExist(szAdjustedFile)))
                    {
                        // note that if we get here, then EITHER
                        // the file exists as a loose asset in the actual adjusted path
                        // OR the file does not exist in the original passed-in aliased name (like '@assets@/whatever')
                        // in which case we may as well just resolve the path to a full path and return it.
                        assetFullPath = adjustedPath;
                        AzFramework::StringFunc::Path::Normalize(assetFullPath);
                        azstrcpy(szAdjustedFile, AZ_MAX_PATH_LEN + PathUtil::maxAliasLength, assetFullPath.c_str());
                    }
                    // if the above case succeeded then it means that the file does NOT exist loose
                    // but DOES exist in a pak, in which case we leave szAdjustedFile with the alias on the front of it, meaning
                    // fopens via crypak will actually succeed.
                }
                return szAdjustedFile;
            }
        }
        else
        {
            return "";
        }
    }

    QString ToUnixPath(const QString& strPath, bool bCallCaselessPath)
    {
        QString str = strPath;
        str.replace('\\', '/');
        return bCallCaselessPath ? CaselessPaths(str) : str;
    }

    QString RemoveBackslash(QString path)
    {
        if (path.isEmpty())
        {
            return path;
        }

        int iLenMinus1 = path.length() - 1;
        QChar cLastChar = path[iLenMinus1];

        if (cLastChar == '\\' || cLastChar == '/')
        {
            return CaselessPaths(path.mid(0, iLenMinus1));
        }

        return CaselessPaths(path);
    }

    QString SubDirectoryCaseInsensitive(const QString& path, const QStringList& parts)
    {
        if (parts.isEmpty())
        {
            return path;
        }

        QStringList modifiedParts = parts;
        auto currentPart = modifiedParts.takeFirst();

        // case insensitive iterator
        QDirIterator it(path);
        while (it.hasNext())
        {
            it.next();
            // the current part already exists, use it, case doesn't matter
            auto actualName = it.fileName();
            if (QString::compare(actualName, currentPart, Qt::CaseInsensitive) == 0)
            {
                return SubDirectoryCaseInsensitive(QDir(path).absoluteFilePath(actualName), modifiedParts);
            }
        }
        // the current path doesn't exist yet, so just create the complete path in one rush
        return QDir(path).absoluteFilePath(parts.join('/'));
    }
}

