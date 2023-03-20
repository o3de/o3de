/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "../Include/SandboxAPI.h"
#include <set>
#include <AzCore/std/containers/vector.h>

#include <QStringList>

class QWidget;

#ifdef CreateDirectory
#undef CreateDirectory
#endif

#ifdef RemoveDirectory
#undef RemoveDirectory
#endif

#ifdef CopyFile
#undef CopyFile
#endif

#ifdef MoveFile
#undef MoveFile
#endif

#ifdef DeleteFile
#undef DeleteFile
#endif

namespace AzToolsFramework
{
    struct SourceControlFileInfo;
}

typedef int (*ProgressRoutine)(
    qint64 totalFileSize,
    qint64 totalBytesTransferred,
    qint64 streamSize,
    qint64 streamBytesTransferred,
    long   streamNumber,
    long   callbackReason,
    void*  sourceFile,
    void*  destinationFile,
    void*  data
);

struct IFileUtil
{
    //! File types used for File Open dialogs
    enum ECustomFileType
    {
        EFILE_TYPE_ANY,
        EFILE_TYPE_GEOMETRY,
        EFILE_TYPE_TEXTURE,
        EFILE_TYPE_SOUND,
        EFILE_TYPE_LAST,
    };

    struct FileDesc
    {
        QString filename;
        unsigned int attrib;
        time_t  time_create;    //! -1 for FAT file systems
        time_t  time_access;    //! -1 for FAT file systems
        time_t  time_write;
        int64 size;
    };

    enum ETextFileType
    {
        FILE_TYPE_SCRIPT,
        FILE_TYPE_SHADER,
        FILE_TYPE_BSPACE,   // added back in 3.8 integration, may not end up needing this.
    };

    enum ECopyTreeResult
    {
        ETREECOPYOK,
        ETREECOPYFAIL,
        ETREECOPYUSERCANCELED,
        ETREECOPYUSERDIDNTCOPYSOMEITEMS,
    };

    struct ExtraMenuItems
    {
        QStringList names;
        int selectedIndexIfAny;

        ExtraMenuItems()
            : selectedIndexIfAny(-1) {}

        int AddItem(const QString& name)
        {
            names.push_back(name);
            return names.size() - 1;
        }
    };

    using FileArray = AZStd::vector<FileDesc>;

    typedef bool (* ScanDirectoryUpdateCallBack)(const QString& msg);

    virtual ~IFileUtil() = default;

    virtual bool ScanDirectory(const QString& path, const QString& fileSpec, FileArray& files, bool recursive = true, bool addDirAlso = false, ScanDirectoryUpdateCallBack updateCB = nullptr, bool bSkipPaks = false) = 0;

    virtual void ShowInExplorer(const QString& path) = 0;

    virtual bool ExtractFile(QString& file, bool bMsgBoxAskForExtraction = true, const char* pDestinationFilename = nullptr) = 0;
    virtual void EditTextureFile(const char* txtureFile, bool bUseGameFolder) = 0;

    //! Reformat filter string for (MFC) CFileDialog style file filtering
    virtual void FormatFilterString(QString& filter) = 0;

    virtual bool SelectSaveFile(const QString& fileFilter, const QString& defaulExtension, const QString& startFolder, QString& fileName) = 0;

    //! Attempt to make a file writable
    virtual bool OverwriteFile(const QString& filename) = 0;

    //! Checks out the file from source control API.  Blocks until completed
    virtual bool CheckoutFile(const char* filename, QWidget* parentWindow = nullptr) = 0;

    //! Discard changes to a file from source control API.  Blocks until completed
    virtual bool RevertFile(const char* filename, QWidget* parentWindow = nullptr) = 0;

    //! Renames (moves) a file through the source control API.  Blocks until completed
    virtual bool RenameFile(const char* sourceFile, const char* targetFile, QWidget* parentWindow = nullptr) = 0;

    //! Deletes a file using source control API.  Blocks until completed
    virtual bool DeleteFromSourceControl(const char* filename, QWidget* parentWindow = nullptr) = 0;

