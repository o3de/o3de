/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include "FileUtil_Common.h"
#include "../Include/SandboxAPI.h"
#include <QString>
#include <QFileInfo>
#include "../Include/IFileUtil.h"

class QStringList;
class QMenu;

class SANDBOX_API CFileUtil
{
public:
    static bool ScanDirectory(const QString& path, const QString& fileSpec, IFileUtil::FileArray& files,
        bool recursive = true, bool addDirAlso = false, IFileUtil::ScanDirectoryUpdateCallBack updateCB = nullptr, bool bSkipPaks = false);

    static void ShowInExplorer(const QString& path);

    static bool ExtractFile(QString& file, bool bMsgBoxAskForExtraction = true, const char* pDestinationFilename = nullptr);
    static void EditTextFile(const char* txtFile, int line = 0, IFileUtil::ETextFileType fileType = IFileUtil::FILE_TYPE_SCRIPT);
    static void EditTextureFile(const char* txtureFile, bool bUseGameFolder);

    //! Reformat filter string for (MFC) CFileDialog style file filtering
    static void FormatFilterString(QString& filter);

    //! Open file selection dialog.
    static bool SelectFile(const QString& fileSpec, const QString& searchFolder, QString& fullFileName);
    //! Open file selection dialog.
    static bool SelectFiles(const QString& fileSpec, const QString& searchFolder, QStringList& files);

    //! Display OpenFile dialog and allow to select multiple files.
    //! @return true if selected, false if canceled.
    //! @outputFile Inputs and Outputs filename.
    /*
    static bool SelectSingleFile(IFileUtil::ECustomFileType fileType, QString& outputFile, const QString& filter = "", const QString& initialDir = "");
    static bool SelectSingleFile(IFileUtil::ECustomFileType fileType, char* outputFile, int outputSize, const char* filter = "", const char* initialDir = "");
    static bool SelectSingleFile(IFileUtil::ECustomFileType fileType, QString& outputFile, const QString& filter = {}, const QString& initialDir = {});
    */
    static bool SelectSaveFile(const QString& fileFilter, const QString& defaulExtension, const QString& startFolder, QString& fileName);

    //! Attempt to make a file writable
    static bool OverwriteFile(const QString& filename);

    //! Checks out the file from source control API.  Blocks until completed
    static bool CheckoutFile(const char* filename, QWidget* parentWindow = nullptr);

    //! Discard changes to a file from source control API.  Blocks until completed
    static bool RevertFile(const char* filename, QWidget* parentWindow = nullptr);

    //! Renames (moves) a file through the source control API.  Blocks until completed
    static bool RenameFile(const char* sourceFile, const char* targetFile, QWidget* parentWindow = nullptr);

    //! Deletes a file using source control API.  Blocks until completed.
    static bool DeleteFromSourceControl(const char* filename, QWidget* parentWindow = nullptr);

    //! Attempts to get the latest version of a file from source control.  Blocks until completed
    static bool GetLatestFromSourceControl(const char* filename, QWidget* parentWindow = nullptr);

    //! Gather information about a file using the source control API.  Blocks until completed
    static bool GetFileInfoFromSourceControl(const char* filename, AzToolsFramework::SourceControlFileInfo& fileInfo, QWidget* parentWindow = nullptr);

    //! Creates this directory if it doesn't exist. Returns false if the director doesn't exist and couldn't be created.
    static bool CreateDirectory(const char* dir);

    //! Makes a backup file.
    static void BackupFile(const char* filename);

    //! Makes a backup file, marked with a datestamp, e.g. myfile.20071014.093320.xml
    //! If bUseBackupSubDirectory is true, moves backup file into a relative subdirectory "backups"
    static void BackupFileDated(const char* filename, bool bUseBackupSubDirectory = false);

    // ! Added deltree as a copy from the function found in Crypak.
    static bool Deltree(const char* szFolder, bool bRecurse);

    // Checks if a file or directory exist.
    // We are using 3 functions here in order to make the names more instructive for the programmers.
    // Those functions only work for OS files and directories.
    static bool   Exists(const QString& strPath, bool boDirectory, IFileUtil::FileDesc* pDesc = nullptr);
    static bool   FileExists(const QString& strFilePath, IFileUtil::FileDesc* pDesc = nullptr);
    static bool   PathExists(const QString& strPath);
    static bool   GetDiskFileSize(const char* pFilePath, uint64& rOutSize);

    // This function should be used only with physical files.
    static bool   IsFileExclusivelyAccessable(const QString& strFilePath);

    // Creates the entire path, if needed.
    static bool   CreatePath(const QString& strPath);

    // Attempts to delete a file (if read only it will set its attributes to normal first).
    static bool   DeleteFile(const QString& strPath);

    // Attempts to remove a directory (if read only it will set its attributes to normal first).
    static bool     RemoveDirectory(const QString& strPath);

