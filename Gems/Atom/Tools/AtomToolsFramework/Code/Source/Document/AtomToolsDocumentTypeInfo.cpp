/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentTypeInfo.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AtomToolsFramework
{
    AtomToolsDocumentRequests* DocumentTypeInfo::CreateDocument(const AZ::Crc32& toolId) const
    {
        return m_documentFactoryCallback ? m_documentFactoryCallback(toolId, *this) : nullptr;
    }

    bool DocumentTypeInfo::CreateDocumentView(const AZ::Crc32& toolId, const AZ::Uuid& documentId) const
    {
        return m_documentViewFactoryCallback ? m_documentViewFactoryCallback(toolId, documentId) : false;
    }

    bool DocumentTypeInfo::IsSupportedExtensionToCreate(const AZStd::string& path) const
    {
        return IsSupportedExtension(m_supportedExtensionsToCreate, path);
    }

    bool DocumentTypeInfo::IsSupportedExtensionToOpen(const AZStd::string& path) const
    {
        return IsSupportedExtension(m_supportedExtensionsToOpen, path);
    }

    bool DocumentTypeInfo::IsSupportedExtensionToSave(const AZStd::string& path) const
    {
        return IsSupportedExtension(m_supportedExtensionsToSave, path);
    }

    bool DocumentTypeInfo::IsSupportedExtension(const DocumentExtensionInfoVector& supportedExtensions, const AZStd::string& path) const
    {
        return AZStd::any_of(
            supportedExtensions.begin(), supportedExtensions.end(),
            [&](const auto& supportedExtension)
            {
                return path.ends_with(supportedExtension.second);
            });
    }

    AZStd::string DocumentTypeInfo::GetDefaultExtensionToSave() const
    {
        auto extensionItr = m_supportedExtensionsToSave.begin();
        return extensionItr != m_supportedExtensionsToSave.end() ? extensionItr->second : AZStd::string();
    }
} // namespace AtomToolsFramework
