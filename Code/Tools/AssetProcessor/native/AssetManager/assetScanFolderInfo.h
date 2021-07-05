/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef ASSETSCANFOLDERINFO_H
#define ASSETSCANFOLDERINFO_H

#include <QString>
#include <QDateTime>
#include <AzCore/base.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace AssetProcessor
{
    /** This Class contains information about the folders to be scanned
     * */
    class ScanFolderInfo
    {
    public:
        ScanFolderInfo(
            QString path,
            QString displayName,
            QString portableKey,
            bool isRoot,
            bool recurseSubFolders,
            AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms = AZStd::vector<AssetBuilderSDK::PlatformInfo>{},
            int order = 0,
            AZ::s64 scanFolderID = 0,
            bool canSaveNewAssets = false);

        ScanFolderInfo() = default;
        ScanFolderInfo(const ScanFolderInfo& other) = default;

        QString ScanPath() const
        {
            return m_scanPath;
        }

        QString GetDisplayName() const
        {
            return m_displayName;
        }

        bool IsRoot() const
        {
            return m_isRoot;
        }

        bool RecurseSubFolders() const
        {
            return m_recurseSubFolders;
        }

        bool CanSaveNewAssets() const
        {
            return m_canSaveNewAssets;
        }

        int GetOrder() const
        {
            return m_order;
        }

        AZ::s64 ScanFolderID() const
        {
            return m_scanFolderID;
        }

        QString GetPortableKey() const
        {
            return m_portableKey;
        }

        const AZStd::vector<AssetBuilderSDK::PlatformInfo>& GetPlatforms() const
        {
            return m_platforms;
        }

        void SetScanFolderID(AZ::s64 scanFolderID)
        {
            m_scanFolderID = scanFolderID;
        }

    private:
        QString m_scanPath; // the local path to scan ("C:\\whatever")
        QString m_displayName; // the display name to show in GUIs that show it.
        QString m_portableKey; // a key that remains the same even if the asset database is moved from computer to computer.
        bool m_isRoot = false; // is it 'the' root folder?
        bool m_recurseSubFolders = true;
        bool m_canSaveNewAssets = false; // Tracks if it is safe to save new assets in this folder.
        int m_order = 0;
        AZ::s64 m_scanFolderID = 0; // this is filled in by the database - don't modify it.
        AZStd::vector<AssetBuilderSDK::PlatformInfo> m_platforms; // This contains the list of platforms that are enabled for the particular scanfolder
    };

    struct AssetFileInfo
    {
        AssetFileInfo() = default;
        AssetFileInfo(QString filePath, QDateTime modTime, AZ::u64 fileSize, const ScanFolderInfo* scanFolder, bool isDirectory)
            : m_filePath(filePath), m_modTime(modTime), m_fileSize(fileSize), m_scanFolder(scanFolder), m_isDirectory(isDirectory) {}

        bool operator==(const AssetFileInfo& rhs) const
        {
            return m_filePath == rhs.m_filePath
                && m_modTime == rhs.m_modTime
                && m_fileSize == rhs.m_fileSize
                && m_isDirectory == rhs.m_isDirectory;
            // m_scanFolder ignored since m_filePath will already ensure this is the same file
        }

        QString m_filePath{}; // Absolute path of the file
        QDateTime m_modTime{};
        AZ::u64 m_fileSize{};
        const ScanFolderInfo* m_scanFolder{};
        bool m_isDirectory{};
    };

    inline uint qHash(const AssetFileInfo& item)
    {
        return qHash(item.m_filePath);
    }
} // end namespace AssetProcessor

#endif //ASSETSCANFOLDERINFO_H
