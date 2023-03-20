/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>

class QWidget;

namespace AtomToolsFramework
{
    class AtomToolsDocumentRequests;

    //! DocumentTypeInfo is used to provide details about a specific document type and register it with the document system. It defines
    //! the associated document type name, a factory for creating instances of that document type, and contains filters for determining
    //! compatibility between this document type and other file types during different operations. Each document class is responsible for
    //! implementing functions that construct its document type info, which will be registered and used by the document system to guide and
    //! validate different operations.
    struct DocumentTypeInfo final
    {
        //! Function type used for instantiating a document described by this type.
        using DocumentFactoryCallback = AZStd::function<AtomToolsDocumentRequests*(const AZ::Crc32&, const DocumentTypeInfo&)>;

        //! Function type used for instantiating different views of document data.
        using DocumentViewFactoryCallback = AZStd::function<bool(const AZ::Crc32&, const AZ::Uuid&)>;

        //! A pair of strings representing a file type description and extension.
        using DocumentExtensionInfo = AZStd::pair<AZStd::string, AZStd::string>;

        //! Container of registered file types used for an action.
        using DocumentExtensionInfoVector = AZStd::vector<DocumentExtensionInfo>;

        //! Invokes the factory callback to create an instance of a document class.
        AtomToolsDocumentRequests* CreateDocument(const AZ::Crc32& toolId) const;

        //! Invokes the view factory callback to create document views.
        bool CreateDocumentView(const AZ::Crc32& toolId, const AZ::Uuid& documentId) const;

        //! Helper functions use determine if a file path or extension is supported for an operation.
        bool IsSupportedExtensionToCreate(const AZStd::string& path) const;
        bool IsSupportedExtensionToOpen(const AZStd::string& path) const;
        bool IsSupportedExtensionToSave(const AZStd::string& path) const;
        bool IsSupportedExtension(const DocumentExtensionInfoVector& supportedExtensions, const AZStd::string& path) const;

        //! Retrieves the first registered, or default, extension for saving this document type.
        AZStd::string GetDefaultExtensionToSave() const;

        //! A string used for displaying and searching for this document type.
        AZStd::string m_documentTypeName;

        //! Factory function for creating an instance of the document.
        DocumentFactoryCallback m_documentFactoryCallback;

        //! Factory function for creating views of the document.
        DocumentViewFactoryCallback m_documentViewFactoryCallback;

        //! Containers for extensions supported by each of the common operations.
        DocumentExtensionInfoVector m_supportedExtensionsToCreate;
        DocumentExtensionInfoVector m_supportedExtensionsToOpen;
        DocumentExtensionInfoVector m_supportedExtensionsToSave;

        //! Used to make the initial selection in the create document dialog.
        AZStd::string m_defaultDocumentTemplate;
    };

    using DocumentTypeInfoVector = AZStd::vector<DocumentTypeInfo>;
} // namespace AtomToolsFramework
