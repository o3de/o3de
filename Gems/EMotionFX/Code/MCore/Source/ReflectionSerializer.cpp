/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReflectionSerializer.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Utils.h>

namespace MCore
{
    const AZ::SerializeContext::ClassElement* RecursivelyFindClassElement(const AZ::SerializeContext* context, const AZ::SerializeContext::ClassData* parentClassData, const AZ::Crc32 nameCrc)
    {
        // Find in parentClassData
        for (const AZ::SerializeContext::ClassElement& classElement : parentClassData->m_elements)
        {
            if (classElement.m_nameCrc == nameCrc)
            {
                return &classElement;
            }
        }
        // Check in base classes
        for (const AZ::SerializeContext::ClassElement& classElement : parentClassData->m_elements)
        {
            if (classElement.m_flags & AZ::SerializeContext::ClassElement::FLG_BASE_CLASS)
            {
                const AZ::SerializeContext::ClassData* baseClassData = context->FindClassData(classElement.m_typeId);
                const AZ::SerializeContext::ClassElement* baseResult = RecursivelyFindClassElement(context, baseClassData, nameCrc);
                if (baseResult)
                {
                    return baseResult;
                }
            }
        }
        // not found
        return nullptr;
    }

    const AZStd::vector<const AZ::SerializeContext::ClassElement*> GetChildClassElements(const AZ::SerializeContext* context, const AZ::SerializeContext::ClassData* parentClassData)
    {
        AZStd::vector<const AZ::SerializeContext::ClassElement*> childClassElements;
        for (const AZ::SerializeContext::ClassElement& classElement : parentClassData->m_elements)
        {
            if (classElement.m_flags & AZ::SerializeContext::ClassElement::FLG_BASE_CLASS)
            {
                const AZ::SerializeContext::ClassData* baseClassData = context->FindClassData(classElement.m_typeId);
                AZStd::vector<const AZ::SerializeContext::ClassElement*> classElements = GetChildClassElements(context, baseClassData);
                AZStd::copy(classElements.begin(), classElements.end(), AZStd::back_inserter(childClassElements));
            }
            else
            {
                childClassElements.emplace_back(&classElement);
            }
        }
        return childClassElements;
    }

