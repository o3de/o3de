/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Include/IFileUtil.h"

#ifdef DeleteFile
#undef DeleteFile
#endif

#ifdef CreateDirectory
#undef CreateDirectory
#endif

#ifdef RemoveDirectory
#undef RemoveDirectory
#endif

#ifdef CopyFile
#undef CopyFile
#endif

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
class SANDBOX_API CFileUtil_impl
    : public IFileUtil
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    bool ScanDirectory(const QString& path, const QString& fileSpec, FileArray& files, bool recursive = true, bool addDirAlso = false, ScanDirectoryUpdateCallBack updateCB = nullptr, bool bSkipPaks = false) override;

    void ShowInExplorer(const QString& path) override;

    bool ExtractFile(QString& file, bool bMsgBoxAskForExtraction = true, const char* pDestinationFilename = nullptr) override;
    void EditTextureFile(const char* txtureFile, bool bUseGameFolder) override;

    //! Reformat filter string for (MFC) CFileDialog style file filtering
    void FormatFilterString(QString& filter) override;

    bool SelectSaveFile(const QString& fileFilter, const QString& defaulExtension, const QString& startFolder, QString& fileName) override;

    //! Attempt to make a file writable
    bool OverwriteFile(const QString& filename) override;

    //! Checks out the file from source control API.  Blocks until completed
    bool CheckoutFile(const char* filename, QWidget* parentWindow = nullptr) override;

    //! Discard changes to a file from source control API.  Blocks until completed
    bool RevertFile(const char* filename, QWidget* parentWindow = nullptr) override;

    //! Renames (moves) a file through the source control API.  Blocks until completed
    bool RenameFile(const char* sourceFile, const char* targetFile, QWidget* parentWindow = nullptr) override;

    //! Deletes a file using source control API.  Blocks until completed.
    bool DeleteFromSourceControl(const char* filename, QWidget* parentWindow = nullptr) override;

    //! Attempts to get the latest version of a file from source control.  Blocks until completed
    bool GetLatestFromSourceControl(const char* filename, QWidget* parentWindow = nullptr) override;

    //! Gather information about a file using the source control API.  Blocks until completed
    bool GetFileInfoFromSourceControl(const char* filename, AzToolsFramework::SourceControlFileInfo& fileInfo, QWidget* parentWindow = nullptr) override;

    //! Creates this directory.
    void CreateDirectory(const char* dir) override;

    //! Makes a backup file.
    void BackupFile(const char* filename) override;

    //! Makes a backup file, marked with a datestamp, e.g. myfile.20071014.093320.xml
    //! If bUseBackupSubDirectory is true, moves backup file into a relative subdirectory "backups"
    void BackupFileDated(const char* filename, bool bUseBackupSubDirectory = false) override;

    // ! Added deltree as a copy from the function found in Crypak.
    bool Deltree(const char* szFolder, bool bRecurse) override;

    // Checks if a file or directory exist.
    // We are using 3 functions here in order to make the names more instructive for the programmers.
    // Those functions only work for OS files and directories.
    bool Exists(const QString& strPath, bool boDirectory, FileDesc* pDesc = nullptr) override;
    bool FileExists(const QString& strFilePath, FileDesc* pDesc = nullptr) override;
    bool PathExists(const QString& strPath) override;
    bool GetDiskFileSize(const char* pFilePath, uint64& rOutSize) override;

    // This function should be used only with physical files.
    bool IsFileExclusivelyAccessable(const QString& strFilePath) override;

    // Creates the entire path, if needed.
    bool CreatePath(const QString& strPath) override;

    // Attempts to delete a file (if read only it will set its attributes to normal first).
    bool DeleteFile(const QString& strPath) override;

    // Attempts to remove a directory (if read only it will set its attributes to normal first).
    bool RemoveDirectory(const QString& strPath) override;

    // Copies all the elements from the source directory to the target directory.
    // It doesn't copy the source folder to the target folder, only it's contents.
    // THIS FUNCTION IS NOT DESIGNED FOR MULTI-THREADED USAGE
    ECopyTreeResult CopyTree(const QString& strSourceDirectory, const QString& strTargetDirectory, bool boRecurse = true, bool boConfirmOverwrite = false) override;

    //////////////////////////////////////////////////////////////////////////
    // @param LPPROGRESS_ROUTINE pfnProgress - called by the system to notify of file copy progress
    // @param LPBOOL pbCancel - when the contents of this BOOL are set to true, the system cancels the copy operation
    ECopyTreeResult CopyFile(const QString& strSourceFile, const QString& strTargetFile, bool boConfirmOverwrite = false, ProgressRoutine pfnProgress = nullptr, bool* pbCancel = nullptr) override;

    // As we don't have a FileUtil interface here, we have to duplicate some code :-( in order to keep
    // function calls clean.
    // Moves all the elements from the source directory to the target directory.
    // It doesn't move the source folder to the target folder, only it's contents.
    // THIS FUNCTION IS NOT DESIGNED FOR MULTI-THREADED USAGE
    ECopyTreeResult MoveTree(const QString& strSourceDirectory, const QString& strTargetDirectory, bool boRecurse = true, bool boConfirmOverwrite = false) override;

    // Get file attributes include source control attributes if available
    uint32 GetAttributes(const char* filename, bool bUseSourceControl = true) override;

    // Returns true if the files have the same content, false otherwise
    bool CompareFiles(const QString& strFilePath1, const QString& strFilePath2) override;

    // Extract path from full specified file path.
    QString GetPath(const QString& path) override;
};