    //! Attempts to get the latest version of a file from source control.  Blocks until completed
    virtual bool GetLatestFromSourceControl(const char* filename, QWidget* parentWindow = nullptr) = 0;

    //! Gather information about a file using the source control API.  Blocks until completed
    virtual bool GetFileInfoFromSourceControl(const char* filename, AzToolsFramework::SourceControlFileInfo& fileInfo, QWidget* parentWindow = nullptr) = 0;

    //! Creates this directory.
    virtual void CreateDirectory(const char* dir) = 0;

    //! Makes a backup file.
    virtual void BackupFile(const char* filename) = 0;

    //! Makes a backup file, marked with a datestamp, e.g. myfile.20071014.093320.xml
    //! If bUseBackupSubDirectory is true, moves backup file into a relative subdirectory "backups"
    virtual void BackupFileDated(const char* filename, bool bUseBackupSubDirectory = false) = 0;

    // ! Added deltree as a copy from the function found in Crypak.
    virtual bool Deltree(const char* szFolder, bool bRecurse) = 0;

    // Checks if a file or directory exist.
    // We are using 3 functions here in order to make the names more instructive for the programmers.
    // Those functions only work for OS files and directories.
    virtual bool Exists(const QString& strPath, bool boDirectory, FileDesc* pDesc = nullptr) = 0;
    virtual bool FileExists(const QString& strFilePath, FileDesc* pDesc = nullptr) = 0;
    virtual bool PathExists(const QString& strPath) = 0;
    virtual bool GetDiskFileSize(const char* pFilePath, uint64& rOutSize) = 0;

    // This function should be used only with physical files.
    virtual bool IsFileExclusivelyAccessable(const QString& strFilePath) = 0;

    // Creates the entire path, if needed.
    virtual bool CreatePath(const QString& strPath) = 0;

    // Attempts to delete a file (if read only it will set its attributes to normal first).
    virtual bool DeleteFile(const QString& strPath) = 0;

    // Attempts to remove a directory (if read only it will set its attributes to normal first).
    virtual bool RemoveDirectory(const QString& strPath) = 0;

    // Copies all the elements from the source directory to the target directory.
    // It doesn't copy the source folder to the target folder, only it's contents.
    // THIS FUNCTION IS NOT DESIGNED FOR MULTI-THREADED USAGE
    virtual ECopyTreeResult CopyTree(const QString& strSourceDirectory, const QString& strTargetDirectory, bool boRecurse = true, bool boConfirmOverwrite = false) = 0;

    //////////////////////////////////////////////////////////////////////////
    /**
     * @brief CopyFile
     * @param strSourceFile
     * @param strTargetFile
     * @param boConfirmOverwrite
     * @param pfnProgress - called by the system to notify of file copy progress
     * @param pbCancel - when the contents of this bool are set to true, the system cancels the copy operation
     * @return
     */
    virtual ECopyTreeResult CopyFile(const QString& strSourceFile, const QString& strTargetFile, bool boConfirmOverwrite = false, ProgressRoutine pfnProgress = nullptr, bool* pbCancel = nullptr) = 0;

    // As we don't have a FileUtil interface here, we have to duplicate some code :-( in order to keep
    // function calls clean.
    // Moves all the elements from the source directory to the target directory.
    // It doesn't move the source folder to the target folder, only it's contents.
    // THIS FUNCTION IS NOT DESIGNED FOR MULTI-THREADED USAGE
    virtual ECopyTreeResult MoveTree(const QString& strSourceDirectory, const QString& strTargetDirectory, bool boRecurse = true, bool boConfirmOverwrite = false) = 0;

    virtual void GatherAssetFilenamesFromLevel(std::set<QString>& rOutFilenames, bool bMakeLowerCase = false, bool bMakeUnixPath = false) = 0;

    // Get file attributes include source control attributes if available
    virtual uint32 GetAttributes(const char* filename, bool bUseSourceControl = true) = 0;

    // Returns true if the files have the same content, false otherwise
    virtual bool CompareFiles(const QString& strFilePath1, const QString& strFilePath2) = 0;

    virtual QString GetPath(const QString& path) = 0;
};
