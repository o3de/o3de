/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/ObjectStream.h>

#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Memory/OSAllocator.h>

#include <AzCore/Debug/Profiler.h>

namespace AZ::Utils
{
    bool LoadObjectFromStreamInPlace(IO::GenericStream& stream, AZ::SerializeContext* context, const SerializeContext::ClassData* objectClassData, void* targetPointer, const FilterDescriptor& filterDesc)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        AZ_Assert(objectClassData, "Class data is required.");

        if (!context)
        {
            ComponentApplicationBus::BroadcastResult(context, &ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(context, "No serialize context");
        }

        if (!context)
        {
            return false;
        }

        AZ_Assert(targetPointer, "You must provide a target pointer");

        bool foundSuccess = false;
        using CreationCallback = AZStd::function<void (void **, const SerializeContext::ClassData **, const Uuid &, SerializeContext *)>;
        auto handler = [&targetPointer, objectClassData, &foundSuccess](void** instance, const SerializeContext::ClassData** classData, const Uuid& classId, SerializeContext* context)
            {
                void* convertibleInstance{};
                if (objectClassData->ConvertFromType(convertibleInstance, classId, targetPointer, *context))
                {
                    foundSuccess = true;
                    if (instance)
                    {
                        // The ObjectStream will ask us for the address of the target to load into, so provide it.
                        *instance = convertibleInstance;
                    }
                    if (classData)
                    {
                        // The ObjectStream will ask us for the class data of the target being loaded into, so provide it if needed.
                        // This allows us to load directly into a generic object (templated containers, strings, etc).
                        *classData = objectClassData;
                    }
                }
            };
        bool readSuccess = ObjectStream::LoadBlocking(&stream, *context, ObjectStream::ClassReadyCB(), filterDesc, CreationCallback(handler, AZ::OSStdAllocator()));

        AZ_Warning("Serialization", readSuccess, "LoadObjectFromStreamInPlace: Stream did not deserialize correctly");
        AZ_Warning("Serialization", foundSuccess, "LoadObjectFromStreamInPlace: Did not find the expected type in the stream");

        return readSuccess && foundSuccess;
    }

    bool LoadObjectFromStreamInPlace(IO::GenericStream& stream, AZ::SerializeContext* context, const Uuid& targetClassId, void* targetPointer, const FilterDescriptor& filterDesc)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        if (!context)
        {
            ComponentApplicationBus::BroadcastResult(context, &ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(context, "No serialize context");
        }

        if (!context)
        {
            return false;
        }

        const SerializeContext::ClassData* classData = context->FindClassData(targetClassId);
        if (!classData)
        {
            AZ_Error("Serialization", false,
                     "Unable to locate class data for uuid \"%s\". This object cannot be serialized as a root element. "
                     "Make sure the Uuid is valid, or if this is a generic type, use the override that takes a ClassData pointer instead.",
                     targetClassId.ToString<AZStd::string>().c_str());
            return false;
        }

        return LoadObjectFromStreamInPlace(stream, context, classData, targetPointer, filterDesc);
    }

    bool LoadObjectFromFileInPlace(const AZStd::string& filePath, const Uuid& targetClassId, void* destination, AZ::SerializeContext* context /*= nullptr*/, const FilterDescriptor& filterDesc /*= FilterDescriptor()*/)
    {
        AZ::IO::FileIOStream fileStream;
        if (!fileStream.Open(filePath.c_str(), IO::OpenMode::ModeRead | IO::OpenMode::ModeBinary))
        {
            return false;
        }

        return LoadObjectFromStreamInPlace(fileStream, context, targetClassId, destination, filterDesc);
    }

