/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    //=========================================================================
    // EditContext
    // [10/26/2012]
    //=========================================================================
    EditContext::EditContext(SerializeContext& serializeContext)
        : m_serializeContext(serializeContext)
    {
    }

    //=========================================================================
    // ~EditContext
    // [10/26/2012]
    //=========================================================================
    EditContext::~EditContext()
    {
        for (ClassDataListType::iterator classIt = m_classData.begin(); classIt != m_classData.end(); ++classIt)
        {
            if (classIt->m_classData)
            {
                classIt->m_classData->m_editData = nullptr;
                classIt->m_classData = nullptr;
            }
            classIt->ClearElements();
        }
        for (auto& enumIt : m_enumData)
        {
            enumIt.second.ClearAttributes();
        }
        m_enumData.clear();
    }

    //=========================================================================
    // EditContext::EnumerateInstanceCallContext
    //=========================================================================
    EditContext::EnumerateInstanceCallContext::EnumerateInstanceCallContext(
        const SerializeContext::BeginElemEnumCB& beginElemCB,
        const SerializeContext::EndElemEnumCB& endElemCB,
        const EditContext* context,
        unsigned int accessFlags,
        SerializeContext::ErrorHandler* errorHandler)
        : m_beginElemCB(beginElemCB)
        , m_endElemCB(endElemCB)
        , m_accessFlags(accessFlags)
    {
        m_errorHandler = errorHandler ? errorHandler : &m_defaultErrorHandler;

        m_elementCallback = [this, context](void* ptr, const Uuid& classId, const SerializeContext::ClassData* classData, const SerializeContext::ClassElement* classElement) -> bool
        {
            return context->EnumerateInstance(this, ptr, classId, classData, classElement);
        };
    }

    //=========================================================================
    // ~RemoveClassData
    //=========================================================================
    void EditContext::RemoveClassData(SerializeContext::ClassData* classData)
    {
        Edit::ClassData* data = classData->m_editData;
        if (data)
        {
            data->m_classData->m_editData = nullptr;
            data->m_classData = nullptr;
            data->ClearElements();
            for (auto editClassDataIt = m_classData.begin(); editClassDataIt != m_classData.end(); ++editClassDataIt)
            {
                if (&(*editClassDataIt) == data)
                {
                    m_classData.erase(editClassDataIt);
                    break;
                }
            }
        }
    }

    //=========================================================================
    // GetEnumElementData
    //=========================================================================
    const Edit::ElementData* EditContext::GetEnumElementData(const AZ::Uuid& enumId) const
    {
        auto enumIt = m_enumData.find(enumId);
        const Edit::ElementData* data = nullptr;
        if (enumIt != m_enumData.end())
        {
            data = &enumIt->second;
        }
        return data;
    }

    //=========================================================================
    // EnumerateInstanceConst
    //=========================================================================
    bool EditContext::EnumerateInstanceConst(
        EnumerateInstanceCallContext* callContext,
        const void* ptr,
        const Uuid& classId,
        const SerializeContext::ClassData* classData,
        const SerializeContext::ClassElement* classElement) const
    {
        AZ_Assert((callContext->m_accessFlags & SerializeContext::ENUM_ACCESS_FOR_WRITE) == 0, "You are asking the serializer to lock the data for write but you only have a const pointer!");
        return EnumerateInstance(callContext, const_cast<void*>(ptr), classId, classData, classElement);
    }

    //=========================================================================
    // EnumerateInstance
    //=========================================================================
    bool EditContext::EnumerateInstance(
        EnumerateInstanceCallContext* callContext,
        void* ptr,
        const Uuid& classId,
        const SerializeContext::ClassData* classData,
        const SerializeContext::ClassElement* classElement) const
    {
        // if useClassData is provided, just use it, otherwise try to find it using the classId provided.
        void* objectPtr = ptr;
        const AZ::Uuid* classIdPtr = &classId;
        const SerializeContext::ClassData* dataClassInfo = classData;

        if (classElement)
        {
            // if we are a pointer, then we may be pointing to a derived type.
            if (classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER)
            {
                // if ptr is a pointer-to-pointer, cast its value to a void* (or const void*) and dereference to get to the actual object
                // pointer.
                objectPtr = *(void**)(ptr);

                if (!objectPtr)
                {
                    return true; // nothing to serialize
                }
                if (classElement->m_azRtti)
                {
                    const AZ::Uuid& actualClassId = classElement->m_azRtti->GetActualUuid(objectPtr);
                    if (actualClassId != *classIdPtr)
                    {
                        // we are pointing to derived type, adjust class data, uuid and pointer.
                        classIdPtr = &actualClassId;
                        dataClassInfo = m_serializeContext.FindClassData(actualClassId);
                        if ((dataClassInfo) && (dataClassInfo->m_azRtti)) // it could be missing RTTI if its deprecated.
                        {
                            objectPtr = classElement->m_azRtti->Cast(objectPtr, dataClassInfo->m_azRtti->GetTypeId());

                            AZ_Assert(
                                objectPtr, "Potential Data Loss: AZ_RTTI Cast to type %s Failed on element: %s", dataClassInfo->m_name,
                                classElement->m_name);
                            if (!objectPtr)
                            {
#if defined(AZ_ENABLE_SERIALIZER_DEBUG)
                                // Additional error information: Provide Type IDs and the serialization stack from our enumeration
                                AZStd::string sourceTypeID = dataClassInfo->m_typeId.ToString<AZStd::string>();
                                AZStd::string targetTypeID = classElement->m_typeId.ToString<AZStd::string>();
                                AZStd::string error = AZStd::string::format(
                                    "EnumarateElements RTTI Cast Error: %s -> %s", sourceTypeID.c_str(), targetTypeID.c_str());
                                callContext->m_errorHandler->ReportError(error.c_str());
#endif
                                return true; // RTTI Error. Continue serialization
                            }
                        }
                    }
                }
            }
        }

        if (!dataClassInfo)
        {
            dataClassInfo = m_serializeContext.FindClassData(*classIdPtr);
        }

#if defined(AZ_ENABLE_SERIALIZER_DEBUG)
        {
            DbgStackEntry de;
            de.m_dataPtr = objectPtr;
            de.m_uuidPtr = classIdPtr;
            de.m_elementName = classElement ? classElement->m_name : nullptr;
            de.m_classData = dataClassInfo;
            de.m_classElement = classElement;
            callContext->m_errorHandler->Push(de);
        }
#endif // AZ_ENABLE_SERIALIZER_DEBUG

        if (dataClassInfo == nullptr)
        {
#if defined(AZ_ENABLE_SERIALIZER_DEBUG)
            AZStd::string error;

            // output an error
            if (classElement && classElement->m_flags & SerializeContext::ClassElement::FLG_BASE_CLASS)
            {
                error = AZStd::string::format(
                    "Element with class ID '%s' was declared as a base class of another type but is not registered with the serializer.  "
                    "Either remove it from the Class<> call or reflect it.",
                    classIdPtr->ToString<AZStd::string>().c_str());
            }
            else
            {
                error = AZStd::string::format(
                    "Element with class ID '%s' is not registered with the serializer!", classIdPtr->ToString<AZStd::string>().c_str());
            }

            callContext->m_errorHandler->ReportError(error.c_str());

            callContext->m_errorHandler->Pop();
#endif // AZ_ENABLE_SERIALIZER_DEBUG

            return true; // we errored, but return true to continue enumeration of our siblings and other unrelated hierarchies
        }

        if (dataClassInfo->m_eventHandler)
        {
            if ((callContext->m_accessFlags & SerializeContext::ENUM_ACCESS_FOR_WRITE) == SerializeContext::ENUM_ACCESS_FOR_WRITE)
            {
                dataClassInfo->m_eventHandler->OnWriteBegin(objectPtr);
            }
            else
            {
                dataClassInfo->m_eventHandler->OnReadBegin(objectPtr);
            }
        }

        bool keepEnumeratingSiblings = true;

        // Call beginElemCB for this element if there is one. If the callback
        // returns false, stop enumeration of this branch
        // pass the original ptr to the user instead of objectPtr because
        // he may want to replace the actual object.
        if (!callContext->m_beginElemCB || callContext->m_beginElemCB(ptr, dataClassInfo, classElement))
        {
            if (dataClassInfo->m_container)
            {
                dataClassInfo->m_container->EnumElements(objectPtr, callContext->m_elementCallback);
            }
            else
            {
                if (dataClassInfo->m_editData)
                {
                    for (auto& element : dataClassInfo->m_editData->m_elements)
                    {
                        const SerializeContext::ClassElement* ed = element.m_serializeClassElement;
                        if (ed)
                        {
                            void* dataAddress = (char*)(objectPtr) + ed->m_offset;
                            if (dataAddress)
                            {
                                const SerializeContext::ClassData* elemClassInfo = ed->m_genericClassInfo
                                    ? ed->m_genericClassInfo->GetClassData()
                                    : m_serializeContext.FindClassData(ed->m_typeId, dataClassInfo, ed->m_nameCrc);

                                keepEnumeratingSiblings = EnumerateInstance(callContext, dataAddress, ed->m_typeId, elemClassInfo, ed);
                                if (!keepEnumeratingSiblings)
                                {
                                    break;
                                }
                            }
                        }
                        else
                        {
                            // If there's no ClassElement, create a synthetic one and provide it.
                            // This can happen in the case of entries like UIElements that aren't tied directly to
                            // a class member, but are still part of the UI.
                            if (callContext->m_beginElemCB)
                            {
                                SerializeContext::ClassElement syntheticClassElement;
                                syntheticClassElement.m_flags = SerializeContext::ClassElement::Flags::FLG_UI_ELEMENT;
                                syntheticClassElement.m_editData = &element;
                                callContext->m_beginElemCB(ptr, dataClassInfo, &syntheticClassElement);
                            }
                            if (callContext->m_endElemCB)
                            {
                                callContext->m_endElemCB();
                            }
                        }
                    }
                }
                else
                {
                    for (size_t i = 0, n = dataClassInfo->m_elements.size(); i < n; ++i)
                    {
                        const SerializeContext::ClassElement& ed = dataClassInfo->m_elements[i];
                        void* dataAddress = (char*)(objectPtr) + ed.m_offset;
                        if (dataAddress)
                        {
                            const SerializeContext::ClassData* elemClassInfo = ed.m_genericClassInfo
                                ? ed.m_genericClassInfo->GetClassData()
                                : m_serializeContext.FindClassData(ed.m_typeId, dataClassInfo, ed.m_nameCrc);

                            keepEnumeratingSiblings = EnumerateInstance(callContext, dataAddress, ed.m_typeId, elemClassInfo, &ed);
                            if (!keepEnumeratingSiblings)
                            {
                                break;
                            }
                        }
                    }
                }
            }
        }

        // call endElemCB
        if (callContext->m_endElemCB)
        {
            keepEnumeratingSiblings = callContext->m_endElemCB();
        }

        if (dataClassInfo->m_eventHandler)
        {
            if ((callContext->m_accessFlags & SerializeContext::ENUM_ACCESS_HOLD) == 0)
            {
                if ((callContext->m_accessFlags & SerializeContext::ENUM_ACCESS_FOR_WRITE) == SerializeContext::ENUM_ACCESS_FOR_WRITE)
                {
                    dataClassInfo->m_eventHandler->OnWriteEnd(objectPtr);
                }
                else
                {
                    dataClassInfo->m_eventHandler->OnReadEnd(objectPtr);
                }
            }
        }

#if defined(AZ_ENABLE_SERIALIZER_DEBUG)
        callContext->m_errorHandler->Pop();
#endif // AZ_ENABLE_SERIALIZER_DEBUG

        return keepEnumeratingSiblings;
    }

    namespace Edit
    {
        void GetComponentUuidsWithSystemComponentTag(
            const SerializeContext* serializeContext,
            const AZStd::vector<AZ::Crc32>& requiredTags,
            AZStd::unordered_set<AZ::Uuid>& componentUuids)
        {
            componentUuids.clear();

            serializeContext->EnumerateAll(
                [&requiredTags, &componentUuids](const AZ::SerializeContext::ClassData* data, const AZ::Uuid& typeId) -> bool
            {
                if (SystemComponentTagsMatchesAtLeastOneTag(data, requiredTags))
                {
                    componentUuids.emplace(typeId);
                }

                return true;
            });
        }

        //=========================================================================
        // ClearAttributes
        //=========================================================================
        void ElementData::ClearAttributes()
        {
            for (auto& attrib : m_attributes)
            {
                delete attrib.second;
            }
            m_attributes.clear();
        }

        //=========================================================================
        // FindAttribute
        //=========================================================================
        Edit::Attribute* ElementData::FindAttribute(AttributeId attributeId) const
        {
            for (const AttributePair& attributePair : m_attributes)
            {
                if (attributePair.first == attributeId)
                {
                    return attributePair.second;
                }
            }
            return nullptr;
        }

        //=========================================================================
        // ClearElements
        //=========================================================================
        void ClassData::ClearElements()
        {
            for (ElementData& element : m_elements)
            {
                if (element.m_serializeClassElement)
                {
                    element.m_serializeClassElement->m_editData = nullptr;
                    element.m_serializeClassElement = nullptr;
                }
                element.ClearAttributes();
            }
            m_elements.clear();
        }

        //=========================================================================
        // FindElementData
        //=========================================================================
        const ElementData* ClassData::FindElementData(AttributeId elementId) const
        {
            for (const ElementData& element : m_elements)
            {
                if (element.m_elementId == elementId)
                {
                    return &element;
                }
            }
            return nullptr;
        }
    } // namespace Edit
} // namespace AZ
