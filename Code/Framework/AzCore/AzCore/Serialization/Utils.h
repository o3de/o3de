/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace IO
    {
        class GenericStream;
    }

    namespace Utils
    {
        typedef AZ::ObjectStream::FilterDescriptor FilterDescriptor;

        void* LoadObjectFromStream(IO::GenericStream& stream, SerializeContext* context = nullptr, const Uuid* targetClassId = nullptr, const FilterDescriptor& filterDesc = FilterDescriptor());
        bool LoadObjectFromStreamInPlace(IO::GenericStream& stream, AZ::SerializeContext* context, const Uuid& targetClassId, void* targetPointer, const FilterDescriptor& filterDesc = FilterDescriptor());
        bool LoadObjectFromStreamInPlace(IO::GenericStream& stream, AZ::SerializeContext* context, const SerializeContext::ClassData* objectClassData, void* targetPointer, const FilterDescriptor& filterDesc = FilterDescriptor());

        template <typename ObjectType>
        ObjectType* LoadObjectFromStream(IO::GenericStream& stream, SerializeContext* context = nullptr, const FilterDescriptor& filterDesc = FilterDescriptor())
        {
            const Uuid objectTypeId = AzTypeInfo<ObjectType>::Uuid();
            return reinterpret_cast<ObjectType*>(LoadObjectFromStream(stream, context, &objectTypeId, filterDesc));
        }

        template <typename ObjectType>
        ObjectType* LoadObjectFromBuffer(const void* buffer, const AZStd::size_t length, SerializeContext* context = nullptr, const FilterDescriptor& filterDesc = FilterDescriptor())
        {
            AZ::IO::MemoryStream stream(buffer, length);
            return LoadObjectFromStream<ObjectType>(stream, context, filterDesc);
        }

        void* LoadObjectFromFile(const AZStd::string& filePath, const Uuid& targetClassId, SerializeContext* context = nullptr, const FilterDescriptor& filterDesc = FilterDescriptor(), int platformFlags = 0);

        template <typename ObjectType>
        ObjectType* LoadObjectFromFile(const AZStd::string& filePath, SerializeContext* context = nullptr, const FilterDescriptor& filterDesc = FilterDescriptor(), int platformFlags = 0)
        {
            return reinterpret_cast<ObjectType*>(LoadObjectFromFile(filePath, AzTypeInfo<ObjectType>::Uuid(), context, filterDesc, platformFlags));
        }

        bool SaveObjectToStream(IO::GenericStream& stream, DataStream::StreamType streamType, const void* classPtr, const Uuid& classId, SerializeContext* context = nullptr, const SerializeContext::ClassData* classData = nullptr);

        template <typename ObjectType>
        bool SaveObjectToStream(IO::GenericStream& stream, DataStream::StreamType streamType, const ObjectType* classPtr, SerializeContext* context = nullptr, const SerializeContext::ClassData* classData = nullptr)
        {
            return SaveObjectToStream(stream, streamType, classPtr, AzTypeInfo<ObjectType>::Uuid(), context, classData);
        }

        bool SaveStreamToFile(const AZStd::string& filePath, const AZStd::vector<AZ::u8>& streamData, int platformFlags = 0);

        bool SaveObjectToFile(const AZStd::string& filePath, DataStream::StreamType fileType, const void* classPtr, const Uuid& classId, SerializeContext* context = nullptr, int platformFlags = 0);

        template <typename ObjectType>
        bool SaveObjectToFile(const AZStd::string& filePath, DataStream::StreamType fileType, const ObjectType* classPtr, SerializeContext* context = nullptr, int platformFlags = 0)
        {
            return SaveObjectToFile(filePath, fileType, classPtr, AzTypeInfo<ObjectType>::Uuid(), context, platformFlags);
        }

        template <typename ObjectType>
        bool LoadObjectFromStreamInPlace(IO::GenericStream& stream, ObjectType& destination, AZ::SerializeContext* context = nullptr, const FilterDescriptor& filterDesc = FilterDescriptor())
        {
            return LoadObjectFromStreamInPlace(stream, context, AzTypeInfo<ObjectType>::Uuid(), &destination, filterDesc);
        }

        template <typename ObjectType>
        bool LoadObjectFromBufferInPlace(const void* buffer, const AZStd::size_t length, ObjectType& destination, AZ::SerializeContext* context = nullptr, const FilterDescriptor& filterDesc = FilterDescriptor())
        {
            AZ::IO::MemoryStream stream(buffer, length);
            return LoadObjectFromStreamInPlace(stream, context, AzTypeInfo<ObjectType>::Uuid(), &destination, filterDesc);
        }

        AZStd::vector<AZ::SerializeContext::DataElementNode*> FindDescendantElements(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement, const AZStd::vector<AZ::Crc32>& elementCrcQueue);
        void FindDescendantElements(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement, AZStd::vector<AZ::SerializeContext::DataElementNode*>& dataElementNodes, AZStd::vector<AZ::Crc32>::const_iterator first, AZStd::vector<AZ::Crc32>::const_iterator last);
        bool LoadObjectFromFileInPlace(const AZStd::string& filePath, const Uuid& targetClassId, void* destination, AZ::SerializeContext* context = nullptr, const FilterDescriptor& filterDesc = FilterDescriptor());

        template <typename ObjectType>
        bool LoadObjectFromFileInPlace(const AZStd::string& filePath, ObjectType& destination, AZ::SerializeContext* context = nullptr, const FilterDescriptor& filterDesc = FilterDescriptor())
        {
            return LoadObjectFromFileInPlace(filePath, AzTypeInfo<ObjectType>::Uuid(), &destination, context, filterDesc);
        }

        template<class T>
        bool GetEnumStringRepresentation(
            AZStd::string& value, const AZ::Edit::ElementData* data, void* instance, const AZ::Uuid& storageTypeId)
        {
            if (storageTypeId == azrtti_typeid<T>())
            {
                for (const AZ::AttributePair& attributePair : data->m_attributes)
                {
                    AZ::AttributeReader reader(instance, attributePair.second);
                    AZ::Edit::EnumConstant<T> enumPair;
                    if (reader.Read<AZ::Edit::EnumConstant<T>>(enumPair))
                    {
                        T* enumValue = reinterpret_cast<T*>(instance);
                        if (static_cast<T>(enumPair.m_value) == *enumValue)
                        {
                            value = enumPair.m_description;
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        // Try GetEnumStringRepresentation<Type> on all of the specified types
        template<class T1, class T2, class... TRest>
        bool GetEnumStringRepresentation(
            AZStd::string& value, const AZ::Edit::ElementData* data, void* instance, const AZ::Uuid& storageTypeId)
        {
            return GetEnumStringRepresentation<T1>(value, data, instance, storageTypeId) ||
                GetEnumStringRepresentation<T2, TRest...>(value, data, instance, storageTypeId);
        }

        bool IsVectorContainerType(const AZ::Uuid& type);
        bool IsSetContainerType(const AZ::Uuid& type);
        bool IsMapContainerType(const AZ::Uuid& type);
        bool IsContainerType(const AZ::Uuid& type);
        bool IsOutcomeType(const AZ::Uuid& type);
        bool IsPairContainerType(const AZ::Uuid& type);
        bool IsTupleContainerType(const AZ::Uuid& type);

        AZ::TypeId GetGenericContainerType(const AZ::TypeId& type);
        bool IsGenericContainerType(const AZ::TypeId& type);
        AZStd::pair<AZ::Uuid, AZ::Uuid> GetOutcomeTypes(const AZ::Uuid& type);
        AZStd::vector<AZ::Uuid> GetContainedTypes(const AZ::Uuid& type);

        /// Resolve the instance pointer for a given ClassElement by casting it to the actual type
        /// expected by the ClassData for this element
        void* ResolvePointer(void* ptr, const SerializeContext::ClassElement& classElement, const SerializeContext& context);

        //! Open the given file and load it into an ObjectStream that you can inspect
        //! @param classCallback Called for each root object loaded via the ObjectStream. Use it to read values from the file.
        //! @param assetFilterCallback Limit the processing/loading to specific asset type(s)
        bool InspectSerializedFile(
            const char* filePath,
            SerializeContext* sc,
            const ObjectStream::ClassReadyCB& classCallback,
            Data::AssetFilterCB assetFilterCallback = AZ::Data::AssetFilterNoAssetLoading);
    } // namespace Utils
} // namespace Az


namespace AZ::Utils
{
    //! Search the Edit context and Serialize Context attributes of class for the first attribute
    //! matching the attributeId parameter
    //! Locates attributes by searching in the following order:
    //! 1) Class element edit data attributes (EditContext from the given row of a class)
    //! 2) Class element data attributes (SerializeContext from the given row of a class)
    //! 3) Edit Class data attributes (the attributes added to a EditContext reflected class "EditorData" element)
    //! 4) Class data attributes (the base attributes of a class)
    //! @param attributeId numeric ID of attribute to lookup. A string can be converted to an AttributeId by converting it to `AZ::Crc32`
    //! @param classData Serialize Context reflected ClassData structure
    //! @param classElement Serialize Context field element for the field being examined. This is added via the SerializeContext
    //! ClassBuilder `Field` function
    //! @param editClassData Edit Context reflected ClassData structure
    //! @param elementEditData Edit Context data element for a specific field. This is added via the EditContext Class Builder `DataElement`
    //! function
    //! @return first attribute which matches the specified attribute ID or nullptr
    AZ::Attribute* FindEditOrSerializeContextAttribute(
        AZ::AttributeId attributeId,
        const AZ::SerializeContext::ClassData* classData,
        const AZ::SerializeContext::ClassElement* classElement,
        const AZ::Edit::ClassData* editClassData,
        const AZ::Edit::ElementData* elementEditData);
    AZ::Attribute* FindEditOrSerializeContextAttribute(
        AZ::AttributeId attributeId,
        const AZ::SerializeContext::ClassData* classData,
        const AZ::SerializeContext::ClassElement* classElement,
        const AZ::Edit::ElementData* elementEditData);

    AZ::Attribute* FindEditOrSerializeContextAttribute(
        AZ::AttributeId attributeId,
        const AZ::SerializeContext::ClassData* classData,
        const AZ::SerializeContext::ClassElement* classElement,
        const AZ::Edit::ClassData* editClassData);

    AZ::Attribute* FindEditOrSerializeContextAttribute(
        AZ::AttributeId attributeId,
        const AZ::SerializeContext::ClassData* classData,
        const AZ::SerializeContext::ClassElement* classElement);
} // namespace AZ::Utils
