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
    //! AtomToolsDocumentSystem manages requests for registering multiple document types and creating, loading, saving multiple documents
    //! from them. For each operation, it collects all of the warnings and errors and displays them to alert the user.
    class AtomToolsDocumentSystem
        : public AtomToolsDocumentNotificationBus::Handler
        , public AtomToolsDocumentSystemRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(AtomToolsFramework::AtomToolsDocumentSystem, AZ::SystemAllocator);
        AZ_RTTI(AtomToolsFramework::AtomToolsDocumentSystem, "{9D31F309-6B20-40C5-813C-F1226180E1F8}");
        AZ_DISABLE_COPY_MOVE(AtomToolsDocumentSystem);

        static void Reflect(AZ::ReflectContext* context);

        AtomToolsDocumentSystem() = default;
        AtomToolsDocumentSystem(const AZ::Crc32& toolId);
        ~AtomToolsDocumentSystem();

        // AtomToolsDocumentSystemRequestBus::Handler overrides...
        void RegisterDocumentType(const DocumentTypeInfo& documentType) override;
        const DocumentTypeInfoVector& GetRegisteredDocumentTypes() const override;
        AZ::Uuid CreateDocumentFromType(const DocumentTypeInfo& documentType) override;
        AZ::Uuid CreateDocumentFromTypeName(const AZStd::string& documentTypeName) override;
        AZ::Uuid CreateDocumentFromFileType(const AZStd::string& path) override;
        AZ::Uuid CreateDocumentFromFilePath(const AZStd::string& sourcePath, const AZStd::string& targetPath) override;
        bool DestroyDocument(const AZ::Uuid& documentId) override;
        AZ::Uuid OpenDocument(const AZStd::string& sourcePath) override;
        bool CloseDocument(const AZ::Uuid& documentId) override;
        bool CloseAllDocuments() override;
        bool CloseAllDocumentsExcept(const AZ::Uuid& documentId) override;
        bool SaveDocument(const AZ::Uuid& documentId) override;
        bool SaveDocumentAsCopy(const AZ::Uuid& documentId, const AZStd::string& targetPath) override;
        bool SaveDocumentAsChild(const AZ::Uuid& documentId, const AZStd::string& targetPath) override;
        bool SaveAllDocuments() override;
        bool SaveAllModifiedDocuments() override;
        bool QueueReopenModifiedDocuments() override;
        bool ReopenModifiedDocuments() override;
        AZ::u32 GetDocumentCount() const override;
        bool IsDocumentOpen(const AZ::Uuid& documentId) const override;
        void AddRecentFilePath(const AZStd::string& absolutePath) override;
        void ClearRecentFilePaths() override;
        void SetRecentFilePaths(const AZStd::vector<AZStd::string>& absolutePaths) override;
        const AZStd::vector<AZStd::string> GetRecentFilePaths() const override;

    private:
        // AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentModified(const AZ::Uuid& documentId) override;
        void OnDocumentDependencyModified(const AZ::Uuid& documentId) override;
        void OnDocumentExternallyModified(const AZ::Uuid& documentId) override;

        const AZ::Crc32 m_toolId = {};
        DocumentTypeInfoVector m_documentTypes;
        AZStd::unordered_map<AZ::Uuid, AZStd::shared_ptr<AtomToolsDocumentRequests>> m_documentMap;
        AZStd::unordered_set<AZ::Uuid> m_documentIdsWithExternalChanges;
        AZStd::unordered_set<AZ::Uuid> m_documentIdsWithDependencyChanges;
        bool m_queueReopenModifiedDocuments = false;
        bool m_queueSaveAllModifiedDocuments = false;
        const size_t m_maxMessageBoxLineCount = 15;
    };
} // namespace AtomToolsFramework
