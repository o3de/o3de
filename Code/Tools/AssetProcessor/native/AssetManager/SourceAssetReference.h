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

        SourceAssetReference(AZ::IO::PathView scanfolderPath, AZ::IO::PathView pathRelativeToScanfolder)
        {
            m_scanfolderPath = scanfolderPath;
            m_relativePath = pathRelativeToScanfolder;
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
