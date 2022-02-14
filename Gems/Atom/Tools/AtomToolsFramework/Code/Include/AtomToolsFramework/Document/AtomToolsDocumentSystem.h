/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Document/AtomToolsDocument.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AtomToolsFramework
{
    //! AtomToolsDocumentSystem Is responsible for creation, management, and requests related to documents
    class AtomToolsDocumentSystem
        : public AtomToolsDocumentNotificationBus::Handler
        , public AtomToolsDocumentSystemRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(AtomToolsFramework::AtomToolsDocumentSystem, AZ::SystemAllocator, 0);
        AZ_RTTI(AtomToolsFramework::AtomToolsDocumentSystem, "{9D31F309-6B20-40C5-813C-F1226180E1F8}");
        AZ_DISABLE_COPY_MOVE(AtomToolsDocumentSystem);

        static void Reflect(AZ::ReflectContext* context);

        AtomToolsDocumentSystem() = default;
        AtomToolsDocumentSystem(const AZ::Crc32& toolId);
        ~AtomToolsDocumentSystem();

        // AtomToolsDocumentSystemRequestBus::Handler overrides...
        void RegisterDocumentType(const AtomToolsDocumentFactoryCallback& documentCreator) override;
        AZ::Uuid CreateDocument() override;
        bool DestroyDocument(const AZ::Uuid& documentId) override;
        AZ::Uuid OpenDocument(AZStd::string_view sourcePath) override;
        AZ::Uuid CreateDocumentFromFile(AZStd::string_view sourcePath, AZStd::string_view targetPath) override;
        bool CloseDocument(const AZ::Uuid& documentId) override;
        bool CloseAllDocuments() override;
        bool CloseAllDocumentsExcept(const AZ::Uuid& documentId) override;
        bool SaveDocument(const AZ::Uuid& documentId) override;
        bool SaveDocumentAsCopy(const AZ::Uuid& documentId, AZStd::string_view targetPath) override;
        bool SaveDocumentAsChild(const AZ::Uuid& documentId, AZStd::string_view targetPath) override;
        bool SaveAllDocuments() override;
        AZ::u32 GetDocumentCount() const override;

    private:
        // AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentDependencyModified(const AZ::Uuid& documentId) override;
        void OnDocumentExternallyModified(const AZ::Uuid& documentId) override;

        void QueueReopenDocuments();
        void ReopenDocuments();

        AZ::Uuid OpenDocumentImpl(AZStd::string_view sourcePath, bool checkIfAlreadyOpen);

        const AZ::Crc32 m_toolId = {};
        AtomToolsDocumentFactoryCallback m_documentCreator;
        AZStd::unordered_map<AZ::Uuid, AZStd::shared_ptr<AtomToolsDocument>> m_documentMap;
        AZStd::unordered_set<AZ::Uuid> m_documentIdsWithExternalChanges;
        AZStd::unordered_set<AZ::Uuid> m_documentIdsWithDependencyChanges;
        bool m_queueReopenDocuments = false;
        const size_t m_maxMessageBoxLineCount = 15;
    };
} // namespace AtomToolsFramework