    void* LoadObjectFromStream(IO::GenericStream& stream, AZ::SerializeContext* context, const Uuid* targetClassId, const FilterDescriptor& filterDesc)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        if (!context)
        {
            ComponentApplicationBus::BroadcastResult(context, &ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(context, "No serialize context");
        }

        if (!context)
        {
            return nullptr;
        }

        void* loadedInstance = nullptr;
        bool success = ObjectStream::LoadBlocking(&stream, *context,
                [&loadedInstance, targetClassId](void* classPtr, const Uuid& classId, const SerializeContext* serializeContext)
                {
                    if (targetClassId)
                    {
                        void* instance = serializeContext->DownCast(classPtr, classId, *targetClassId);

                        // Given a valid object
                        if (instance)
                        {
                            AZ_Assert(!loadedInstance, "loadedInstance must be NULL, otherwise we are being invoked with multiple valid objects");
                            loadedInstance = instance;
                            return;
                        }
                    }
                    else
                    {
                        if (!loadedInstance)
                        {
                            loadedInstance = classPtr;
                            return;
                        }
                    }

                    auto classData = serializeContext->FindClassData(classId);
                    if (classData && classData->m_factory)
                    {
                        classData->m_factory->Destroy(classPtr);
                    }
                },
                filterDesc,
                ObjectStream::InplaceLoadRootInfoCB()
                );

        if (!success)
        {
            return nullptr;
        }

        return loadedInstance;
    }

    void* LoadObjectFromFile(const AZStd::string& filePath, const Uuid& targetClassId, SerializeContext* context, const FilterDescriptor& filterDesc, int /*platformFlags*/)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        AZ::IO::FileIOStream fileStream;
        if (!fileStream.Open(filePath.c_str(), IO::OpenMode::ModeRead | IO::OpenMode::ModeBinary))
        {
            return nullptr;
        }

