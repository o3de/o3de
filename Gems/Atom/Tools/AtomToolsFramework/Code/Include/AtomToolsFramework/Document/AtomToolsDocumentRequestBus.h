/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Document/AtomToolsDocumentObjectInfo.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentTypeInfo.h>
#include <AzCore/EBus/EBus.h>

namespace AtomToolsFramework
{
    //! This bus provides the most basic interface for implementing a document that works with the document system.
    //! Any extensions or application specific functionality should be added using a domain specific buses.
    class AtomToolsDocumentRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Uuid BusIdType;

        virtual ~AtomToolsDocumentRequests() = default;

        //! Get the document type info that was used to create this document.
        virtual const DocumentTypeInfo& GetDocumentTypeInfo() const = 0;

        //! Returns a container describing all reflected objects contained in a document
        virtual DocumentObjectInfoVector GetObjectInfo() const = 0;

        //! Get the unique ID of the document, which should be the same as the bus ID.
        virtual const AZ::Uuid& GetId() const = 0;

        //! Get absolute path of document
        virtual const AZStd::string& GetAbsolutePath() const = 0;

        //! Load document and related data
        //! @param loadPath absolute path of document to load
        virtual bool Open(const AZStd::string& loadPath) = 0;

        //! Reopen document preserving edits
        virtual bool Reopen() = 0;

        //! Save document to file
        virtual bool Save() = 0;

        //! Save document copy
        //! @param savePath absolute path where document is saved
        virtual bool SaveAsCopy(const AZStd::string& savePath) = 0;

        //! Save document to a new source file derived from of the open document
        //! @param savePath absolute path where document is saved
        virtual bool SaveAsChild(const AZStd::string& savePath) = 0;

        //! Close document and reset its data
        virtual bool Close() = 0;

        //! Clear document of all content
        virtual void Clear() = 0;

        //! Document is loaded
        virtual bool IsOpen() const = 0;

        //! Document has changes pending
        virtual bool IsModified() const = 0;

        //! Can the document be saved as a child or derived document
        virtual bool CanSaveAsChild() const = 0;

        //! Returns true if there are reversible modifications to the document
        virtual bool CanUndo() const = 0;

        //! Returns true if there are changes that were reversed and can be re-applied to the document
        virtual bool CanRedo() const = 0;

        //! Restores the previous state of the document
        virtual bool Undo() = 0;

        //! Restores the next state of the document
        virtual bool Redo() = 0;

        //! Signal that editing is about to begin, like beginning to drag a slider control
        virtual bool BeginEdit() = 0;

        //! Signal that editing has completed, like after releasing the mouse button after continuously dragging a slider control
        virtual bool EndEdit() = 0;
    };

    using AtomToolsDocumentRequestBus = AZ::EBus<AtomToolsDocumentRequests>;
} // namespace AtomToolsFramework
