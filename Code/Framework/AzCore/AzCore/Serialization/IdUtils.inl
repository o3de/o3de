/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/std/functional.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/std/typetraits/remove_pointer.h>

namespace AZ
{
    namespace IdUtils
    {
        // Type IDs of types of special interest (we copy the string as it should never change)
        static const AZ::Uuid s_GenericClassPairID = AZ::Uuid::CreateString("{9F3F5302-3390-407a-A6F7-2E011E3BB686}"); // GenericClassPair
        struct StackDataType
        {
            const AZ::SerializeContext::ClassData* m_classData;
            const AZ::SerializeContext::ClassElement* m_elementData;
            void* m_dataPtr;
            bool m_isModifiedContainer;
        };

        template<typename IdType>
        unsigned int Remapper<IdType>::RemapIds(void* classPtr, const AZ::Uuid& classUuid, const typename Remapper<IdType>::IdMapper& mapper, AZ::SerializeContext* context, bool replaceId)
        {
            if (!context)
            {
                AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                if (!context)
                {
                    AZ_Error("Serialization", false, "No serialize context provided! Failed to get component application default serialize context! ComponentApp is not started or input serialize context should not be null!");
                    return 0;
                }
            }

            unsigned int replaced = 0;
            AZStd::vector<StackDataType> parentStack;
            parentStack.reserve(30);
            auto beginCB = [&](void* ptr, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* elementData) -> bool
            {
                if (classData->m_typeId == AZ::SerializeTypeInfo<IdType>::GetUuid())
                {
                    IdGenerator idGeneratorFunc;
                    if (elementData)
                    {
                        AZ::Attribute* attribute = FindAttribute(AZ::Edit::Attributes::IdGeneratorFunction, elementData->m_attributes);
                        if (attribute)
                        {
                            AZ_Assert(azrtti_istypeof<AttributeFunction<IdType()>>(attribute), "Attribute \"AZ::Edit::Attributes::IdGeneratorFunction\" must contain a non-member function with signature IdType()");
                            if (auto funcAttribute = azrtti_cast<AZ::AttributeFunction<IdType()>*>(attribute))
                            {
                                idGeneratorFunc = [funcAttribute](){ return funcAttribute->Invoke(nullptr); };
                            }
                        }
                    }

                    auto originalIdPtr = reinterpret_cast<IdType*>(ptr);
                    if (elementData->m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER)
                    {
                        originalIdPtr = *reinterpret_cast<IdType**>(ptr);
                    }

                    if (mapper)
                    {
                        // Only invoke mapper if replaceId bool matches idGeneratorFunc state
                        // if replaceId is true, the mapper function is expected to generate a new id, so IdGenerator needs to be valid
                        // if replaceId is false, the mapper function is expected to return an already mapped id, so idGenerator needs to be invalid
                        bool invokeMapper = replaceId == static_cast<bool>(idGeneratorFunc);
                        IdType newId;
                        if (invokeMapper)
                        {
                            newId = mapper(*originalIdPtr, replaceId, idGeneratorFunc);
                        }
                        else
                        {
                            newId = *originalIdPtr;
                        }

                        if (newId != *originalIdPtr)
                        {
                            if (parentStack.size() >= 1)
                            {
                                // check when the ID is in a container that will need to change as a result of the ID changing, keep in mind that is ONLY 
                                // covering basic cases and internal containers. We can't guess if ID is used in a complex function that affects the container
                                // or any other side effect. If this become the case, we can Reflect a function to handle when we remap ID (or any field) 
                                // reflect it during the serialization and react anytime we change the content on the ID, via the OnWriteBegin/OnWriteEnd events
                                StackDataType& parentInfo = parentStack.back();
                                GenericClassInfo* genericClassInfo = context->FindGenericClassInfo(parentInfo.m_classData->m_typeId);
                                if (genericClassInfo && genericClassInfo->GetGenericTypeId() == IdUtils::s_GenericClassPairID)
                                {
                                    if (parentStack.size() >= 2) // check if the pair is part of a container
                                    {
                                        StackDataType& pairContainerInfo = parentStack[parentStack.size() - 2];
                                        if (pairContainerInfo.m_classData->m_container)
                                        {
                                            // make sure the ID is the first element in the pair, as this is what associative containers care about, the rest is a payload
                                            // we can skip this check, but it helps avoiding extra work on a high level)
                                            if (parentInfo.m_elementData->m_genericClassInfo->GetTemplatedTypeId(0) == classData->m_typeId)
                                            {
                                                pairContainerInfo.m_isModifiedContainer = true; // we will modify the container
                                            }
                                        }
                                    }
                                }
                                else if (parentInfo.m_classData->m_container)
                                {
                                    parentInfo.m_isModifiedContainer = true; // we will modify this container
                                }
                            }

                            *originalIdPtr = newId;

                            replaced++;
                        }
                    }
                }
                parentStack.push_back({ classData, elementData, ptr, false });
                return true;
            };

            auto endCB = [&]() -> bool
            {
                if (parentStack.back().m_isModifiedContainer)
                {
                    parentStack.back().m_classData->m_container->ElementsUpdated(parentStack.back().m_dataPtr);
                }
                parentStack.pop_back();
                return true;
            };

            context->EnumerateInstance(
                classPtr,
                classUuid,
                beginCB,
                endCB,
                AZ::SerializeContext::ENUM_ACCESS_FOR_WRITE,
                nullptr,
                nullptr,
                nullptr
            );

            return replaced;
        }

