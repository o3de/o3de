/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/DocumentEditor/ToolsAnyDocumentRequestBus.h>
#include <AzToolsFramework/DocumentEditor/ToolsDocument.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>

namespace AzToolsFramework
{
    //! ToolsAnyDocument is a bare bones document class for adapting arbitrary data types that use standard serialize and edit context
    //! reflection into AzToolsFramework document system and applications. Document data is wrapped by AZStd::any to load, save, and edit
    //! content. Pre-existing JSON serialized data must include the outer type info object in order to be deserialized by this class. Many
    //! Atom classes like shader source data, material types, and materials don't serialize the type info so the class needs to support two
    //! paths for saving and loading based on a supply type id.
    class ToolsAnyDocument
        : public ToolsDocument
        , public ToolsAnyDocumentRequestBus::Handler
    {
    public:
        AZ_RTTI(ToolsAnyDocument, "{415118F6-9403-4169-BA39-719D96EF7C68}", ToolsDocument);
        AZ_CLASS_ALLOCATOR(ToolsAnyDocument, AZ::SystemAllocator, 0);
        AZ_DISABLE_COPY_MOVE(ToolsAnyDocument);

        static void Reflect(AZ::ReflectContext* context);

        ToolsAnyDocument() = default;
        ToolsAnyDocument(
            const AZ::Crc32& toolId,
            const DocumentTypeInfo& documentTypeInfo,
            const AZStd::any& defaultValue,
            const AZ::Uuid& contentTypeIdIfNotEmbedded);
        virtual ~ToolsAnyDocument();

        // Configure the document type name and supported extensions for this document class.
        // @documentTypeName The display name for the document and registered extensions
        // @documentTypeExtensions The set of extensions supported by this document type
        // @contentTypeIdIfNotEmbedded The explicit type ID that should be used for documents that do not have the type info
        // embedded in the JSON file. If this value is not null, then the document class will use an alternate path for loading and saving
        // the JSON data that assumes no type info exists in the file.
        static DocumentTypeInfo BuildDocumentTypeInfo(
            const AZStd::string& documentTypeName,
            const AZStd::vector<AZStd::string>& documentTypeExtensions,
            const AZStd::vector<AZStd::string>& documentTypeTemplateExtensions,
            const AZStd::any& defaultValue,
            const AZ::Uuid& contentTypeIdIfNotEmbedded);

        // ToolsDocument overrides...
        DocumentObjectInfoVector GetObjectInfo() const override;
        bool Open(const AZStd::string& loadPath) override;
        bool Save() override;
        bool SaveAsCopy(const AZStd::string& savePath) override;
        bool SaveAsChild(const AZStd::string& savePath) override;
        bool IsModified() const override;
        bool BeginEdit() override;
        bool EndEdit() override;
        void Clear() override;

        // ToolsAnyDocumentRequestBus::Handler overrides...
        const AZStd::any& GetContent() const override;

    private:
        void RecordContentState();
        void RestoreContentState(const AZStd::vector<AZ::u8>& contentState);
        bool LoadAny();
        bool SaveAny() const;

        AZStd::any m_content;
        AZ::Uuid m_contentTypeIdIfNotEmbedded = AZ::Uuid::CreateNull();
        AZStd::vector<AZ::u8> m_contentStateForUndoRedo;
        bool m_modified = {};
    };
} // namespace AzToolsFramework
