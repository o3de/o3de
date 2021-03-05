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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/any.h>

namespace ShaderManagementConsole
{
    class ShaderManagementConsoleDocumentNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Signal that a document was created
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentCreated([[maybe_unused]] const AZ::Uuid& documentId) {}

        //! Signal that a document was destroyed
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentDestroyed([[maybe_unused]] const AZ::Uuid& documentId) {}

        //! Signal that a document was opened
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentOpened([[maybe_unused]] const AZ::Uuid& documentId) {}

        //! Signal that a document was closed
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentClosed([[maybe_unused]] const AZ::Uuid& documentId) {}

        //! Signal that a document was saved
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentSaved([[maybe_unused]] const AZ::Uuid& documentId) {}

        //! Signal that a document was selected
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentSelected([[maybe_unused]] const AZ::Uuid& documentId) {}

        //! Signal that a document was modified
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentModified([[maybe_unused]] const AZ::Uuid& documentId) {}

        //! Signal that a document undo state was updated
        //! @param documentId unique id of document for which the notification is sent
        virtual void OnDocumentUndoStateChanged([[maybe_unused]] const AZ::Uuid& documentId) {}
    };

    using ShaderManagementConsoleDocumentNotificationBus = AZ::EBus<ShaderManagementConsoleDocumentNotifications>;
} // namespace ShaderManagementConsole
