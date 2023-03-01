/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/DocumentEditor/ToolsAnyDocument.h>
#include <AzToolsFramework/DocumentEditor/ToolsDocumentNotificationBus.h>
#include <AzToolsFramework/API/Utils.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Serialization/Json/JsonUtils.h>

namespace AzToolsFramework
{
    void ToolsAnyDocument::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ToolsAnyDocument, ToolsDocument>()->Version(0);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ToolsAnyDocumentRequestBus>("ToolsAnyDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "tools")
                ->Event("GetContent", &ToolsAnyDocumentRequests::GetContent);
        }
    }

    ToolsAnyDocument::ToolsAnyDocument(
        const AZ::Crc32& toolId,
        const DocumentTypeInfo& documentTypeInfo,
        const AZStd::any& defaultValue,
        const AZ::Uuid& contentTypeIdIfNotEmbedded)
        : ToolsDocument(toolId, documentTypeInfo)
        , m_content(defaultValue)
        , m_contentTypeIdIfNotEmbedded(contentTypeIdIfNotEmbedded)
    {
        ToolsAnyDocumentRequestBus::Handler::BusConnect(m_id);
    }

    ToolsAnyDocument::~ToolsAnyDocument()
    {
        ToolsAnyDocumentRequestBus::Handler::BusDisconnect();
    }

    DocumentTypeInfo ToolsAnyDocument::BuildDocumentTypeInfo(
        const AZStd::string& documentTypeName,
        const AZStd::vector<AZStd::string>& documentTypeExtensions,
        const AZStd::vector<AZStd::string>& documentTypeTemplateExtensions,
        const AZStd::any& defaultValue,
        const AZ::Uuid& contentTypeIdIfNotEmbedded)
    {
        DocumentTypeInfo documentType;
        documentType.m_documentTypeName = documentTypeName;
        documentType.m_documentFactoryCallback =
            [defaultValue, contentTypeIdIfNotEmbedded](const AZ::Crc32& toolId, const DocumentTypeInfo& documentTypeInfo)
        {
            return aznew ToolsAnyDocument(toolId, documentTypeInfo, defaultValue, contentTypeIdIfNotEmbedded);
        };

        for (const auto& extension : documentTypeExtensions)
        {
            documentType.m_supportedExtensionsToOpen.push_back({ documentTypeName, extension });
            documentType.m_supportedExtensionsToSave.push_back({ documentTypeName, extension });
        }

        for (const auto& extension : documentTypeTemplateExtensions)
        {
            documentType.m_supportedExtensionsToCreate.push_back({ documentTypeName + " Template", extension });
        }
        return documentType;
    }

    DocumentObjectInfoVector ToolsAnyDocument::GetObjectInfo() const
    {
        DocumentObjectInfoVector objects = ToolsDocument::GetObjectInfo();

        if (!m_content.empty())
        {
            // The reflected data stored within the document will be converted to a description of the object and its type info. This data
            // will be used to populate the inspector.
            DocumentObjectInfo objectInfo;
            objectInfo.m_visible = true;
            objectInfo.m_name = GetDocumentTypeInfo().m_documentTypeName;
            objectInfo.m_displayName = GetDocumentTypeInfo().m_documentTypeName;
            objectInfo.m_description = GetDocumentTypeInfo().m_documentTypeName;
            objectInfo.m_objectType = m_content.type();
            objectInfo.m_objectPtr = AZStd::any_cast<void>(const_cast<AZStd::any*>(&m_content));
            objects.push_back(AZStd::move(objectInfo));
        }

        return objects;
    }

    bool ToolsAnyDocument::Open(const AZStd::string& loadPath)
    {
        if (!ToolsDocument::Open(loadPath))
        {
            return false;
        }

        if (!LoadAny())
        {
            return OpenFailed();
        }

        m_modified = false;
        return OpenSucceeded();
    }

    bool ToolsAnyDocument::Save()
    {
        if (!ToolsDocument::Save())
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        if (!SaveAny())
        {
            return SaveFailed();
        }

        m_modified = false;
        m_absolutePath = m_savePathNormalized;
        return SaveSucceeded();
    }

    bool ToolsAnyDocument::SaveAsCopy(const AZStd::string& savePath)
    {
        if (!ToolsDocument::SaveAsCopy(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }


        if (!SaveAny())
        {
            return SaveFailed();
        }

        m_modified = false;
        m_absolutePath = m_savePathNormalized;
        return SaveSucceeded();
    }

    bool ToolsAnyDocument::SaveAsChild(const AZStd::string& savePath)
    {
        if (!ToolsDocument::SaveAsChild(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        if (!SaveAny())
        {
            return SaveFailed();
        }

        m_modified = false;
        m_absolutePath = m_savePathNormalized;
        return SaveSucceeded();
    }

    bool ToolsAnyDocument::IsModified() const
    {
        return m_modified;
    }

    bool ToolsAnyDocument::BeginEdit()
    {
        RecordContentState();
        return true;
    }

    bool ToolsAnyDocument::EndEdit()
    {
        auto undoState = m_contentStateForUndoRedo;
        RecordContentState();
        auto redoState = m_contentStateForUndoRedo;

        if (undoState != redoState)
        {
            AddUndoRedoHistory(
                [this, undoState]() { RestoreContentState(undoState); },
                [this, redoState]() { RestoreContentState(redoState); });

            m_modified = true;
            ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
        }
        return true;
    }

    void ToolsAnyDocument::Clear()
    {
        m_contentStateForUndoRedo.clear();
        m_content.clear();
        m_modified = false;

        ToolsDocument::Clear();
    }

    const AZStd::any& ToolsAnyDocument::GetContent() const
    {
        return m_content;
    }

    void ToolsAnyDocument::RecordContentState()
    {
        // Serialize the current content to a byte stream so that it can be restored with undo redo operations.
        m_contentStateForUndoRedo.clear();
        AZ::IO::ByteContainerStream<decltype(m_contentStateForUndoRedo)> undoContentStateStream(&m_contentStateForUndoRedo);
        AZ::Utils::SaveObjectToStream(undoContentStateStream, AZ::ObjectStream::ST_BINARY, &m_content);
    }

    void ToolsAnyDocument::RestoreContentState(const AZStd::vector<AZ::u8>& contentState)
    {
        // Restore a version of the content that was previously serialized to a byte stream
        m_contentStateForUndoRedo = contentState;
        AZ::IO::ByteContainerStream<decltype(m_contentStateForUndoRedo)> undoContentStateStream(&m_contentStateForUndoRedo);

        m_content.clear();
        AZ::Utils::LoadObjectFromStreamInPlace(undoContentStateStream, m_content);

        m_modified = true;
        ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentObjectInfoInvalidated, m_id);
        ToolsDocumentNotificationBus::Event(m_toolId, &ToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
    }

    bool ToolsAnyDocument::LoadAny()
    {
        m_content.clear();

        // When this type ID is provided an attempt is made to load the data from the JSON file, assuming that the file only contains
        // reflected object data.
        if (!m_contentTypeIdIfNotEmbedded.IsNull())
        {
            // Serialized context is required to create a placeholder object using the type ID.
            AZ::SerializeContext* serializeContext = {};
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(serializeContext, "Failed to acquire application serialize context.");

            m_content = serializeContext->CreateAny(m_contentTypeIdIfNotEmbedded);
            if (m_content.empty())
            {
                AZ_Error(
                    "ToolsAnyDocument",
                    false,
                    "Failed to create AZStd::any from type: %s",
                    m_contentTypeIdIfNotEmbedded.ToFixedString().c_str());
                return false;
            }

            // Attempt to read the JSON file data from the document breath.
            auto loadOutcome = AZ::JsonSerializationUtils::ReadJsonFile(m_absolutePath);
            if (!loadOutcome.IsSuccess())
            {
                AZ_Error("ToolsAnyDocument", false, "Failed to read JSON file: %s", loadOutcome.GetError().c_str());
                return false;
            }

            // Read the rapid JSON document data into the object we just created.
            AZ::JsonDeserializerSettings jsonSettings;
            AZ::JsonSerializationUtils::JsonReportingHelper reportingHelper;
            reportingHelper.Attach(jsonSettings);

            rapidjson::Document& document = loadOutcome.GetValue();
            AZ::JsonSerialization::Load(AZStd::any_cast<void>(&m_content), m_contentTypeIdIfNotEmbedded, document, jsonSettings);

            if (reportingHelper.ErrorsReported())
            {
                AZ_Error("ToolsAnyDocument", false, "Failed to load object from JSON file: %s", m_absolutePath.c_str());
                return false;
            }
        }
        else
        {
            // If no time ID was provided the serializer will attempt to parse it from the JSON file.
            auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(m_absolutePath);
            if (!loadResult)
            {
                AZ_Error("ToolsAnyDocument", false, "Failed to load object from JSON file: %s", m_absolutePath.c_str());
                return false;
            }
            m_content = loadResult.GetValue();
        }

        return true;

    }

    bool ToolsAnyDocument::SaveAny() const
    {
        if (m_content.empty())
        {
            return false;
        }

        if (!m_contentTypeIdIfNotEmbedded.IsNull())
        {
            // Create a default content object of the specified type that will be used to keep the serializer from writing out unmodified
            // values.
            AZ::SerializeContext* serializeContext = {};
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(serializeContext, "Failed to acquire application serialize context.");

            AZStd::any defaultContent = serializeContext->CreateAny(m_contentTypeIdIfNotEmbedded);
            if (defaultContent.empty())
            {
                AZ_Error(
                    "ToolsAnyDocument",
                    false,
                    "Failed to create AZStd::any from type: %s",
                    m_contentTypeIdIfNotEmbedded.ToFixedString().c_str());
                return false;
            }

            // Create a rapid JSON document and object to serialize the document data into.
            AZ::JsonSerializerSettings settings;
            AZ::JsonSerializationUtils::JsonReportingHelper reportingHelper;
            reportingHelper.Attach(settings);

            rapidjson::Document document;
            document.SetObject();
            AZ::JsonSerialization::Store(
                document,
                document.GetAllocator(),
                AZStd::any_cast<void>(&m_content),
                AZStd::any_cast<void>(&defaultContent),
                m_contentTypeIdIfNotEmbedded,
                settings);

            if (reportingHelper.ErrorsReported())
            {
                AZ_Error("ToolsAnyDocument", false, "Failed to write object data to JSON document: %s", m_savePathNormalized.c_str());
                return false;
            }

            AZ::JsonSerializationUtils::WriteJsonFile(document, m_savePathNormalized);
            if (reportingHelper.ErrorsReported())
            {
                AZ_Error("ToolsAnyDocument", false, "Failed to write JSON document to file: %s", m_savePathNormalized.c_str());
                return false;
            }
        }
        else
        {
            if (!AZ::JsonSerializationUtils::SaveObjectToFileByType(
                    AZStd::any_cast<void>(&m_content), m_content.type(), m_savePathNormalized))
            {
                AZ_Error("ToolsAnyDocument", false, "Failed to write JSON document to file: %s", m_savePathNormalized.c_str());
                return false;
            }
        }
        return true;
    }
} // namespace AzToolsFramework