    AZ::Outcome<AZStd::string> ReflectionSerializer::SerializeMember(const AZ::TypeId& classTypeId, const void* classPtr, const char* memberName)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return AZ::Failure();
        }

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(classTypeId);
        AZ_Assert(classData, "Expected valid class data, is the type reflected?");

        const AZ::Crc32 nameCrc(memberName);
        const AZ::SerializeContext::ClassElement* classElement = RecursivelyFindClassElement(context, classData, nameCrc);
        if (classElement)
        {
            const AZ::SerializeContext::ClassData* classDataElement = context->FindClassData(classElement->m_typeId);
            if (classDataElement)
            {
                if (classDataElement->m_serializer)
                {
                    AZStd::vector<char> inBuffer;
                    AZ::IO::ByteContainerStream<AZStd::vector<char>> inStream(&inBuffer);
                    classDataElement->m_serializer->Save(static_cast<const char*>(classPtr) + classElement->m_offset, inStream);
                    inStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

                    AZStd::string outBuffer;
                    AZ::IO::ByteContainerStream<AZStd::string> outStream(&outBuffer);
                    if (classDataElement->m_serializer->DataToText(inStream, outStream, false) != 0) // returns 0 if it failed
                    {
                        return AZ::Success(outBuffer);
                    }
                }
                else
                {
                    AZStd::string outBuffer;
                    AZ::IO::ByteContainerStream<AZStd::string> outStream(&outBuffer);
                    if (AZ::Utils::SaveObjectToStream(outStream, AZ::ObjectStream::ST_XML, static_cast<const char*>(classPtr) + classElement->m_offset, classDataElement->m_typeId))
                    {
                        return AZ::Success(outBuffer);
                    }
                }
            }
        }
        return AZ::Failure();
    }

    AZ::Outcome<AZStd::string> ReflectionSerializer::SerializeMembersExcept(const AZ::TypeId& classTypeId, const void* classPtr, const AZStd::vector<AZStd::string>& excludeMembers)
    {
        AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> membersAndValues;
        
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return AZ::Failure();
        }

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(classTypeId);
        AZ_Assert(classData, "Expected valid class data, is the type reflected?");

        AZStd::unordered_set<AZ::Crc32> excludedMemberCrcs;
        for (const AZStd::string& memberName : excludeMembers)
        {
            excludedMemberCrcs.emplace(AZ::Crc32(memberName.c_str()));
        }
        const AZStd::vector<const AZ::SerializeContext::ClassElement*> childMembers = GetChildClassElements(context, classData);
        for (const AZ::SerializeContext::ClassElement* classElement : childMembers)
        {
            if (excludedMemberCrcs.find(classElement->m_nameCrc) == excludedMemberCrcs.end())
            {
                const AZ::SerializeContext::ClassData* classDataElement = context->FindClassData(classElement->m_typeId);
                if (!classDataElement && classElement->m_genericClassInfo)
                {
                    classDataElement = classElement->m_genericClassInfo->GetClassData();
                }

                if (classDataElement)
                {
                    if (classDataElement->m_serializer)
                    {
                        AZStd::vector<char> inBuffer;
                        AZ::IO::ByteContainerStream<AZStd::vector<char>> inStream(&inBuffer);
                        classDataElement->m_serializer->Save(static_cast<const char*>(classPtr) + classElement->m_offset, inStream);
                        inStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

                        AZStd::string outBuffer;
                        AZ::IO::ByteContainerStream<AZStd::string> outStream(&outBuffer);
                        if (classDataElement->m_serializer->DataToText(inStream, outStream, false) != 0) // returns 0 if it failed
                        {
                            membersAndValues.emplace_back(classElement->m_name, outBuffer);
                        }
                    }
                    else
                    {
                        AZStd::string outBuffer;
                        AZ::IO::ByteContainerStream<AZStd::string> outStream(&outBuffer);
                        if (AZ::Utils::SaveObjectToStream(outStream, AZ::ObjectStream::ST_XML, static_cast<const char*>(classPtr) + classElement->m_offset, classElement->m_typeId))
                        {
                            membersAndValues.emplace_back(classElement->m_name, outBuffer);
                        }
                    }
                }
            }
        }

        return Serialize(&membersAndValues);
    }

    bool ReflectionSerializer::DeserializeIntoMember(const AZ::TypeId& classTypeId, void* classPtr, const char* memberName, const AZStd::string& value)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return false;
        }

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(classTypeId);
        AZ_Assert(classData, "Expected valid class data, is the type reflected?");

        const AZ::Crc32 nameCrc(memberName);
        const AZ::SerializeContext::ClassElement* classElement = RecursivelyFindClassElement(context, classData, nameCrc);
        if (classElement)
        {
            const AZ::SerializeContext::ClassData* classDataElement = context->FindClassData(classElement->m_typeId);
            if (!classDataElement && classElement->m_genericClassInfo)
            {
                classDataElement = classElement->m_genericClassInfo->GetClassData();
            }

            if (classDataElement->m_serializer)
            {
                AZStd::vector<AZ::u8> byteArray;
                AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> convertedStream(&byteArray);
                classDataElement->m_serializer->TextToData(value.c_str(), 0, convertedStream);
                convertedStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
                return classDataElement->m_serializer->Load(static_cast<char*>(classPtr) + classElement->m_offset, convertedStream, 0);
            }
            else
            {
                AZ::IO::ByteContainerStream<const AZStd::string> inputStream(&value);
                return AZ::Utils::LoadObjectFromStreamInPlace(inputStream, context, classElement->m_typeId, static_cast<char*>(classPtr) + classElement->m_offset);
            }
        }
        return false;
    }

    AZ::Outcome<AZStd::string> ReflectionSerializer::Serialize(const AZ::TypeId& classTypeId, const void* classPtr)
    {
        AZStd::string destinationBuffer;
        AZ::IO::ByteContainerStream<AZStd::string> byteStream(&destinationBuffer);
        if (AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_XML, classPtr, classTypeId))
        {
            return AZ::Success(destinationBuffer);
        }
        else
        {
            return AZ::Failure();
        }
    }

    bool ReflectionSerializer::Deserialize(const AZ::TypeId& classTypeId, void* classPtr, const AZStd::string& sourceBuffer)
    {
        AZ::IO::ByteContainerStream<const AZStd::string> byteStream(&sourceBuffer);
        return AZ::Utils::LoadObjectFromStreamInPlace(byteStream, nullptr, classTypeId, classPtr);
    }

    void RecursivelyGetClassElement(AZStd::vector<const AZ::SerializeContext::ClassElement*>& elements, const AZ::SerializeContext* context, const AZ::SerializeContext::ClassData* parentClassData)
    {
        // Check in base classes
        for (const AZ::SerializeContext::ClassElement& classElement : parentClassData->m_elements)
        {
            if (classElement.m_flags & AZ::SerializeContext::ClassElement::FLG_BASE_CLASS)
            {
                const AZ::SerializeContext::ClassData* baseClassData = context->FindClassData(classElement.m_typeId);
                RecursivelyGetClassElement(elements, context, baseClassData);
            }
            else
            {
                elements.emplace_back(&classElement);
            }
        }
    }

    AZ::Outcome<AZStd::unordered_map<AZStd::string, AZStd::string>> ReflectionSerializer::SerializeIntoMap(const AZ::TypeId& classTypeId, const void* classPtr)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return AZ::Failure();
        }

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(classTypeId);
        AZ_Assert(classData, "Expected valid class data, is the type reflected?");

        AZStd::vector<const AZ::SerializeContext::ClassElement*> elements;
        RecursivelyGetClassElement(elements, context, classData);

        AZ::Outcome<AZStd::unordered_map<AZStd::string, AZStd::string>> result {AZStd::unordered_map<AZStd::string, AZStd::string>(elements.size())};

        for (const AZ::SerializeContext::ClassElement* element : elements)
        {
            const AZ::SerializeContext::ClassData* classDataElement = context->FindClassData(element->m_typeId);

            if (classDataElement)
            {
                if (classDataElement->m_serializer)
                {
                    AZStd::vector<char> inBuffer;
                    AZ::IO::ByteContainerStream<AZStd::vector<char>> inStream(&inBuffer);
                    classDataElement->m_serializer->Save(static_cast<const char*>(classPtr) + element->m_offset, inStream);
                    inStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

                    AZStd::string outBuffer;
                    AZ::IO::ByteContainerStream<AZStd::string> outStream(&outBuffer);
                    classDataElement->m_serializer->DataToText(inStream, outStream, false);
                    result.GetValue().emplace(element->m_name, outBuffer);
                }
                else
                {
                    AZStd::string outBuffer;
                    AZ::IO::ByteContainerStream<AZStd::string> outStream(&outBuffer);
                    if (!AZ::Utils::SaveObjectToStream(outStream, AZ::ObjectStream::ST_XML, static_cast<const char*>(classPtr), classData->m_typeId))
                    {
                        return AZ::Failure();
                    }
                    result.GetValue().emplace(element->m_name, outBuffer);
                }
            }
        }
        return result;
    }

    AZ::Outcome<AZStd::string> ReflectionSerializer::SerializeIntoCommandLine(const AZ::TypeId& classTypeId, const void* classPtr)
    {
        const auto& serializeMap = SerializeIntoMap(classTypeId, classPtr);
        if (!serializeMap.IsSuccess())
        {
            return AZ::Failure();
        }

        AZ::Outcome<AZStd::string> result {AZStd::string()};
        result.GetValue().reserve(1024);

        for (const AZStd::pair<AZStd::string, AZStd::string>& keyValuePair : serializeMap.GetValue())
        {
            result.GetValue() += AZStd::string::format("-%s {%s} ", keyValuePair.first.c_str(), keyValuePair.second.c_str());
        }

        if (!result.GetValue().empty())
        {
            result.GetValue().pop_back(); // remove the last space
        }
        return result;
    }

    AZ::Outcome<AZStd::string> ReflectionSerializer::SerializeValue(const AZ::TypeId& classTypeId, const void* classPtr)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return AZ::Failure();
        }

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(classTypeId);
        AZ_Assert(classData, "Expected valid class data, is the type reflected?");

        AZStd::string outBuffer;

        if (classData->m_serializer)
        {
            AZStd::vector<char> inBuffer;
            AZ::IO::ByteContainerStream<AZStd::vector<char>> inStream(&inBuffer);
            classData->m_serializer->Save(static_cast<const char*>(classPtr), inStream);
            inStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            AZ::IO::ByteContainerStream<AZStd::string> outStream(&outBuffer);
            if (!classData->m_serializer->DataToText(inStream, outStream, false))
            {
                AZ_Error("EMotionFX", false, "Error serializing type \"%s\"", classData->m_name);
                return AZ::Failure();
            }
        }
        else
        {
            AZ::IO::ByteContainerStream<AZStd::string> outStream(&outBuffer);
            AZ::Utils::SaveObjectToStream(outStream, AZ::ObjectStream::ST_XML, static_cast<const char*>(classPtr), classData->m_typeId);
        }

        return AZ::Outcome<AZStd::string>(outBuffer);
    }

    bool ReflectionSerializer::Deserialize(const AZ::TypeId& classTypeId, void* classPtr, const MCore::CommandLine& sourceCommandLine)
    {
        bool someError = false;
        const uint32 numParameters = sourceCommandLine.GetNumParameters();
        for (uint32 i = 0; i < numParameters; ++i)
        {
            someError |= !DeserializeIntoMember(classTypeId, classPtr, sourceCommandLine.GetParameterName(i).c_str(), sourceCommandLine.GetParameterValue(i).c_str());
        }
        // We are deserializing without checking if all the members can be deserialized, therefore this is not an atomic operation
        // If we need it to be atomic, we can serialize the class at the beginning, if something fails we roll it back
        return someError;
    }

    bool ReflectionSerializer::DeserializeMembers(const AZ::TypeId& classTypeId, void* classPtr, const AZStd::string& memberValuesMap)
    {
        AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> membersAndValues;
        if (Deserialize(&membersAndValues, memberValuesMap))
        {
            for (const AZStd::pair<AZStd::string, AZStd::string>& memberAndValue : membersAndValues)
            {
                DeserializeIntoMember(classTypeId, classPtr, memberAndValue.first.c_str(), memberAndValue.second);
            }
            return true;
        }

        return false;
    }

    void* ReflectionSerializer::Clone(const AZ::TypeId& classTypeId, const void* classPtr)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return nullptr;
        }

        return context->CloneObject(classPtr, classTypeId);
    }

    void ReflectionSerializer::CloneInplace(void* dest, const void* src, const AZ::Uuid& classId)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return;
        }

        context->CloneObjectInplace(dest, src, classId);
    }

    void* ReflectionSerializer::GetPointerToMember(AZ::SerializeContext* context, const AZ::TypeId& classTypeId, void* classPtr, const char* memberName)
    {
        AZ_Assert(context, "Invalid serialize context.");

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(classTypeId);
        AZ_Assert(classData, "Expected valid class data, is the type reflected?");

        const AZ::Crc32 nameCrc(memberName);
        const AZ::SerializeContext::ClassElement* classElement = RecursivelyFindClassElement(context, classData, nameCrc);
        if (classElement)
        {
            return static_cast<char*>(classPtr) + classElement->m_offset;
        }
        return nullptr;
    }

    void ReflectionSerializer::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            // Needed for SerializeMembersExcept() and the case that the generic type hasn't been
            // registered by any other system yet (Idempotent operation).
            using VectorOfPairOfStrings = AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>;
            serializeContext->RegisterGenericType<VectorOfPairOfStrings>();
        }
    }
} // namespace EMotionFX
