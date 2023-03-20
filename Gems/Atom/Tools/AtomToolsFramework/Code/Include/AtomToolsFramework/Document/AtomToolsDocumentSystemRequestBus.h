/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Document/AtomToolsDocumentTypeInfo.h>
#include <AzCore/EBus/EBus.h>

namespace AtomToolsFramework
{
    //! AtomToolsDocumentSystemRequestBus is an interface that provides requests for high level user interactions with a system of documents
    class AtomToolsDocumentSystemRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Crc32 BusIdType;

        //! Register a document type descriptor including factory function
        virtual void RegisterDocumentType(const DocumentTypeInfo& documentType) = 0;

        //! Get a container of all the registered document types
        virtual const DocumentTypeInfoVector& GetRegisteredDocumentTypes() const = 0;

        //! Create a document from type info and add it to the system
        //! @return Uuid of new document, or null Uuid if failed
        virtual AZ::Uuid CreateDocumentFromType(const DocumentTypeInfo& documentType) = 0;

        //! Search for document type info by name and create a document from it
        //! @return Uuid of new document, or null Uuid if failed
        virtual AZ::Uuid CreateDocumentFromTypeName(const AZStd::string& documentTypeName) = 0;

        //! Search for document type info corresponding to the extension of the path then create a document from it
        //! @return Uuid of new document, or null Uuid if failed
        virtual AZ::Uuid CreateDocumentFromFileType(const AZStd::string& path) = 0;

        //! Create a new document by opening a source as a template then saving it as a derived document at the target path 
        //! @param sourcePath document to open.
        //! @param targetPath location where document is saved.
        //! @return unique id of new document if successful, otherwise null Uuid
        virtual AZ::Uuid CreateDocumentFromFilePath(const AZStd::string& sourcePath, const AZStd::string& targetPath) = 0;

        //! Destroy a document with the specified id
        //! @return true if Uuid was found and removed, otherwise false
        virtual bool DestroyDocument(const AZ::Uuid& documentId) = 0;

        //! Open a document for editing
        //! @param sourcePath document to open.
        //! @return unique id of new document if successful, otherwise null Uuid
        virtual AZ::Uuid OpenDocument(const AZStd::string& sourcePath) = 0;

        //! Close the specified document
        //! @param documentId unique id of document to close
        virtual bool CloseDocument(const AZ::Uuid& documentId) = 0;

        //! Close all documents
        virtual bool CloseAllDocuments() = 0;

        //! Close all documents except for documentId
        //! @param documentId unique id of document to not close
        virtual bool CloseAllDocumentsExcept(const AZ::Uuid& documentId) = 0;

        //! Save the specified document
        //! @param documentId unique id of document to save
        virtual bool SaveDocument(const AZ::Uuid& documentId) = 0;

        //! Save the specified document to a different file
        //! @param documentId unique id of document to save
        //! @param targetPath location where document is saved.
        virtual bool SaveDocumentAsCopy(const AZ::Uuid& documentId, const AZStd::string& targetPath) = 0;

        //! Save the specified document to a different file, referencing the original document as its parent
        //! @param documentId unique id of document to save
        //! @param targetPath location where document is saved.
        virtual bool SaveDocumentAsChild(const AZ::Uuid& documentId, const AZStd::string& targetPath) = 0;

        //! Save all documents
        virtual bool SaveAllDocuments() = 0;

        //! Save all modified documents
        virtual bool SaveAllModifiedDocuments() = 0;

        //! Queues request to reopen modified documents.
        virtual bool QueueReopenModifiedDocuments() = 0;

        //! Process requests to reopen modified documents.
        virtual bool ReopenModifiedDocuments() = 0;

        //! Get number of allocated documents
        virtual AZ::u32 GetDocumentCount() const = 0;

        //! Determine if a document is open in the system
        //! @param documentId unique id of document to check
        virtual bool IsDocumentOpen(const AZ::Uuid& documentId) const = 0;

        //! Add a file path to the top of the list of recent file paths
        virtual void AddRecentFilePath(const AZStd::string& absolutePath) = 0;

        //! Remove all file paths from the list of recent file paths
        virtual void ClearRecentFilePaths() = 0;

        //! Replace the list of recent file paths in the settings registry
        virtual void SetRecentFilePaths(const AZStd::vector<AZStd::string>& absolutePaths) = 0;

        //! Retrieve the list of recent file paths from the settings registry
        virtual const AZStd::vector<AZStd::string> GetRecentFilePaths() const = 0;
    };

    using AtomToolsDocumentSystemRequestBus = AZ::EBus<AtomToolsDocumentSystemRequests>;

} // namespace AtomToolsFramework