        void* loadedObject = LoadObjectFromStream(fileStream, context, &targetClassId, filterDesc);
        return loadedObject;
    }

    bool SaveObjectToStream(IO::GenericStream& stream, DataStream::StreamType streamType, const void* classPtr, const Uuid& classId, SerializeContext* context, const SerializeContext::ClassData* classData)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        if (!context)
        {
            ComponentApplicationBus::BroadcastResult(context, &ComponentApplicationBus::Events::GetSerializeContext);

            if(!context)
            {
                AZ_Assert(false, "No serialize context");
                return false;
            }
        }

        if (!classPtr)
        {
            AZ_Assert(false, "SaveObjectToStream: classPtr is null, object cannot be serialized.");
            return false;
        }

        AZ::ObjectStream* objectStream = AZ::ObjectStream::Create(&stream, *context, streamType);
        if (!objectStream)
        {
            return false;
        }

        if (!objectStream->WriteClass(classPtr, classId, classData))
        {
            objectStream->Finalize();
            return false;
        }

        if (!objectStream->Finalize())
        {
            return false;
        }

        return true;
    }

    bool SaveStreamToFile(const AZStd::string& filePath, const AZStd::vector<AZ::u8>& streamData, int platformFlags)
    {
        AZ::IO::FileIOBase* fileIo = AZ::IO::FileIOBase::GetInstance();
        AZ::IO::FixedMaxPathString resolvedPath;
        if (fileIo == nullptr || !fileIo->ResolvePath(filePath.c_str(), resolvedPath.data(), resolvedPath.capacity() + 1))
        {
            resolvedPath = filePath;
        }
        if (AZ::IO::SystemFile fileHandle; fileHandle.Open(resolvedPath.c_str(),
            AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY,
            platformFlags))
        {
            AZ::IO::SizeType bytesWritten = fileHandle.Write(streamData.data(), streamData.size());
            return bytesWritten == streamData.size();
        }

        return false;
    }

    bool SaveObjectToFile(const AZStd::string& filePath, DataStream::StreamType fileType, const void* classPtr, const Uuid& classId, SerializeContext* context, int platformFlags)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        // \note This is ok for tools, but we should use the streamer to write objects directly (no memory store)
        AZStd::vector<AZ::u8> dstData;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > dstByteStream(&dstData);

        if (!SaveObjectToStream(dstByteStream, fileType, classPtr, classId, context))
        {
            return false;
        }

        return SaveStreamToFile(filePath, dstData, platformFlags);
    }

    /*!
    \brief Finds any descendant DataElementNodes of the @classElement which match each Crc32 values in the supplied elementCrcQueue
    \param context SerializeContext used for looking up ClassData
    \param classElement Top level DataElementNode to begin comparison each the Crc32 queue
    \param elementCrcQueue Container of Crc32 values in the order in which DataElementNodes should be matched as the DataElementNode tree is traversed
    \return Vector of valid pointers to DataElementNodes which match the entire element Crc32 queue
    */
    AZStd::vector<AZ::SerializeContext::DataElementNode*> FindDescendantElements(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement,
        const AZStd::vector<AZ::Crc32>& elementCrcQueue)
    {
        AZStd::vector<AZ::SerializeContext::DataElementNode*> dataElementNodes;
        FindDescendantElements(context, classElement, dataElementNodes, elementCrcQueue.begin(), elementCrcQueue.end());

        return dataElementNodes;
    }

    /*!
    \brief Finds any descendant DataElementNodes of the @classElement which match each Crc32 values in the supplied elementCrcQueue
    \param context SerializeContext used for looking up ClassData
    \param classElement The current DataElementNode which will be compared against be to current top Crc32 value in the Crc32 queue
    \param dataElementNodes[out] Array to populate with a DataElementNode which was found by matching all Crc32 values in the Crc32 queue
    \param first The current front of the Crc32 queue
    \param last The end of the Crc32 queue
    */
    void FindDescendantElements(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement,
        AZStd::vector<AZ::SerializeContext::DataElementNode*>& dataElementNodes, AZStd::vector<AZ::Crc32>::const_iterator first, AZStd::vector<AZ::Crc32>::const_iterator last)
    {
        if (first == last)
        {
            return;
        }

        for (int i = 0; i < classElement.GetNumSubElements(); ++i)
        {
            auto& childElement = classElement.GetSubElement(i);
            if (*first == AZ::Crc32(childElement.GetName()))
            {
                if (AZStd::distance(first, last) == 1)
                {
                    dataElementNodes.push_back(&childElement);
                }
                else
                {
                    FindDescendantElements(context, childElement, dataElementNodes, AZStd::next(first), last);
                }
            }
        }
    }

    bool IsVectorContainerType(const AZ::Uuid& type)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        if (serializeContext)
        {
            AZ::TypeId containerTypeId = type;

            if (GenericClassInfo* classInfo = serializeContext->FindGenericClassInfo(type))
            {
                // This type is a container
                containerTypeId = classInfo->GetGenericTypeId();
            }

            if (containerTypeId == AZ::GetGenericClassInfoVectorTypeId()
                || containerTypeId == AZ::GetGenericClassInfoFixedVectorTypeId()
                || containerTypeId == AZ::GetGenericClassInfoArrayTypeId()
                )
            {
                return true;
            }
        }

        return false;
    }

    bool IsSetContainerType(const AZ::Uuid& type)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        if (serializeContext)
        {
            AZ::TypeId containerTypeId = type;

            if (GenericClassInfo* classInfo = serializeContext->FindGenericClassInfo(type))
            {
                // This type is a container
                containerTypeId = classInfo->GetGenericTypeId();
            }

            if (containerTypeId == AZ::GetGenericClassSetTypeId()
                || containerTypeId == AZ::GetGenericClassUnorderedSetTypeId()
                )
            {
                return true;
            }
        }

        return false;
    }


    bool IsMapContainerType(const AZ::Uuid& type)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        if (serializeContext)
        {
            AZ::Uuid containerTypeId = type;

            if (GenericClassInfo* classInfo = serializeContext->FindGenericClassInfo(type))
            {
                containerTypeId = classInfo->GetGenericTypeId();
            }

            if (containerTypeId == AZ::GetGenericClassMapTypeId()
                || containerTypeId == AZ::GetGenericClassUnorderedMapTypeId()
                )
            {
                return true;
            }
        }

        return false;
    }

    bool IsContainerType(const AZ::Uuid& type)
    {
        return IsVectorContainerType(type) || IsSetContainerType(type) || IsMapContainerType(type);
    }

    AZStd::vector<AZ::Uuid> GetContainedTypes(const AZ::Uuid& type)
    {
        AZStd::vector<AZ::Uuid> types;

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        if (serializeContext)
        {
            if (GenericClassInfo* classInfo = serializeContext->FindGenericClassInfo(type))
            {
                for (int i = 0; i < classInfo->GetNumTemplatedArguments(); ++i)
                {
                    types.push_back(classInfo->GetTemplatedTypeId(i));
                }
            }
        }

        return types;
    }

    bool IsOutcomeType(const AZ::Uuid& type)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        if (serializeContext)
        {
            GenericClassInfo* classInfo = serializeContext->FindGenericClassInfo(type);
            return classInfo && classInfo->GetGenericTypeId() == AZ::GetGenericOutcomeTypeId();
        }

        return false;
    }

    bool IsPairContainerType(const AZ::Uuid& type)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        if (serializeContext)
        {
            AZ::Uuid containerTypeId = type;

            if (GenericClassInfo* classInfo = serializeContext->FindGenericClassInfo(type))
            {
                containerTypeId = classInfo->GetGenericTypeId();
            }

            if (containerTypeId == AZ::GetGenericClassPairTypeId())
            {
                return true;
            }
        }

        return false;
    }

    bool IsTupleContainerType(const AZ::Uuid& type)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        if (serializeContext)
        {
            GenericClassInfo* classInfo = serializeContext->FindGenericClassInfo(type);
            return classInfo && classInfo->GetGenericTypeId() == AZ::GetGenericClassTupleTypeId();
        }

        return false;
    }

    AZ::TypeId GetGenericContainerType(const AZ::TypeId& type)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        if (serializeContext)
        {
            if (GenericClassInfo* classInfo = serializeContext->FindGenericClassInfo(type))
            {
                // This type is a container
                return classInfo->GetGenericTypeId();
            }
        }

        return azrtti_typeid<void>();
    }

    bool IsGenericContainerType(const AZ::TypeId& type)
    {
        return IsContainerType(type) && GetGenericContainerType(type) == azrtti_typeid<void>();
    }

    AZStd::pair<AZ::Uuid, AZ::Uuid> GetOutcomeTypes(const AZ::Uuid& type)
    {
        AZStd::vector<AZ::Uuid> types;

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        if (serializeContext)
        {
            if (GenericClassInfo* classInfo = serializeContext->FindGenericClassInfo(type))
            {
                AZ_Assert(classInfo->GetNumTemplatedArguments() == 2, "Outcome template arguments must be 2, even if void, void");
                return AZStd::make_pair(classInfo->GetTemplatedTypeId(0), classInfo->GetTemplatedTypeId(1));
            }
        }

        return AZStd::make_pair(azrtti_typeid<void>(), azrtti_typeid<void>());
    }

    void* ResolvePointer(void* ptr, const SerializeContext::ClassElement& classElement, const SerializeContext& context)
    {
        if (classElement.m_flags & SerializeContext::ClassElement::FLG_POINTER)
        {
            // In the case of pointer-to-pointer, we'll deference.
            ptr = *(void**)(ptr);

            // Pointer-to-pointer fields may be base class / polymorphic, so cast pointer to actual type,
            // safe for passing as 'this' to member functions.
            if (ptr && classElement.m_azRtti)
            {
                Uuid actualClassId = classElement.m_azRtti->GetActualUuid(ptr);
                if (actualClassId != classElement.m_typeId)
                {
                    const SerializeContext::ClassData* classData = context.FindClassData(actualClassId);
                    if (classData)
                    {
                        ptr = classElement.m_azRtti->Cast(ptr, classData->m_azRtti->GetTypeId());
                    }
                }
            }
        }

        return ptr;
    }

    bool InspectSerializedFile(
        const char* filePath,
        SerializeContext* sc,
        const ObjectStream::ClassReadyCB& classCallback,
        Data::AssetFilterCB assetFilterCallback)
    {
        AZ::IO::FileIOStream stream(filePath, AZ::IO::OpenMode::ModeRead);
        if (!stream.IsOpen())
        {
            AZ_Error("Verify", false, "File '%s' could not be opened.", filePath);
            return false;
        }

        ObjectStream::FilterDescriptor filter;
        // By default, never load dependencies. That's another file that would need to be processed
        // separately from this one.
        filter.m_assetCB = assetFilterCallback;
        if (!ObjectStream::LoadBlocking(&stream, *sc, classCallback, filter))
        {
            AZ_Printf("Verify", "Failed to deserialize '%s'\n", filePath);
            return false;
        }

        return true;
    }

    AZ::Attribute* FindEditOrSerializeContextAttribute(AZ::AttributeId attributeId, const AZ::SerializeContext::ClassData* classData,
        const AZ::SerializeContext::ClassElement* classElement, const AZ::Edit::ClassData* editClassData, const AZ::Edit::ElementData* elementEditData)
    {
        // Locates attributes by searching in the following order:
        // 1) Class element edit data attributes (EditContext from the given row of a class)
        // 2) Class element data attributes (SerializeContext from the given row of a class)
        // 3) Edit Class data attributes (the attributes added to a EditContext reflected class "EditorData" element)
        // 4) Class data attributes (the base attributes of a class)
        if (elementEditData != nullptr)
        {
            if (AZ::Edit::Attribute* editAttribute = elementEditData->FindAttribute(attributeId);
                editAttribute != nullptr)
            {
                return editAttribute;
            }
        }

        if (classElement != nullptr)
        {
            if (auto classFieldAttribute = classElement->FindAttribute(attributeId);
                classFieldAttribute != nullptr)
            {
                return classFieldAttribute;
            }
        }

        if (classData != nullptr)
        {
            if (auto classDataAttribute = classData->FindAttribute(attributeId);
                classDataAttribute != nullptr)
            {
                return classDataAttribute;
            }
        }
        // Lastly, check the AZ::SerializeContext::ClassData -> AZ::Edit::ClassData -> EditorData for attributes
        if (editClassData != nullptr)
        {
            if (const auto* classEditorData = editClassData->FindElementData(AZ::Edit::ClassElements::EditorData))
            {
                if (auto editClassAttribute = classEditorData->FindAttribute(attributeId);
                    editClassAttribute != nullptr)
                {
                    return editClassAttribute;
                }
            }
        }

        return nullptr;
    }

    AZ::Attribute* FindEditOrSerializeContextAttribute(AZ::AttributeId attributeId, const AZ::SerializeContext::ClassData* classData,
        const AZ::SerializeContext::ClassElement* classElement, const AZ::Edit::ElementData* elementEditData)
    {
        const AZ::Edit::ClassData* editClassData = classData != nullptr ? classData->m_editData : nullptr;
        return FindEditOrSerializeContextAttribute(attributeId, classData, classElement, editClassData, elementEditData);
    }

    AZ::Attribute* FindEditOrSerializeContextAttribute(AZ::AttributeId attributeId, const AZ::SerializeContext::ClassData* classData,
        const AZ::SerializeContext::ClassElement* classElement, const AZ::Edit::ClassData* editClassData)
    {
        const AZ::Edit::ElementData* elementEditData = classElement != nullptr ? classElement->m_editData : nullptr;
        return FindEditOrSerializeContextAttribute(attributeId, classData, classElement, editClassData, elementEditData);
    }

    AZ::Attribute* FindEditOrSerializeContextAttribute(AZ::AttributeId attributeId, const AZ::SerializeContext::ClassData* classData,
        const AZ::SerializeContext::ClassElement* classElement)
    {
        const AZ::Edit::ClassData* editClassData = classData != nullptr ? classData->m_editData : nullptr;
        const AZ::Edit::ElementData* elementEditData = classElement != nullptr ? classElement->m_editData : nullptr;
        return FindEditOrSerializeContextAttribute(attributeId, classData, classElement, editClassData, elementEditData);
    }

} // namespace AZ::Utils
