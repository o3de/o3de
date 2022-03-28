/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasFileObject.h"
#include "UiSerialize.h"
#include <AzCore/Serialization/Utils.h>
#include <LyShine/UiSerializeHelpers.h>
#include "UiCanvasComponent.h"
#include "UiElementComponent.h"


////////////////////////////////////////////////////////////////////////////////////////////////
// Load a serialized stream that may be in an older format that may require massaging the steam
UiCanvasFileObject* UiCanvasFileObject::LoadCanvasFromStream(AZ::IO::GenericStream& stream, const AZ::ObjectStream::FilterDescriptor& filterDesc)
{
    // get the size of the file
    size_t fileSize = stream.GetLength();

    if (fileSize == 0)
    {
        AZ_Error("UI", false, "UI Canvas file: %s is zero bytes on disk, and cannot be loaded.", stream.GetFilename());
        return nullptr;
    }

    // read in the entire file into a char buffer.  Note that files are not 0-truncated!
    char* buffer = new char[fileSize + 1];
    size_t bytesRead = stream.Read(fileSize, buffer);

    // null terminate in case we perform string operations.
    // this is not necessary on ObjectStream, but loading legacy files often requires string ops.
    buffer[bytesRead] = 0;

    // if ReadRaw read the file ok then load the entity from the buffer using AZ
    // serialization
    UiCanvasFileObject* canvas = nullptr;
    if (bytesRead == fileSize)
    {
        // Check to see if this is an old format canvas file that cannot be handled simply in the
        // version convert functions

        enum class FileFormat
        {
            ReallyOld, Old, CanvasObject
        };
        FileFormat fileFormat = FileFormat::CanvasObject;

        // All canvas files start with this (at least up to the introduction of the UiCanvasFileObject)
        const char* objectStreamPrefix =
            "<ObjectStream version=\"1\">";

        // This is what canvas files looked like prior to the introduction of the UiCanvasFileObject
        const char* oldStylePrefix =
            "<Class name=\"AZ::Entity\"";

        // This is what canvas files looked like in Fall 2015 (prior to R1)
        const char* reallyOldStylePrefix =
            "<Entity type=\"{";

        // See if we can identify the buffer as one of the old formats
        if (strncmp(buffer, objectStreamPrefix, strlen(objectStreamPrefix)) == 0)
        {
            // Is started with the usually ObjectStream prefix
            // find the start of the next tag
            const char* secondTag = buffer + strlen(objectStreamPrefix);
            secondTag = strchr(secondTag, '<');
            if (secondTag)
            {
                if (strncmp(secondTag, oldStylePrefix, strlen(oldStylePrefix)) == 0)
                {
                    fileFormat = FileFormat::Old;
                }
                else if (strncmp(secondTag, reallyOldStylePrefix, strlen(reallyOldStylePrefix)) == 0)
                {
                    fileFormat = FileFormat::ReallyOld;
                }
            }
        }

        if (fileFormat == FileFormat::Old)
        {
            // We can load this format but copying all of the entities from the canvas component (and children)
            // to the root slice is not efficient. So write a warning to the log that load times are impacted.
            AZ_Warning("UI", false, "UI canvas file: %s is in an old format, load times will be faster if you resave it.",
                stream.GetFilename());

            // Read this as an old format canvas file
            canvas = LoadCanvasEntitiesFromOldFormatFile(buffer, fileSize, filterDesc);

            if (!canvas)
            {
                AZ_Warning("UI", false, "Old format UI canvas file: %s could not be loaded. It may be corrupted.",
                    stream.GetFilename());
            }
        }
        else
        {
            // This does not look like an old format canvas file so treat it as new format
            AZ::IO::MemoryStream newFormatStream(buffer, fileSize);
            canvas = LoadCanvasFromNewFormatStream(newFormatStream, filterDesc);

            if (!canvas)
            {
                AZ_Warning("UI", false, "UI canvas file: %s could not be loaded. It may be corrupted.",
                    newFormatStream.GetFilename());
            }
        }
    }

    delete [] buffer;
    return canvas;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasFileObject::SaveCanvasToStream(AZ::IO::GenericStream& stream, UiCanvasFileObject* canvasFileObject)
{
    AZ::Utils::SaveObjectToStream<UiCanvasFileObject>(stream, AZ::DataStream::ST_XML, canvasFileObject);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiCanvasFileObject::LoadCanvasEntitiesFromStream(AZ::IO::GenericStream& stream, AZ::Entity*& rootSliceEntity)
{
    AZ::Entity* canvasEntity = nullptr;

    UiCanvasFileObject* fileObject = AZ::Utils::LoadObjectFromStream<UiCanvasFileObject>(stream);
    if (fileObject && fileObject->m_canvasEntity && fileObject->m_rootSliceEntity)
    {
        canvasEntity = fileObject->m_canvasEntity;
        rootSliceEntity = fileObject->m_rootSliceEntity;
    }

    SAFE_DELETE(fileObject);

    return canvasEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiCanvasFileObject::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

    if (serializeContext)
    {
        serializeContext->Class<UiCanvasFileObject>()
            ->Version(2, &UiCanvasFileObject::VersionConverter)
            ->Field("CanvasEntity", &UiCanvasFileObject::m_canvasEntity)
            ->Field("RootSliceEntity", &UiCanvasFileObject::m_rootSliceEntity);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasFileObject* UiCanvasFileObject::LoadCanvasEntitiesFromOldFormatFile(const char* buffer, size_t bufferSize, const AZ::ObjectStream::FilterDescriptor& filterDesc)
{
    // This function attempts to read an old format (pre root slice) canvas file.
    // This is a little complex for a VersionConvert function to do. If we tried to do it in the version
    // converter for the UiCanvasComponent it would be hard because the root slice entity is saved as a
    // sibling of the entity with the UiCanvasComponent on it so it is not accessible within the
    // UiCanvasComponent version converter. Trying to save things into a static list for processing later
    // would be messy and would fail if two canvases were being loaded at the same time on different threads.
    // So we want to do the version conversion in the next level up - which is the CanvasFileObject
    // However, there is no CanvasFileObject level in an old style canvas file. So what we do is modify the buffer
    // so that it looks (just at the top level) like a new style file - with a CanvasFileObject.
    // Then we can handle the conversion in the CanvasFileObject version converter.

    // These are the prefix and suffix for the new style file:
    const char* prefixToAdd1 =
        "<ObjectStream version=\"1\">\n"
        "\t<Class name=\"CanvasFileObject\" version=\"1\" type=\"{1F02632F-F113-49B1-85AD-8CD0FA78B8AA}\">\n"
        "\t\t<Class name=\"AZ::Entity\" field=\"CanvasEntity\" version=\"2\" type=\"{75651658-8663-478D-9090-2432DFCAFA44}\">\n";

    const char* prefixToAdd2 =
        "<ObjectStream version=\"1\">\n"
        "\t<Class name=\"CanvasFileObject\" version=\"1\" type=\"{1F02632F-F113-49B1-85AD-8CD0FA78B8AA}\">\n"
        "\t\t<Class name=\"AZ::Entity\" field=\"CanvasEntity\" type=\"{75651658-8663-478D-9090-2432DFCAFA44}\">\n";

    const char* suffixToAdd =
        "\t\t</Class>\n"
        "\t</Class>\n"
        "</ObjectStream>\n";

    const char* prefixToAdd = prefixToAdd1;

    // These are the prefix and suffix for an old style file. Note that the use of \r\n versus \n only is inconsistent
    // sometimes it comes in with one and sometimes the other.
    const char* prefixToRemove1 =
        "<ObjectStream version=\"1\">\n"
        "\t<Class name=\"AZ::Entity\" version=\"2\" type=\"{75651658-8663-478D-9090-2432DFCAFA44}\">\n";

    const char* prefixToRemove2 =
        "<ObjectStream version=\"1\">\r\n"
        "\t<Class name=\"AZ::Entity\" version=\"2\" type=\"{75651658-8663-478D-9090-2432DFCAFA44}\">\r\n";

    const char* typeString =
        "type=\"{75651658-8663-478D-9090-2432DFCAFA44}\">";

    const char* suffixToRemove1 =
        "\t</Class>\n"
        "</ObjectStream>\n";

    const char* suffixToRemove2 =
        "\t</Class>\r\n"
        "</ObjectStream>\r\n";

    // Do a sanity check that the buffer does start with the prefix that we will remove
    // Also, determine how newlines are represented in the file.
    const char* suffixToRemove = nullptr;

    size_t prefixToRemoveLen = 0;
    if (strncmp(buffer, prefixToRemove1, strlen(prefixToRemove1)) == 0)
    {
        prefixToRemoveLen = strlen(prefixToRemove1);
        suffixToRemove = suffixToRemove1;
    }
    else if (strncmp(buffer, prefixToRemove2, strlen(prefixToRemove2)) == 0)
    {
        prefixToRemoveLen = strlen(prefixToRemove2);
        suffixToRemove = suffixToRemove2;
    }
    else
    {
        // not an exact match - this can happen, for example if the entity version is not 2
        // it can have a missing version
        // This is a more forgiving way to do the test. It could replace the code above but
        // that code has been working for a while so we add this code as a backup
        const char* typeStart = strstr(buffer, typeString);
        if (!typeStart)
        {
            // We can't convert this file.
            if (bufferSize < strlen(prefixToRemove2))
            {
                // Something is very wrong. The file is shorter that the expected prefix.
                // note that we must use AZ_Warning here as this code is shared in tools which don't have gEnv.
                AZ_Warning("UI", false, "Error converting canvas file. File appears to be truncated.");
            }
            else
            {
                // Print out the start of the file for help in debugging
                // user reported issues
                AZStd::string messageBuffer(buffer, strlen(prefixToRemove2));
                AZ_Warning("UI", false, "Error converting canvas file. Prefix is:\r\n%s", messageBuffer.c_str());
            }

            return nullptr;
        }

        prefixToAdd = prefixToAdd2;
        suffixToRemove = suffixToRemove1;

        const char* p = typeStart + strlen(typeString);
        if (*p == '\r')
        {
            ++p;
            suffixToRemove = suffixToRemove2;
        }
        if (*p == '\n')
        {
            ++p;
        }

        // the prefix length is up to the \n after the typeString
        prefixToRemoveLen = p - buffer;
    }

    // work out some lengths here for the strings we want to mess with
    const size_t prefixToAddLen = strlen(prefixToAdd);
    const size_t suffixToAddLen = strlen(suffixToAdd);

    const size_t suffixToRemoveLen = strlen(suffixToRemove);

    // This allows for not knowing exactly how many extra chars will be at the end of the file.
    // We search backward for some arbitrary character in the suffixToRemove ('<') and line things
    // up using that.
    const char* lastOpenAngleInBuffer = strrchr(buffer, '<');
    const char* lastOpenAngleInSuffixToRemove = strrchr(suffixToRemove, '<');
    const char* suffixToRemoveStart = lastOpenAngleInBuffer - (lastOpenAngleInSuffixToRemove - suffixToRemove);

    // sanity check to check that the suffix matches
    if (strncmp(suffixToRemoveStart, suffixToRemove, suffixToRemoveLen) != 0)
    {
        AZ_Warning("UI", false, "Error converting canvas file. File appears to be truncated at the end.");
        return nullptr;
    }

    // Compute the start and length of the part we want to copy from the old buffer to the new buffer
    const char* oldBufferCoreStart = buffer + prefixToRemoveLen;
    const size_t oldBufferCoreLen = suffixToRemoveStart - oldBufferCoreStart;

    // allocate the new buffer
    size_t newBufferSize = prefixToAddLen + oldBufferCoreLen + suffixToAddLen + 1;
    char* newBuffer = new char[newBufferSize];

    // fill the new buffer with the new prefix, the old core and the new suffix
    char* insertPoint = newBuffer;

    azstrncpy(insertPoint, newBufferSize, prefixToAdd, prefixToAddLen);
    insertPoint += prefixToAddLen;

    azstrncpy(insertPoint, newBufferSize - prefixToAddLen, oldBufferCoreStart, oldBufferCoreLen);
    insertPoint += oldBufferCoreLen;

    azstrncpy(insertPoint, newBufferSize - prefixToAddLen - oldBufferCoreLen, suffixToAdd, suffixToAddLen);
    insertPoint += suffixToAddLen;

    insertPoint[0] = '\0';

    // Now try loading from this new buffer, the rest of the conversion is done in CanvasFileObject::VersionConverter
    AZ::IO::MemoryStream stream(newBuffer, newBufferSize);
    UiCanvasFileObject* canvas = LoadCanvasFromNewFormatStream(stream, filterDesc);

    delete [] newBuffer;

    return canvas;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiCanvasFileObject* UiCanvasFileObject::LoadCanvasFromNewFormatStream(AZ::IO::GenericStream& stream, const AZ::ObjectStream::FilterDescriptor& filterDesc)
{
    UiCanvasFileObject* fileObject =
        AZ::Utils::LoadObjectFromStream<UiCanvasFileObject>(stream, nullptr, filterDesc);
    return fileObject;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper function to find the root element node in a canvas entity node
AZ::SerializeContext::DataElementNode* UiCanvasFileObject::FindRootElementInCanvasEntity(
    [[maybe_unused]] AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& canvasEntityNode)
{
    // Find the UiCanvasComponent in the CanvasEntity
    AZ::SerializeContext::DataElementNode* canvasComponentNode =
        LyShine::FindComponentNode(canvasEntityNode, UiCanvasComponent::TYPEINFO_Uuid());
    if (!canvasComponentNode)
    {
        return nullptr;
    }

    // Find the RootElement entity in the UiCanvasComponent
    int rootElementIndex = canvasComponentNode->FindElement(AZ_CRC("RootElement", 0x9ac9557b));
    if (rootElementIndex == -1)
    {
        return nullptr;
    }
    AZ::SerializeContext::DataElementNode& rootElementNode = canvasComponentNode->GetSubElement(rootElementIndex);

    return &rootElementNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper function to create the root slice entity node and all its sub nodes and then copy
// the entities representing all the UI elements in the canvas into the SliceComponent node
bool UiCanvasFileObject::CreateRootSliceNodeAndCopyInEntities(
    AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& canvasFileObjectNode,
    AZStd::vector<AZ::SerializeContext::DataElementNode>& copiedEntities)
{
    // Create an entity node for the root slice
    int entityIndex = canvasFileObjectNode.AddElement<AZ::Entity>(context, "RootSliceEntity");
    if (entityIndex == -1)
    {
        return false;
    }
    AZ::SerializeContext::DataElementNode& entityNode = canvasFileObjectNode.GetSubElement(entityIndex);

    // create the entity Id node
    if (!LyShine::CreateEntityIdNode(context, entityNode))
    {
        return false;
    }

    // Do not create a name node.
    // EntityContext::CreateRootSlice creates an Entity with no name for the root slice entity
    // This means that it defaults to the EntityId. If we don't create a name node here it seems to get a random
    // value. That doesn't seem to matter though since the name of this entity is not used for anything.

    // create the IsDependencyReady node
    bool isDependencyReady = true;
    int isDependencyReadyIndex = entityNode.AddElementWithData(context, "IsDependencyReady", isDependencyReady);
    if (isDependencyReadyIndex == -1)
    {
        return false;
    }

    // create the components vector node (which is a generic vector)
    using componentsVector = AZStd::vector<AZ::Component*>;
    AZ::SerializeContext::ClassData* componentVectorClassData = AZ::SerializeGenericTypeInfo<componentsVector>::GetGenericInfo()->GetClassData();
    int componentsIndex = entityNode.AddElement(context, "Components", *componentVectorClassData);
    if (componentsIndex == -1)
    {
        return false;
    }
    AZ::SerializeContext::DataElementNode& componentsNode = entityNode.GetSubElement(componentsIndex);

    // create the slice component node
    int sliceComponentIndex = componentsNode.AddElement(context, "element", AZ::SliceComponent::TYPEINFO_Uuid());
    if (sliceComponentIndex == -1)
    {
        return false;
    }
    AZ::SerializeContext::DataElementNode& sliceComponentNode = componentsNode.GetSubElement(sliceComponentIndex);

    // create the component base class
    if (!LyShine::CreateComponentBaseClassNode(context, sliceComponentNode))
    {
        return false;
    }

    // create the Entities vector
    using entityVector = AZStd::vector<AZ::Entity*>;
    AZ::SerializeContext::ClassData* entityVectorClassData = AZ::SerializeGenericTypeInfo<entityVector>::GetGenericInfo()->GetClassData();
    int entitiesIndex = sliceComponentNode.AddElement(context, "Entities", *entityVectorClassData);
    if (entitiesIndex == -1)
    {
        return false;
    }
    AZ::SerializeContext::DataElementNode& entitiesNode = sliceComponentNode.GetSubElement(entitiesIndex);

    // Add the entities to the entities vector
    for (AZ::SerializeContext::DataElementNode& entityElement : copiedEntities)
    {
        entityElement.SetName("element");   // all elements in the Vector should have this name
        entitiesNode.AddElement(entityElement);
    }

    // No need to create the empty Slices node

    // create the IsDynamic node
    bool isDynamic = true;
    int isDynamicIndex = sliceComponentNode.AddElementWithData(context, "IsDynamic", isDynamic);
    if (isDynamicIndex == -1)
    {
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiCanvasFileObject::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& canvasFileObjectNode)
{
    if (canvasFileObjectNode.GetVersion() == 1)
    {
        // this is a pre-slice dummy CanvasFileObject programatically created on load
        // we need to change all Entity* references (m_rootElement in UiCanvasComponent
        // and m_children in UiElementComponent) into EntityId's instead and move
        // the entities data into the slice component.

        // find the CanvasEntity in the CanvasFileObject
        int canvasEntityIndex = canvasFileObjectNode.FindElement(AZ_CRC("CanvasEntity", 0x87ff30ab));
        if (canvasEntityIndex == -1)
        {
            return false;
        }
        AZ::SerializeContext::DataElementNode& canvasEntityNode = canvasFileObjectNode.GetSubElement(canvasEntityIndex);

        // Find the m_rootElement member in the UiCanvasComponent on the canvas entity
        AZ::SerializeContext::DataElementNode* rootElementNode = FindRootElementInCanvasEntity(context, canvasEntityNode);
        if (!rootElementNode)
        {
            return false;
        }

        // All UI element entities will be copied to this container and then added to the slice component
        AZStd::vector<AZ::SerializeContext::DataElementNode> copiedEntities;

        // recursively process the root element and all of its child elements, copying their child entities to the
        // entities container and replacing them with EntityIds
        if (!UiElementComponent::MoveEntityAndDescendantsToListAndReplaceWithEntityId(context, *rootElementNode, -1, copiedEntities))
        {
            return false;
        }

        // Create the RootSliceEntity in the CanvasFileObject and copy the entities into it
        if (!CreateRootSliceNodeAndCopyInEntities(context, canvasFileObjectNode, copiedEntities))
        {
            return false;
        }
    }

    return true;
}

