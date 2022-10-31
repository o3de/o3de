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

        if(absolutePath.empty())
        {
            return;
        }

        QString relativePath, scanFolderPath;

        if(!pathConversion->ConvertToRelativePath(absolutePath.FixedMaxPathStringAsPosix().c_str(), relativePath, scanFolderPath))
        {
            return;
        }

        auto* scanFolderInfo = pathConversion->GetScanFolderForFile(scanFolderPath);

        if(!scanFolderInfo)
        {
            return;
        }

        m_scanFolderPath = scanFolderPath.toUtf8().constData();
        m_relativePath = relativePath.toUtf8().constData();
        m_absolutePath = absolutePath;
        m_scanFolderId = scanFolderInfo->ScanFolderID();

        Normalize();
    }

    SourceAssetReference::SourceAssetReference(AZ::IO::PathView scanFolderPath, AZ::IO::PathView pathRelativeToScanFolder)
    {
        IPathConversion* pathConversion = AZ::Interface<IPathConversion>::Get();

        AZ_Assert(pathConversion, "IPathConversion interface is not available");

        if(scanFolderPath.empty() || pathRelativeToScanFolder.empty())
        {
            return;
        }

        auto* scanFolderInfo = pathConversion->GetScanFolderForFile(scanFolderPath.FixedMaxPathStringAsPosix().c_str());

        if(!scanFolderInfo)
        {
            return;
        }

        m_scanFolderPath = scanFolderPath;
        m_relativePath = pathRelativeToScanFolder;
        m_absolutePath = m_scanFolderPath / m_relativePath;
        m_scanFolderId = scanFolderInfo->ScanFolderID();

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

    bool SourceAssetReference::operator>(const SourceAssetReference& other) const
    {
        return m_absolutePath > other.m_absolutePath;
    }

    SourceAssetReference::operator bool() const
    {
        return IsValid();
    }

    bool SourceAssetReference::IsValid() const
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

    AZ::IO::Path SourceAssetReference::ScanFolderPath() const
    {
        return m_scanFolderPath;
    }

    AZ::s64 SourceAssetReference::ScanFolderId() const
    {
        return m_scanFolderId;
    }

    void SourceAssetReference::Normalize()
    {
        m_scanFolderPath = m_scanFolderPath.LexicallyNormal().AsPosix();
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
