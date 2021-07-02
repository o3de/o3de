/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace MaterialEditor
{
    static const char* MaterialExtension = "material";
    static const char* MaterialTypeExtension = "materialtype";

    //! MaterialDocumentSystemRequestBus provides high level file requests for menus, scripts, etc.
    class MaterialDocumentSystemRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Create a material document object
        //! @return Uuid of new material document, or null Uuid if failed
        virtual AZ::Uuid CreateDocument() = 0;

        //! Destroy a material document object with the specified id
        //! @return true if Uuid was found and removed, otherwise false
        virtual bool DestroyDocument(const AZ::Uuid& documentId) = 0;

        //! Open a material document for editing
        //! @param sourcePath material document to open.
        //! @return unique id of new material document if successful, otherwise null Uuid
        virtual AZ::Uuid OpenDocument(AZStd::string_view sourcePath) = 0;

        //! Create a new document by specifying a source and prompting the user for destination path.
        //! If the source file is a material type then this results in creating a new material based on that type.
        //! If the source file is a material this results in creating a child material with the source file as its parent.
        //! @param sourcePath material document to open.
        //! @param targetPath location where document is saved.
        //! @return unique id of new material document if successful, otherwise null Uuid
        virtual AZ::Uuid CreateDocumentFromFile(AZStd::string_view sourcePath, AZStd::string_view targetPath) = 0;

        //! Close the specified material document
        //! @param documentId unique id of material document to close
        virtual bool CloseDocument(const AZ::Uuid& documentId) = 0;

        //! Close all material documents
        virtual bool CloseAllDocuments() = 0;

        //! Close all material documents except for documentId
        //! @param documentId unique id of material document to not close
        virtual bool CloseAllDocumentsExcept(const AZ::Uuid& documentId) = 0;

        //! Save the specified material document
        //! @param documentId unique id of material document to save
        virtual bool SaveDocument(const AZ::Uuid& documentId) = 0;

        //! Save the specified material document to a different file
        //! @param documentId unique id of material document to save
        //! @param targetPath location where document is saved.
        virtual bool SaveDocumentAsCopy(const AZ::Uuid& documentId, AZStd::string_view targetPath) = 0;

        //! Save the specified material document to a different file, referencing the original material as its parent
        //! @param documentId unique id of material document to save
        //! @param targetPath location where document is saved.
        virtual bool SaveDocumentAsChild(const AZ::Uuid& documentId, AZStd::string_view targetPath) = 0;

        //! Save all material documents
        virtual bool SaveAllDocuments() = 0;
    };

    using MaterialDocumentSystemRequestBus = AZ::EBus<MaterialDocumentSystemRequests>;

} // namespace MaterialEditor
