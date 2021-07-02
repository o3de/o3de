/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>

#include <AzCore/Outcome/Outcome.h>
#include <AzFramework/Platform/PlatformDefaults.h>

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QSet>
#include <QString>


namespace AssetBundler
{

    extern const char* DateTimeFormat;
    extern const char* ReadOnlyFileErrorMessage;

    //! Provides an abstract model that can be subclassed to create table models used to store information about files found on-disk.
    class AssetBundlerAbstractFileTableModel
        : public QAbstractTableModel
    {
    public:

        enum DataRoles
        {
            SortRole = Qt::UserRole + 1,
        };

        explicit AssetBundlerAbstractFileTableModel(QObject* parent = nullptr);
        virtual ~AssetBundlerAbstractFileTableModel() {}

        //////////////////////////////////////////////////////////////////////////
        // Pure virtual functions
        virtual AZStd::vector<AZStd::string> CreateNewFiles(
            const AZStd::string& absoluteFilePath,
            const AzFramework::PlatformFlags& platforms,
            const QString& project = QString()) = 0;
        virtual bool DeleteFile(const QModelIndex& index) = 0;
        virtual void LoadFile(
            const AZStd::string& absoluteFilePath,
            const AZStd::string& projectName = "",
            bool isDefaultFile = false) = 0;
        virtual bool WriteToDisk(const AZStd::string& key) = 0;

        //! Returns the absolute path of the file at the given index on success, returns an empty string on failure.
        virtual AZStd::string GetFileAbsolutePath(const QModelIndex& index) const = 0;

        virtual int GetFileNameColumnIndex() const = 0;
        virtual int GetTimeStampColumnIndex() const = 0;
        //////////////////////////////////////////////////////////////////////////

        //! Reload all the data based on the watched folders and files
        virtual void Reload(
            const char* fileExtension,
            const QSet<QString>& watchedFolders,
            const QSet<QString>& watchedFiles = QSet<QString>(),
            const AZStd::unordered_map<AZStd::string, AZStd::string>& pathToProjectNameMap = AZStd::unordered_map<AZStd::string, AZStd::string>());

        virtual void ReloadFiles(
            const AZStd::vector<AZStd::string>& absoluteFilePathList,
            AZStd::unordered_map<AZStd::string, AZStd::string> pathToProjectNameMap = AZStd::unordered_map<AZStd::string, AZStd::string>());

        bool Save(const QModelIndex& selectedIndex);

        bool SaveAll();

        bool HasUnsavedChanges() const;

        //////////////////////////////////////////////////////////////////////////
        // QAbstractTableModel overrides
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        //////////////////////////////////////////////////////////////////////////

    protected:
        //! Verifies input index is in range and returns the associated key.
        //! Throws an error and returns an empty string if the input index is out of range.
        AZStd::string GetFileKey(const QModelIndex& index) const;

        //! Returns an ordered list of every file key in the model.
        //! If a proxy model is not used, the order of this list will also be the display order.
        const AZStd::vector<AZStd::string>& GetAllFileKeys() const;

        //! Get the index row if the file info is stored in the model
        //! Returns -1 if the file key doesn't exist.
        int GetIndexRowByKey(const AZStd::string& key) const;

        //! Adds input key to the end of the list of all keys and notifies the view that a row has been added.
        //! When subclassing, instantiate all data associated with this key so the view can update properly.
        void AddFileKey(const AZStd::string& key);

        //! Verifies input index, signals to the view that rows will be removed, and removes the key found at the input index.
        //! When subclassing, be sure to remove all data associated with the key at this index before calling this function.
        //! Returns true if the index is successfully removed, and false on failure
        bool RemoveFileKey(const QModelIndex& index);

        //! Delete a file from the disk and remove it from the model by its key
        bool DeleteFileByKey(const AZStd::string& key);

        AZStd::unordered_set<AZStd::string> m_keysWithUnsavedChanges;

    private:
        //! When subclassing: store file information in a map, and add the keys to this vector.
        //! Provides a 1:1 mapping between a QModelIndex::row value and a key.
        AZStd::vector<AZStd::string> m_fileListKeys;
    };

} // namespace AssetBundler
