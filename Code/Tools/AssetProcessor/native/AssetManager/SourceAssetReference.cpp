/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/AssetManager/SourceAssetReference.h>

namespace AssetProcessor
{
    SourceAssetReference::SourceAssetReference(AZ::IO::PathView absolutePath)
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

    SourceAssetReference::SourceAssetReference(AZ::IO::PathView scanFolderPath, AZ::IO::PathView pathRelativeToScanFolder)
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

    bool SourceAssetReference::operator==(const SourceAssetReference& other) const
    {
        return m_absolutePath == other.m_absolutePath;
    }

    bool SourceAssetReference::operator!=(const SourceAssetReference& other) const
    {
        return !operator==(other);
    }

    bool SourceAssetReference::operator<(const SourceAssetReference& other) const
    {
        return m_absolutePath < other.m_absolutePath;
    }

    SourceAssetReference::operator bool() const
    {
        return !m_absolutePath.empty();
    }

    AZ::IO::Path SourceAssetReference::AbsolutePath() const
    {
        return m_absolutePath;
    }

    AZ::IO::Path SourceAssetReference::RelativePath() const
    {
        return m_relativePath;
    }

    AZ::IO::Path SourceAssetReference::ScanfolderPath() const
    {
        return m_scanfolderPath;
    }

    AZ::s64 SourceAssetReference::ScanfolderId() const
    {
        return m_scanfolderId;
    }

    void SourceAssetReference::Normalize()
    {
        m_scanfolderPath = m_scanfolderPath.LexicallyNormal().AsPosix();
        m_relativePath = m_relativePath.LexicallyNormal().AsPosix();
        m_absolutePath = m_absolutePath.LexicallyNormal().AsPosix();
    }
}

AZStd::hash<AssetProcessor::SourceAssetReference>::result_type AZStd::hash<AssetProcessor::SourceAssetReference>::operator()(const argument_type& obj) const
{
    size_t h = 0;
    hash_combine(h, obj.AbsolutePath());
    return h;
}
