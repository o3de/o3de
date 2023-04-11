/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QString>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/Path/Path.h>
#include <utilities/IPathConversion.h>
#include <native/AssetManager/assetScanFolderInfo.h>
#include <AssetDatabase/AssetDatabaseConnection.h>

namespace AssetProcessor
{
    //! Represents a reference to a single source asset on disk
    class SourceAssetReference
    {
    public:
        SourceAssetReference() = default;

        explicit SourceAssetReference(const char* absolutePath)
            : SourceAssetReference(AZ::IO::PathView(absolutePath))
        {
        }

        explicit SourceAssetReference(QString absolutePath)
            : SourceAssetReference(absolutePath.toUtf8().constData())
        {
        }

        explicit SourceAssetReference(AZ::IO::PathView absolutePath);

        SourceAssetReference(AZ::s64 scanFolderId, AZ::IO::PathView pathRelativeToScanFolder);

        SourceAssetReference(const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
            : SourceAssetReference(sourceEntry.m_scanFolderPK, sourceEntry.m_sourceName.c_str())
        {
        }

        SourceAssetReference(QString scanFolderPath, QString pathRelativeToScanFolder)
            : SourceAssetReference(
                  AZ::IO::PathView(scanFolderPath.toUtf8().constData()), AZ::IO::PathView(pathRelativeToScanFolder.toUtf8().constData()))
        {
        }

        SourceAssetReference(const char* scanFolderPath, const char* pathRelativeToScanFolder)
            : SourceAssetReference(AZ::IO::PathView(scanFolderPath), AZ::IO::PathView(pathRelativeToScanFolder))
        {
        }

        SourceAssetReference(AZ::IO::PathView scanFolderPath, AZ::IO::PathView pathRelativeToScanFolder);

        SourceAssetReference(AZ::s64 scanFolderId, AZ::IO::PathView scanFolderPath, AZ::IO::PathView pathRelativeToScanFolder);

        AZ_DEFAULT_COPY_MOVE(SourceAssetReference);

        bool operator==(const SourceAssetReference& other) const;
        bool operator!=(const SourceAssetReference& other) const;
        bool operator<(const SourceAssetReference& other) const;
        bool operator>(const SourceAssetReference& other) const;
        explicit operator bool() const;

        bool IsValid() const;
        AZ::IO::FixedMaxPath AbsolutePath() const;
        AZ::IO::FixedMaxPath RelativePath() const;
        AZ::IO::FixedMaxPath ScanFolderPath() const;
        AZ::s64 ScanFolderId() const;

    private:
        void Normalize();

        AZ::IO::Path m_absolutePath;
        AZ::IO::Path m_relativePath;
        AZ::IO::Path m_scanFolderPath;
        AZ::s64 m_scanFolderId{};
    };
}

namespace AZStd
{
    template<>
    struct hash<AssetProcessor::SourceAssetReference>
    {
        using argument_type = AssetProcessor::SourceAssetReference;
        using result_type = size_t;

        result_type operator()(const argument_type& obj) const;
    };
}
