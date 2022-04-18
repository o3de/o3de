/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Document/AtomToolsDocumentObjectInfo.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/any.h>

namespace AtomToolsFramework
{
    class AtomToolsDocumentNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Crc32 BusIdType;

        //! Signal that a document was created
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentCreated([[maybe_unused]] const AZ::Uuid& documentId){};

        //! Signal that a document was destroyed
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentDestroyed([[maybe_unused]] const AZ::Uuid& documentId){};

        //! Signal that a document was opened
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentOpened([[maybe_unused]] const AZ::Uuid& documentId){};

        //! Signal that a document was closed
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentClosed([[maybe_unused]] const AZ::Uuid& documentId){};

        //! Signal that a document was saved
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentSaved([[maybe_unused]] const AZ::Uuid& documentId){};

        //! Signal that a document was modified
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentModified([[maybe_unused]] const AZ::Uuid& documentId){};

        //! Signal that a document dependency was modified
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentDependencyModified([[maybe_unused]] const AZ::Uuid& documentId){};

        //! Signal that a document was modified externally
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentExternallyModified([[maybe_unused]] const AZ::Uuid& documentId){};

        //! Signal that a document undo state was updated
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentUndoStateChanged([[maybe_unused]] const AZ::Uuid& documentId){};

        //! Signal that the group has been changed.
        //! @param documentId unique id of document for which the notification is sent
        //! @param objectInfo description of the reflected object that's been modified
        //! @param rebuilt signifies if it was a structural change that might require ui to be rebuilt
        virtual void OnDocumentObjectInfoChanged(
            [[maybe_unused]] const AZ::Uuid& documentId,
            [[maybe_unused]] const DocumentObjectInfo& objectInfo,
            [[maybe_unused]] bool rebuilt){};

        //! Signal the document content has been cleared
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentCleared([[maybe_unused]] const AZ::Uuid& documentId){};

        //! Signal the document has experienced an error
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentError([[maybe_unused]] const AZ::Uuid& documentId){};
    };

    using AtomToolsDocumentNotificationBus = AZ::EBus<AtomToolsDocumentNotifications>;
} // namespace AtomToolsFramework
