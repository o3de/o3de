/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Vector3.h>

#include <AzToolsFramework/Thumbnails/Thumbnail.h>

#include <QCollator>
#include <QObject>
#include <QModelIndex>
#endif

class QMimeData;

namespace AzToolsFramework
{
    using namespace Thumbnailer;

    namespace AssetBrowser
    {
        class RootAssetBrowserEntry;
        class FolderAssetBrowserEntry;
        class SourceAssetBrowserEntry;
        class ProductAssetBrowserEntry;

        //! AssetBrowserEntry is a base class for asset tree view entry
        class AssetBrowserEntry
            : public QObject
        {
            friend class AssetBrowserModel;
            friend class AssetBrowserFilterModel;
            friend class RootAssetBrowserEntry;
            friend class FolderAssetBrowserEntry;
            friend class SourceAssetBrowserEntry;
            friend class ProductAssetBrowserEntry;

            Q_OBJECT
        public:
            enum class AssetEntrySortMode
            {
                Name,
                FileType,
                LastModified,
                Size,
                Vertices,
                Dimensions
            };

            enum class AssetEntryType
            {
                Root,
                Folder,
                Source,
                Product
            };

            static QString AssetEntryTypeToString(AssetEntryType assetEntryType);

            //NOTE: this list should be in sync with m_columnNames[] in the cpp file
            enum class Column
            {
                Name,
                Path,
                SourceID,
                Fingerprint,
                Guid,
                ScanFolderID,
                ProductID,
                JobID,
                SubID,
                AssetType,
                ClassID,
                DisplayName,
                Type,
                DiskSize,
                Vertices,
                ApproxSize,
                SourceControlStatus,
                Count
            };

            static const char* m_columnNames[static_cast<int>(Column::Count)];

        protected:
            AssetBrowserEntry();

        public:
            AZ_RTTI(AssetBrowserEntry, "{67679F9E-055D-43BE-A2D0-FB4720E5302A}");

            virtual ~AssetBrowserEntry();

            virtual QVariant data(int column) const;
            int row() const;

            //! @deprecated: Use "AssetBrowserEntryUtils::FromMimeData" instead
            static bool FromMimeData(const QMimeData* mimeData, AZStd::vector<const AssetBrowserEntry*>& entries);
           
            static QString GetMimeType();

            virtual AssetEntryType GetEntryType() const = 0;

            //! Actual name of the asset or folder
            const AZStd::string& GetName() const;
            //! Group name CRC provided by asset type info
            const AZ::u32 GetGroupNameCrc() const;
            //! Group name provided by asset type info
            const QString& GetGroupName() const;
            //! Display name represents how entry is shown in asset browser
            const QString& GetDisplayName() const;

            //! Display name represents how the path to the file is shown in the asset browser.
            //! It does not include the file name of the entry.
            const QString& GetDisplayPath() const;
            //! Return path relative to scan folder
            const AZStd::string& GetRelativePath() const;
            //! Return path visible to asset browser
            const AZStd::string& GetVisiblePath() const;
            //! Return absolute path to this file. Note that this decodes it to native slashes and resolves
            //! any aliases.
            const AZStd::string& GetFullPath() const;
            //! Return the size on disk of the asset
            const size_t GetDiskSize() const;
            //! Return the time the file was last modified.
            const AZ::u64 GetModificationTime() const;
            //! Returns the dimension of the model
            const AZ::Vector3& GetDimension() const;
            //! Returns the number of vertices in the model
            const uint32_t GetNumVertices() const;

            const QString& GetEntryTypeAsString() const;
            static const AZStd::string ExtensionToType(AZStd::string_view str);

            //! Recursively call the visitor function for the current entry and all of its parents.
            //! Returning false from the visitor function stops recursion.
            void VisitUp(const AZStd::function<bool(const AssetBrowserEntry*)>& visitorFn) const;

            //! Recursively call the visitor function for the current entry and all of its children.
            //! Returning false from the visitor function stops recursion.
            void VisitDown(const AZStd::function<bool(const AssetBrowserEntry*)>& visitorFn) const;

            //! Get immediate children of specific type
            template<typename EntryType>
            void GetChildren(AZStd::vector<const EntryType*>& entries) const;

            //! Recurse through the tree down to get all entries of specific type
            template<typename EntryType>
            void GetChildrenRecursively(AZStd::vector<const EntryType*>& entries) const;

            ///! Utility function:  Given a Qt QMimeData pointer, your callbackFunction will be called for each entry of that type it finds in there.
            template <typename EntryType>
            static void ForEachEntryInMimeData(const QMimeData* mimeData, AZStd::function<void(const EntryType*)> callbackFunction);

            //! Get child by index
            const AssetBrowserEntry* GetChild(int index) const;
            AssetBrowserEntry* GetChild(int index);
            //! Get number of children
            int GetChildCount() const;
            //! Get immediate parent
            AssetBrowserEntry* GetParent() const;

            virtual SharedThumbnailKey GetThumbnailKey() const;
            void SetThumbnailKey(SharedThumbnailKey thumbnailKey);
            virtual SharedThumbnailKey CreateThumbnailKey() = 0;

            void SetDisplayName(const QString name);
            void SetIconPath(const AZ::IO::Path path);
            AZ::IO::Path GetIconPath() const; 

            bool lessThan(const AssetBrowserEntry* other, const AssetBrowserEntry::AssetEntrySortMode sortColumn, const QCollator& collator) const;
            void SetFullPath(const AZ::IO::Path& fullPath);

        protected:
            AZStd::string m_name;
            AZ::u32 m_groupNameCrc{ 0 };
            QString m_groupName;
            QString m_displayName;
            QString m_displayPath;
            QString m_entryType;
            AZ::IO::Path m_relativePath;
            AZ::IO::Path m_visiblePath;
            AZ::IO::Path m_fullPath;
            AZ::IO::Path m_iconPath;
            AZ::u64 m_modificationTime;

            size_t m_diskSize;
            AZ::Vector3 m_dimension{ NAN, NAN, NAN };
            uint32_t m_vertices{ 0 };
            AZStd::vector<AssetBrowserEntry*> m_children;
            AssetBrowserEntry* m_parentAssetEntry = nullptr;

            virtual void AddChild(AssetBrowserEntry* child);
            void RemoveChild(AssetBrowserEntry* child);
            void RemoveChildren();

            //! When child is added, its paths are updated relative to this entry
            virtual void UpdateChildPaths(AssetBrowserEntry* child) const;
            virtual void PathsUpdated();

        protected:
            virtual void SetThumbnailDirty();

        private:
            SharedThumbnailKey m_thumbnailKey;
            //! index in its parent's m_children list
            int m_row = 0;

            AZ_DISABLE_COPY_MOVE(AssetBrowserEntry);
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework

Q_DECLARE_METATYPE(const AzToolsFramework::AssetBrowser::AssetBrowserEntry*)

#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.inl>