        template<typename IdType>
        unsigned int Remapper<IdType>::ReplaceIdsAndIdRefs(void* classPtr, const AZ::Uuid& classUuid, const IdMapper& mapper, AZ::SerializeContext* context /*= nullptr*/)
        {
            unsigned int replaced = RemapIds(classPtr, classUuid, mapper, context, true);
            replaced += RemapIds(classPtr, classUuid, mapper, context, false);
            return replaced;
        }

        template<typename IdType>
        unsigned int Remapper<IdType>::RemapIdsAndIdRefs(void* classPtr, const AZ::Uuid& classUuid, const typename Remapper<IdType>::IdReplacer& mapper, AZ::SerializeContext* context)
        {
            if (!context)
            {
                AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                if (!context)
                {
                    AZ_Error("Serialization", false, "No serialize context provided! Failed to get component application default serialize context! ComponentApp is not started or input serialize context should not be null!");
                    return 0;
                }
            }

            unsigned int replaced = 0;
            AZStd::vector<StackDataType> parentStack;
            parentStack.reserve(30);
            auto beginCB = [&](void* ptr, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* elementData) -> bool
            {
                if (classData->m_typeId == AZ::SerializeTypeInfo<IdType>::GetUuid())
                {
                    auto originalIdPtr = reinterpret_cast<IdType*>(ptr);
                    if (elementData->m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER)
                    {
                        originalIdPtr = *reinterpret_cast<IdType**>(ptr);
                    }

                    if (mapper)
                    {
                        IdType newId;
                        newId = mapper(*originalIdPtr);

                        if (newId != *originalIdPtr)
                        {
                            if (parentStack.size() >= 1)
                            {
                                StackDataType& parentInfo = parentStack.back();
                                GenericClassInfo* genericClassInfo = context->FindGenericClassInfo(parentInfo.m_classData->m_typeId);
                                if (genericClassInfo && genericClassInfo->GetGenericTypeId() == IdUtils::s_GenericClassPairID)
                                {
                                    if (parentStack.size() >= 2) // check if the pair is part of a container
                                    {
                                        StackDataType& pairContainerInfo = parentStack[parentStack.size() - 2];
                                        if (pairContainerInfo.m_classData->m_container)
                                        {
                                            // make sure the ID is the first element in the pair, as this is what associative containers care about, the rest is a payload
                                            // we can skip this check, but it helps avoiding extra work on a high level)
                                            if (parentInfo.m_elementData->m_genericClassInfo->GetTemplatedTypeId(0) == classData->m_typeId)
                                            {
                                                pairContainerInfo.m_isModifiedContainer = true; // we will modify the container
                                            }
                                        }
                                    }
                                }
                                else if (parentInfo.m_classData->m_container)
                                {
                                    parentInfo.m_isModifiedContainer = true; // we will modify this container
                                }
                            }

                            *originalIdPtr = newId;
                            replaced++;
                        }
                    }
                }

                parentStack.push_back({ classData, elementData, ptr, false });
                return true;
            };

            auto endCB = [&]() -> bool
            {
                if (parentStack.back().m_isModifiedContainer)
                {
                    parentStack.back().m_classData->m_container->ElementsUpdated(parentStack.back().m_dataPtr);
                }
                parentStack.pop_back();
                return true;
            };

            context->EnumerateInstance(
                classPtr,
                classUuid,
                beginCB,
                endCB,
                AZ::SerializeContext::ENUM_ACCESS_FOR_WRITE,
                nullptr,
                nullptr,
                nullptr
            );

            return replaced;
        }
    } // namespace IdUtils
} // namespace AZ
