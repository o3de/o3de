/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "PathUtil.h"

#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h> // for ebus events
#include <AzFramework/API/ApplicationAPI.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/IO/LocalFileIO.h>

#include <QRegularExpression>

namespace Path
{
    //////////////////////////////////////////////////////////////////////////
    void SplitPath(const QString& rstrFullPathFilename, QString& rstrDriveLetter, QString& rstrDirectory, QString& rstrFilename, QString& rstrExtension)
    {
        AZStd::string   strFullPathString(rstrFullPathFilename.toUtf8().data());
        AZStd::string   strDriveLetter;
        AZStd::string   strDirectory;
        AZStd::string   strFilename;
        AZStd::string   strExtension;

        char*           szPath((char*)strFullPathString.c_str());
        char*           pchLastPosition(szPath);
        char*           pchCurrentPosition(szPath);
        char*           pchAuxPosition(szPath);

        // Directory named filenames containing ":" are invalid, so we can assume if there is a :
        // it will be the drive name.
        pchCurrentPosition = strchr(pchLastPosition, ':');
        if (pchCurrentPosition == nullptr)
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
        if ((pchCurrentPosition == nullptr) && (pchAuxPosition == nullptr))
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
        if (pchCurrentPosition == nullptr)
        {
            rstrExtension = "";
            strFilename.assign(pchLastPosition);
        }
        else
        {
            strExtension.assign(pchCurrentPosition);
            strFilename.assign(pchLastPosition, pchCurrentPosition);
        }

        rstrDriveLetter = strDriveLetter.c_str();
        rstrDirectory = strDirectory.c_str();
        rstrFilename = strFilename.c_str();
        rstrExtension = strExtension.c_str();
    }
    //////////////////////////////////////////////////////////////////////////
    void GetDirectoryQueue(const QString& rstrSourceDirectory, QStringList& rcstrDirectoryTree)
    {
        AZStd::string           strCurrentDirectoryName;
        AZStd::string           strSourceDirectory(rstrSourceDirectory.toUtf8().data());
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
            if (pchCurrentPosition == nullptr)
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
        return AZ::IO::FileIOBase::GetInstance()->IsDirectory(pPath);
    }

    //////////////////////////////////////////////////////////////////////////
    QString GetUserSandboxFolder()
    {
        return QString::fromUtf8("@user@/Sandbox/");
    }

    //////////////////////////////////////////////////////////////////////////
    QString GetResolvedUserSandboxFolder()
    {
        AZ::IO::FixedMaxPath userSandboxFolderPath;
        gEnv->pFileIO->ResolvePath(userSandboxFolderPath, GetUserSandboxFolder().toUtf8().constData());
        return QString::fromUtf8(userSandboxFolderPath.c_str(), static_cast<int>(userSandboxFolderPath.Native().size()));
    }

    // internal function, you should use GetEditingGameDataFolder instead.
    AZStd::string GetGameAssetsFolder()
    {
        AZ::IO::Path projectPath;
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            settingsRegistry->Get(projectPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
        }

        return projectPath.Native();
    }

    /// Get the data folder
    AZStd::string GetEditingGameDataFolder()
    {
        // Define here the mod name
        static AZ::IO::FixedMaxPathString s_currentModName;

        if (s_currentModName.empty())
        {
            return GetGameAssetsFolder();
        }
        AZStd::string str(GetGameAssetsFolder());
        str += "Mods\\";
        str += s_currentModName.c_str();
        return str;
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

        bool relPathFound = false;
        AZStd::string relativePath;
        AZStd::string fullAssetPath(fullPath.toUtf8().data());
        EBUS_EVENT_RESULT(relPathFound, AzToolsFramework::AssetSystemRequestBus, GetRelativeProductPathFromFullSourceOrProductPath, fullAssetPath, relativePath);

        if (relPathFound)
        {
            // do not normalize this path, it will already be an appropriate asset ID.
            return CaselessPaths(relativePath.c_str());
        }

        AZ::IO::FixedMaxPath rootPath = bRelativeToGameFolder ? AZ::Utils::GetProjectPath() : AZ::Utils::GetEnginePath();
        AZ::IO::FixedMaxPath resolvedFullPath;
        AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(resolvedFullPath, fullPath.toUtf8().constData());

        // Create relative path

        return CaselessPaths(resolvedFullPath.LexicallyProximate(rootPath).MakePreferred().c_str());
    }

    QString GamePathToFullPath(const QString& path)
    {
        using namespace AzToolsFramework;
        AZ_Warning("GamePathToFullPath", path.size() <= AZ::IO::MaxPathLength, "Path exceeds maximum path length of %zu", AZ::IO::MaxPathLength);
        if (path.size() <= AZ::IO::MaxPathLength)
        {
            // first, adjust the file name for mods:
            bool fullPathFound = false;
            AZ::IO::Path assetFullPath;
            AZ::IO::Path adjustedFilePath = path.toUtf8().constData();
            AssetSystemRequestBus::BroadcastResult(fullPathFound, &AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath,
                adjustedFilePath.Native(), assetFullPath.Native());
            if (fullPathFound)
            {
                //if the bus message succeeds than normalize
                return assetFullPath.LexicallyNormal().c_str();
            }
            // if the bus message didn't succeed, check if he path exist as a resolved path
            else
            {
                // Not all systems have been converted to use local paths. Some editor files save XML files directly, and a full or correctly aliased path is already passed in.
                // If the path passed in exists already, then return the resolved filepath
                if (AZ::IO::FileIOBase::GetDirectInstance()->Exists(adjustedFilePath.c_str()))
                {
                    AZ::IO::FixedMaxPath resolvedPath;
                    AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(resolvedPath, adjustedFilePath);
                    return QString::fromUtf8(resolvedPath.c_str(), static_cast<int>(resolvedPath.Native().size()));
                }
                return path;
            }
        }
        else
        {
            return QString{};
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

