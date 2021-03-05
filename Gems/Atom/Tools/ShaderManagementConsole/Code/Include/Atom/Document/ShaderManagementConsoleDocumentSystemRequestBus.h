/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