    // Calls predicate() with each entry in the directory
    static void     ForEach(const QString& path, std::function<void(const QString&)> predicate, bool recurse = true);

    //! Copies all the elements from the source directory to the target directory.
    //! It doesn't copy the source folder to the target folder, only it's contents.
    //! THIS FUNCTION IS NOT DESIGNED FOR MULTI-THREADED USAGE
    //! The ignore list can either take a single string of a file name or folder name or it can take PIPE separated names ("something|somethingelse")
    //! It is not a pattern match, merely a file or folder name match.
    static IFileUtil::ECopyTreeResult   CopyTree(const QString& strSourceDirectory, const QString& strTargetDirectory, bool boRecurse = true, bool boConfirmOverwrite = false, const char* const ignoreFilesAndFolders = nullptr);

    //////////////////////////////////////////////////////////////////////////
    // @param LPPROGRESS_ROUTINE pfnProgress - called by the system to notify of file copy progress
    // @param LPBOOL pbCancel - when the contents of this BOOL are set to true, the system cancels the copy operation
    static IFileUtil::ECopyTreeResult   CopyFile(const QString& strSourceFile, const QString& strTargetFile, bool boConfirmOverwrite = false, ProgressRoutine pfnProgress = nullptr, bool* pbCancel = nullptr);


    // As we don't have a FileUtil interface here, we have to duplicate some code :-( in order to keep
    // function calls clean.
    // Moves all the elements from the source directory to the target directory.
    // It doesn't move the source folder to the target folder, only it's contents.
    // THIS FUNCTION IS NOT DESIGNED FOR MULTI-THREADED USAGE
    static IFileUtil::ECopyTreeResult   MoveTree(const QString& strSourceDirectory, const QString& strTargetDirectory, bool boRecurse = true, bool boConfirmOverwrite = false);

    static void PopulateQMenu(QWidget* caller, QMenu* menu, AZStd::string_view fullGamePath);

    // Get file attributes include source control attributes if available
    static uint32 GetAttributes(const char* filename, bool bUseSourceControl = true);

    // Returns true if the files have the same content, false otherwise
    static bool CompareFiles(const QString& strFilePath1, const QString& strFilePath2);

    // Sort Columns( Ascending/Descending )
    static bool SortAscendingFileNames(const IFileUtil::FileDesc& desc1, const IFileUtil::FileDesc& desc2);
    static bool SortDescendingFileNames(const IFileUtil::FileDesc& desc1, const IFileUtil::FileDesc& desc2);
    static bool SortAscendingDates(const IFileUtil::FileDesc& desc1, const IFileUtil::FileDesc& desc2);
    static bool SortDescendingDates(const IFileUtil::FileDesc& desc1, const IFileUtil::FileDesc& desc2);
    static bool SortAscendingSizes(const IFileUtil::FileDesc& desc1, const IFileUtil::FileDesc& desc2);
    static bool SortDescendingSizes(const IFileUtil::FileDesc& desc1, const IFileUtil::FileDesc& desc2);

    // Return true is the filepath is a absolute path
    static bool IsAbsPath(const QString& filepath);

private:
    // True means to use the custom file dialog, false means to use the smart file open dialog.
    static bool s_singleFileDlgPref[IFileUtil::EFILE_TYPE_LAST];
    static bool s_multiFileDlgPref[IFileUtil::EFILE_TYPE_LAST];

    // Keep this variant of this method private! pIsSelected is captured in a lambda, and so requires menu use exec() and never use show()
    static void PopulateQMenu(QWidget* caller, QMenu* menu, AZStd::string_view fullGamePath, bool* pIsSelected);

    static void HandlePrefsDialogForFileType(const Common::EditFileType fileType);
    static QString GetEditorForFileTypeFromPreferences(const Common::EditFileType fileType);
    static QString HandleNoEditorAssigned(const Common::EditFileType fileType);
    static QString HandleEditorOpenFailure(const Common::EditFileType fileType, const QString& currentEditor);
    static AZStd::string GetSettingsKeyForFileType(const Common::EditFileType fileType);
    static void EditFile(const QString&, const Common::EditFileType fileType);
};

class CAutoRestorePrimaryCDRoot
{
public:
    ~CAutoRestorePrimaryCDRoot();
};

//
// A helper for creating a temp file to write to, then copying that over the destination
// file only if it changes (to avoid requiring the user to check out source controlled
// file unnecessarily)
//
class SANDBOX_API CTempFileHelper
{
public:
    CTempFileHelper(const char* pFileName);
    ~CTempFileHelper();

    // Gets the path to the temp file that should be written to
    const QString& GetTempFilePath() { return m_tempFileName; }

    // After the temp file has been written and closed, this should be called to update
    // the destination file.
    // If bBackup is true CFileUtil::BackupFile will be called if the file has changed.
    bool UpdateFile(bool bBackup);

private:
    QString m_fileName;
    QString m_tempFileName;
};
