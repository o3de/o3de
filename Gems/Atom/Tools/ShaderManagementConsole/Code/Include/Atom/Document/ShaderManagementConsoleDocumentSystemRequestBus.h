/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace ShaderManagementConsole
{
    //! ShaderManagementConsoleDocumentSystemRequestBus provides high level file requests for menus, scripts, etc.
    class ShaderManagementConsoleDocumentSystemRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Create a document object
        //! @return Uuid of new document, or null Uuid if failed
        virtual AZ::Uuid CreateDocument() = 0;

        //! Destroy a document object with the specified id
        //! @return true if Uuid was found and removed, otherwise false
        virtual bool DestroyDocument(const AZ::Uuid& documentId) = 0;

        //! Open a document for editing
        //! @param path document to edit.
        //! @return unique id of new document if successful, otherwise null Uuid
        virtual AZ::Uuid OpenDocument(AZStd::string_view path) = 0;

        //! Close the specified document
        //! @param documentId unique id of document to close
        virtual bool CloseDocument(const AZ::Uuid& documentId) = 0;

        //! Close all documents
        virtual bool CloseAllDocuments() = 0;

        //! Save the specified document
        //! @param documentId unique id of document to save
        virtual bool SaveDocument(const AZ::Uuid& documentId) = 0;

        //! Save the specified document to a different file
        //! @param documentId unique id of document to save
        virtual bool SaveDocumentAsCopy(const AZ::Uuid& documentId) = 0;

        //! Save all documents
        virtual bool SaveAllDocuments() = 0;
    };

    using ShaderManagementConsoleDocumentSystemRequestBus = AZ::EBus<ShaderManagementConsoleDocumentSystemRequests>;

} // namespace ShaderManagementConsole
