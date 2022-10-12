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

namespace AssetProcessor
{
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

        explicit SourceAssetReference(AZ::IO::PathView absolutePath)
        {
            IPathConversion* pathConversion = AZ::Interface<IPathConversion>::Get();

            AZ_Assert(pathConversion, "IPathConversion interface is not available");

            QString relativePath, scanfolderPath;
            pathConversion->ConvertToRelativePath(absolutePath.FixedMaxPathStringAsPosix().c_str(), relativePath, scanfolderPath);
            auto* scanFolderInfo = pathConversion->GetScanFolderForFile(scanfolderPath);

            AZ_Assert(scanFolderInfo, "Failed to find scanfolder for " AZ_STRING_FORMAT, AZ_STRING_ARG(absolutePath.Native()));

            m_scanfolderId = scanFolderInfo->ScanFolderID();
            m_absolutePath = absolutePath;
            m_relativePath = relativePath.toUtf8().constData();
            m_scanfolderPath = scanfolderPath.toUtf8().constData();

            Normalize();
        }

        SourceAssetReference(AZ::s64 scanFolderId, AZ::IO::PathView pathRelativeToScanFolder)
            : SourceAssetReference(AZ::Interface<IPathConversion>::Get()->GetScanFolderById(scanFolderId)->ScanPath().toUtf8().constData(), pathRelativeToScanFolder)
        {
        }

        SourceAssetReference(QString scanFolderPath, QString pathRelativeToScanFolder) : SourceAssetReference(AZ::IO::Path(scanFolderPath.toUtf8().constData()), AZ::IO::Path(pathRelativeToScanFolder.toUtf8().constData()))
        {
        }

        SourceAssetReference(const char* scanFolderPath, const char* pathRelativeToScanFolder) : SourceAssetReference(AZ::IO::Path(scanFolderPath), AZ::IO::Path(pathRelativeToScanFolder))
        {
        }

        SourceAssetReference(AZ::IO::PathView scanFolderPath, AZ::IO::PathView pathRelativeToScanFolder)
        {
            AZ_Assert(!scanFolderPath.Native().empty(), "scanfolderPath is empty");
            AZ_Assert(!pathRelativeToScanFolder.Native().empty(), "pathRelativeToScanFolder is empty");

            m_scanfolderPath = scanFolderPath;
            m_relativePath = pathRelativeToScanFolder;
            m_absolutePath = m_scanfolderPath / m_relativePath;

            IPathConversion* pathConversion = AZ::Interface<IPathConversion>::Get();

            AZ_Assert(pathConversion, "IPathConversion interface is not available");

            auto* scanFolderInfo = pathConversion->GetScanFolderForFile(m_scanfolderPath.c_str());

            AZ_Assert(scanFolderInfo, "Failed to find scanfolder for " AZ_STRING_FORMAT, AZ_STRING_ARG(m_absolutePath.Native()));

            m_scanfolderId = scanFolderInfo->ScanFolderID();

            Normalize();
        }

        AZ_DEFAULT_COPY_MOVE(SourceAssetReference);

        bool operator==(const SourceAssetReference& other) const
        {
            return m_absolutePath == other.m_absolutePath;
        }

        bool operator!=(const SourceAssetReference& other) const
        {
            return !operator==(other);
        }

        bool operator<(const SourceAssetReference& other) const
        {
            return m_absolutePath < other.m_absolutePath;
        }

        explicit operator bool() const
        {
            return !m_absolutePath.empty();
        }

        AZ::IO::Path AbsolutePath() const
        {
            return m_absolutePath;
        }

        AZ::IO::Path RelativePath() const
        {
            return m_relativePath;
        }

        AZ::IO::Path ScanfolderPath() const
        {
            return m_scanfolderPath;
        }

        AZ::s64 ScanfolderId() const
        {
            return m_scanfolderId;
        }

    private:
        void Normalize()
        {
            m_scanfolderPath = m_scanfolderPath.LexicallyNormal().AsPosix();
            m_relativePath = m_relativePath.LexicallyNormal().AsPosix();
            m_absolutePath = m_absolutePath.LexicallyNormal().AsPosix();
        }

        AZ::IO::Path m_absolutePath;
        AZ::IO::Path m_relativePath;
        AZ::IO::Path m_scanfolderPath;
        AZ::s64 m_scanfolderId{};
    };
}

namespace AZStd
{
    template<>
    struct hash<AssetProcessor::SourceAssetReference>
    {
        using argument_type = AssetProcessor::SourceAssetReference;
        using result_type = size_t;

        result_type operator()(const argument_type& obj) const
        {
            size_t h = 0;
            hash_combine(h, obj.AbsolutePath());
            return h;
        }
    };
}
