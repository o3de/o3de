/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AtomToolsFramework/Document/AtomToolsAnyDocument.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Utils/Utils.h>

namespace AtomToolsFramework
{
    void AtomToolsAnyDocument::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtomToolsAnyDocument, AtomToolsDocument>()->Version(0);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AtomToolsAnyDocumentRequestBus>("AtomToolsAnyDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "atomtools")
                ->Event("GetContent", &AtomToolsAnyDocumentRequests::GetContent);
        }
    }

    AtomToolsAnyDocument::AtomToolsAnyDocument(
        const AZ::Crc32& toolId,
        const DocumentTypeInfo& documentTypeInfo,
        const AZStd::any& defaultValue,
        const AZ::Uuid& contentTypeIdIfNotEmbedded)
        : AtomToolsDocument(toolId, documentTypeInfo)
        , m_content(defaultValue)
        , m_contentTypeIdIfNotEmbedded(contentTypeIdIfNotEmbedded)
    {
        AtomToolsAnyDocumentRequestBus::Handler::BusConnect(m_id);
    }

    AtomToolsAnyDocument::~AtomToolsAnyDocument()
    {
        AtomToolsAnyDocumentRequestBus::Handler::BusDisconnect();
    }

    DocumentTypeInfo AtomToolsAnyDocument::BuildDocumentTypeInfo(
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
            return aznew AtomToolsAnyDocument(toolId, documentTypeInfo, defaultValue, contentTypeIdIfNotEmbedded);
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

    DocumentObjectInfoVector AtomToolsAnyDocument::GetObjectInfo() const
    {
        DocumentObjectInfoVector objects = AtomToolsDocument::GetObjectInfo();

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

    bool AtomToolsAnyDocument::Open(const AZStd::string& loadPath)
    {
        if (!AtomToolsDocument::Open(loadPath))
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

    bool AtomToolsAnyDocument::Save()
    {
        if (!AtomToolsDocument::Save())
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

    bool AtomToolsAnyDocument::SaveAsCopy(const AZStd::string& savePath)
    {
        if (!AtomToolsDocument::SaveAsCopy(savePath))
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

    bool AtomToolsAnyDocument::SaveAsChild(const AZStd::string& savePath)
    {
        if (!AtomToolsDocument::SaveAsChild(savePath))
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

    bool AtomToolsAnyDocument::IsModified() const
    {
        return m_modified;
    }

    bool AtomToolsAnyDocument::BeginEdit()
    {
        RecordContentState();
        return true;
    }

    bool AtomToolsAnyDocument::EndEdit()
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
            AtomToolsDocumentNotificationBus::Event(m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
        }
        return true;
    }

    void AtomToolsAnyDocument::Clear()
    {
        m_contentStateForUndoRedo.clear();
        m_content.clear();
        m_modified = false;

        AtomToolsDocument::Clear();
    }

    const AZStd::any& AtomToolsAnyDocument::GetContent() const
    {
        return m_content;
    }

    void AtomToolsAnyDocument::RecordContentState()
    {
        // Serialize the current content to a byte stream so that it can be restored with undo redo operations.
        m_contentStateForUndoRedo.clear();
        AZ::IO::ByteContainerStream<decltype(m_contentStateForUndoRedo)> undoContentStateStream(&m_contentStateForUndoRedo);
        AZ::Utils::SaveObjectToStream(undoContentStateStream, AZ::ObjectStream::ST_BINARY, &m_content);
    }

    void AtomToolsAnyDocument::RestoreContentState(const AZStd::vector<AZ::u8>& contentState)
    {
        // Restore a version of the content that was previously serialized to a byte stream
        m_contentStateForUndoRedo = contentState;
        AZ::IO::ByteContainerStream<decltype(m_contentStateForUndoRedo)> undoContentStateStream(&m_contentStateForUndoRedo);

        m_content.clear();
        AZ::Utils::LoadObjectFromStreamInPlace(undoContentStateStream, m_content);

        m_modified = true;
        AtomToolsDocumentNotificationBus::Event(m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentObjectInfoInvalidated, m_id);
        AtomToolsDocumentNotificationBus::Event(m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
    }

    bool AtomToolsAnyDocument::LoadAny()
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
                    "AtomToolsAnyDocument",
                    false,
                    "Failed to create AZStd::any from type: %s",
                    m_contentTypeIdIfNotEmbedded.ToFixedString().c_str());
                return false;
            }

            // Attempt to read the JSON file data from the document breath.
            auto loadOutcome = AZ::JsonSerializationUtils::ReadJsonFile(m_absolutePath);
            if (!loadOutcome.IsSuccess())
            {
                AZ_Error("AtomToolsAnyDocument", false, "Failed to read JSON file: %s", loadOutcome.GetError().c_str());
                return false;
            }

            // Read the rapid JSON document data into the object we just created.
            AZ::JsonDeserializerSettings jsonSettings;
            AZ::RPI::JsonReportingHelper reportingHelper;
            reportingHelper.Attach(jsonSettings);

            rapidjson::Document& document = loadOutcome.GetValue();
            AZ::JsonSerialization::Load(AZStd::any_cast<void>(&m_content), m_contentTypeIdIfNotEmbedded, document, jsonSettings);

            if (reportingHelper.ErrorsReported())
            {
                AZ_Error("AtomToolsAnyDocument", false, "Failed to load object from JSON file: %s", m_absolutePath.c_str());
                return false;
            }
        }
        else
        {
            // If no time ID was provided the serializer will attempt to parse it from the JSON file.
            auto loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(m_absolutePath);
            if (!loadResult)
            {
                AZ_Error("AtomToolsAnyDocument", false, "Failed to load object from JSON file: %s", m_absolutePath.c_str());
                return false;
            }
            m_content = loadResult.GetValue();
        }

        return true;
    }

    bool AtomToolsAnyDocument::SaveAny() const
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
                    "AtomToolsAnyDocument",
                    false,
                    "Failed to create AZStd::any from type: %s",
                    m_contentTypeIdIfNotEmbedded.ToFixedString().c_str());
                return false;
            }

            // Create a rapid JSON document and object to serialize the document data into.
            AZ::JsonSerializerSettings settings;
            AZ::RPI::JsonReportingHelper reportingHelper;
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
                AZ_Error("AtomToolsAnyDocument", false, "Failed to write object data to JSON document: %s", m_savePathNormalized.c_str());
                return false;
            }

            AZ::JsonSerializationUtils::WriteJsonFile(document, m_savePathNormalized);
            if (reportingHelper.ErrorsReported())
            {
                AZ_Error("AtomToolsAnyDocument", false, "Failed to write JSON document to file: %s", m_savePathNormalized.c_str());
                return false;
            }
        }
        else
        {
            if (!AZ::JsonSerializationUtils::SaveObjectToFileByType(
                    AZStd::any_cast<void>(&m_content), m_content.type(), m_savePathNormalized))
            {
                AZ_Error("AtomToolsAnyDocument", false, "Failed to write JSON document to file: %s", m_savePathNormalized.c_str());
                return false;
            }
        }
        return true;
    }
} // namespace AtomToolsFramework
