/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/any.h>
#include <AtomToolsFramework/DynamicProperty/DynamicProperty.h>
#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>
#include <AzCore/EBus/EBus.h>

namespace AtomToolsFramework
{
    //! Structure used as an opaque description of objects reflected inside of a document.
    //! An example use would be semi automatic handling of serialization or reflection to a property editor.
    struct DocumentObjectInfo
    {
        bool m_visible = true;
        AZStd::string m_name;
        AZStd::string m_displayName;
        AZStd::string m_description;
        AZ::Uuid m_objectType = AZ::Uuid::CreateNull();
        void* m_objectPtr = {};
    };

    //! This bus provides the most basic interface for implementing a document that works with the document system.
    //! Any extensions or application specific functionality should be added using a domain specific buses. 
    class AtomToolsDocumentRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Uuid BusIdType;

        //! Get absolute path of document
        virtual AZStd::string_view GetAbsolutePath() const = 0;

        //! Returns a container describing all reflected objects contained in a document 
        virtual AZStd::vector<DocumentObjectInfo> GetObjectInfo() const = 0;

        //! Load document and related data
        //! @param loadPath absolute path of document to load
        virtual bool Open(AZStd::string_view loadPath) = 0;

        //! Reopen document preserving edits
        virtual bool Reopen() = 0;

        //! Save document to file
        virtual bool Save() = 0;

        //! Save document copy
        //! @param savePath absolute path where document is saved
        virtual bool SaveAsCopy(AZStd::string_view savePath) = 0;

        //! Save document to a new source file derived from of the open document
        //! @param savePath absolute path where document is saved
        virtual bool SaveAsChild(AZStd::string_view savePath) = 0;

        //! Close document and reset its data
        virtual bool Close() = 0;

        //! Document is loaded
        virtual bool IsOpen() const = 0;

        //! Document has changes pending
        virtual bool IsModified() const = 0;

        //! Can the document be saved
        virtual bool IsSavable() const = 0;

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
