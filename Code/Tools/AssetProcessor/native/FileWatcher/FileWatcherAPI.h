/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef FILEWATCHERAPI_H
#define FILEWATCHERAPI_H

#include <QString>
#include <QObject>
#include <QDir>

//////////////////////////////////////////////////////////////////////////
//! FileAction
/*! Enum for which file changes are tracked.
 * */
enum FileAction
{
    FileAction_None = 0x00,
    FileAction_Added = 0x01,
    FileAction_Removed = 0x02,
    FileAction_Modified = 0x04,
    FileAction_Any = 0xFF,
};
inline FileAction operator | (FileAction a, FileAction b)
{
    return static_cast<FileAction>(static_cast<int>(a) | static_cast<int>(b));
}
inline FileAction operator & (FileAction a, FileAction b)
{
    return static_cast<FileAction>(static_cast<int>(a) & static_cast<int>(b));
}

//////////////////////////////////////////////////////////////////////////
//! FileChangeInfo
/*! Struct for passing along information about file changes.
 * */
struct FileChangeInfo
{
    FileChangeInfo()
        : m_action(FileAction::FileAction_None)
    {}
    FileChangeInfo(const FileChangeInfo& rhs)
        : m_action(rhs.m_action)
        , m_filePath(rhs.m_filePath)
        , m_filePathOld(rhs.m_filePathOld)
    {
    }

    FileAction m_action;
    QString m_filePath;
    QString m_filePathOld;
};

Q_DECLARE_METATYPE(FileChangeInfo)

//////////////////////////////////////////////////////////////////////////
//! FolderWatchBase
/*! Class for filtering file changes generated from a root watch. Define your own
 *! custom filtering by deriving from this base class and implement your own
 *! custom code for what to do when receiving a file change notification.
 * */
class FolderWatchBase
    : public QObject
{
    Q_OBJECT

public:
    FolderWatchBase(const QString strFolder, bool bWatchSubtree = true, FileAction fileAction = FileAction::FileAction_Any)
        : m_folder(strFolder)
        , m_watchSubtree(bWatchSubtree)
        , m_fileAction(fileAction)
    {
        m_folder = QDir::toNativeSeparators(QDir::cleanPath(m_folder) + "/");
    }

    //! IsSubfolder(folderA, folderB)
    //! returns whether folderA is a subfolder of folderB
    //! assumptions: absolute paths, case insensitive
    static bool IsSubfolder(const QString& folderA, const QString& folderB)
    {
        // lets avoid allocating or messing with memory - this is a MAJOR hotspot as it is called for any file change even in the cache!
        int sizeB = folderB.length();
        int sizeA = folderA.length();

        if (sizeA <= sizeB)
        {
            return false;
        }

        QChar slash1 = QChar('\\');
        QChar slash2 = QChar('/');
        int posA = 0;

        // A is going to be the longer one, so use B:
        for (int idx = 0; idx < sizeB; ++idx)
        {
            QChar charAtA = folderA.at(posA);
            QChar charAtB = folderB.at(idx);

            if ((charAtB == slash1) || (charAtB == slash2))
            {
                if ((charAtA != slash1) && (charAtA != slash2))
                {
                    return false;
                }
                ++posA;
            }
            else
            {
                if (charAtA.toLower() != charAtB.toLower())
                {
                    return false;
                }
                ++posA;
            }
        }
        return true;
    }

    QString m_folder;
    bool m_watchSubtree;
    FileAction m_fileAction;

public Q_SLOTS:
    void OnAnyFileChange(FileChangeInfo info)
    {
        //if they set a file action then respect it by rejecting non matching file actions
        if (info.m_action & m_fileAction)
        {
            //is the file is in the folder or subtree (if specified) then call OnFileChange

            if (FolderWatchBase::IsSubfolder(info.m_filePath, m_folder))
            {
                OnFileChange(info);
            }
        }
    }

    virtual void OnFileChange(const FileChangeInfo& info) = 0;
};

//////////////////////////////////////////////////////////////////////////
//! FolderWatchCallbackEx
/*! Class implements a more complex filtering that can optionally filter for file
 *! extension and call different callback for different kinds of file changes
 *! generated from a root watch.
 *! Notes:
 *!  - empty extension "" catches all file changes
 *!  - extension should not include the leading "."
 * */
class FolderWatchCallbackEx
    : public FolderWatchBase
{
    Q_OBJECT

public:
    FolderWatchCallbackEx(const QString strFolder, const QString extension, bool bWatchSubtree)
        : FolderWatchBase(strFolder, bWatchSubtree)
        , m_extension(extension)
    {
    }

    QString m_extension;

    //on file change call the change callback if passes extension then route
    //to specific file action type callback
    virtual void OnFileChange(const FileChangeInfo& info)
    {
        //if they set an extension to watch for only let matching extensions through
        QFileInfo fileInfo(info.m_filePath);

        if (!m_watchSubtree)
        {
            // filter out subtrees too.
            QStringRef subRef = info.m_filePath.rightRef(info.m_filePath.length() - m_folder.length());
            if ((subRef.indexOf('/') != -1) || (subRef.indexOf('\\') != -1))
            {
                return; // filter this out.
            }

            // we don't care about subdirs.  IsDir is more expensive so we do it after the above filter.
            if (fileInfo.isDir())
            {
                return;
            }
        }

        if (m_extension.isEmpty() || fileInfo.completeSuffix().compare(m_extension, Qt::CaseInsensitive) == 0)
        {
            if (info.m_action & FileAction::FileAction_Any)
            {
                Q_EMIT fileChange(info);
            }

            if (info.m_action & FileAction::FileAction_Added)
            {
                Q_EMIT fileAdded(info.m_filePath);
            }

            if (info.m_action & FileAction::FileAction_Removed)
            {
                Q_EMIT fileRemoved(info.m_filePath);
            }

            if (info.m_action & FileAction::FileAction_Modified)
            {
                Q_EMIT fileModified(info.m_filePath);
            }
        }
    }

Q_SIGNALS:
    void fileChange(FileChangeInfo info);
    void fileAdded(QString filePath);
    void fileRemoved(QString filePath);
    void fileModified(QString filePath);
};

#endif//FILEWATCHERAPI_H
