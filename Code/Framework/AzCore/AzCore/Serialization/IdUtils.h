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
#pragma once

#include <AzCore/Math/Uuid.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/functional.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

namespace AZ
{
    class ReflectContext;
    class SerializeContext;
}

namespace AZ
{
    namespace IdUtils
    {
        template<typename IdType>
        struct Remapper
        {
            /**
            * Signature of function that gets passed to the IdMapper to generate a new Id of type IdType
            * The function is retrieved from a AZ::Edit::Attributes::IdGeneratorFunction attribute when processing EditData Data element.
            */
            using IdGenerator = AZStd::function<IdType()>;
            /**
            * Signature of function that is invoked by RemapIds to either generate a new id or to return an existing id
            * \param originalId - the original Id being remapped
            * \param replaceId - boolean that indicates the id supplied should have a new id generated for it
            * \param idGenerator -  function object which can be invoked to generate a new id.
            * The supplied idGenerator function can be empty, so an empty function check should be performed before invoking
            */
            using IdMapper = AZStd::function<IdType(const IdType& originalId, bool replaceId, const IdGenerator& idGenerator)>;
            /**
            * Signature of function that is invoked by RemapIds to return an existing id
            * \param originalId - the original Id being remapped
            */
            using IdReplacer = AZStd::function<IdType(const IdType& originalId)>;

            /**
            * Enumerates all elements in the object's hierarchy invoking the IdMapper function on any element that matches @IdType
            * If the IdMapper returns a value different than the originalId that element is updated with the new value
            * \param classPtr - the object instance to enumerate
            * \param classUuid - the object's type id
            * \param mapper - function object which is invoked to retrieve an id mapped from the originalId
            * \param context - The serialize context for enumerating the @classPtr elements
            * \param replaceId - boolean passed into @mapper function to that indicate that a new id should be generated for the original id.
            * otherwise an id from a map can be returned.
            */
            static unsigned int RemapIds(void* classPtr, const AZ::Uuid& classUuid, const IdMapper& mapper, AZ::SerializeContext* context, bool replaceId);

            /**
            * Template id remap function which which retrieves the type uuid from the supplied object and forwards to the non-template method.
            * Enumerates all elements in the object's hierarchy invoking the IdMapper function on any element that matches @IdType
            * If the IdMapper returns a value different than the originalId that element is updated with the new value
            * \param classPtr - the object instance to enumerate
            * \param mapper - function object which is invoked to retrieve an id mapped from the originalId
            * \param context - The serialize context for enumerating the @classPtr elements
            * \param replaceId - boolean passed into @mapper function to that indicate that a new id should be generated for the original id.
            * otherwise an id from a map can be returned.
            */
            template<typename T>
            static unsigned int RemapIds(T* classPtr, const IdMapper& mapper, AZ::SerializeContext* context, bool replaceId)
            {
                return RemapIds(classPtr, AZ::SerializeTypeInfo<T>::GetUuid(classPtr), mapper, context, replaceId);
            }

            /**
            * Convenience function for remapping ids with by supplying both true and false to the @RemapIds function.
            * True indicates that an id is the main instance id and should have a new value generated for it.
            * False indicates that an id is a reference to the main instance id and should retrieve a new value from a map of original Id -> new Id*.
            * First the replaceId parameter is true in RemapIds in order to allow population of an old Id -> new Id Map using the IdMapper functoin.
            * Second the replaceId parameter is false in RemapIds in order to allow id references to be fixed up using the map populated in the first call
            * \param classPtr - the object instance to enumerate
            * \param classUuid - the object's type id
            * \param mapper - function object which is invoked to retrieve an id mapped from the originalId
            * \param context - The serialize context for enumerating the @classPtr elements
            */
            static unsigned int ReplaceIdsAndIdRefs(void* classPtr, const AZ::Uuid& classUuid, const IdMapper& mapper, AZ::SerializeContext* context = nullptr);

            /**
            * Template version of ReplaceIdsAndIdRefs which retrieves the type uuid from the supplied object and forwards to the non-template method
            * Convenience function for remapping ids with by supplying both true and false to the @RemapIds function.
            * True indicates that an id is the main instance id and should have a new value generated for it.
            * False indicates that an id is a reference to the main instance id and should retrieve a new value from a map of original Id -> new Id*.
            * First the replaceId parameter is true in RemapIds in order to allow population of an old Id -> new Id Map using the IdMapper functoin.
            * Second the replaceId parameter is false in RemapIds in order to allow id references to be fixed up using the map populated in the first call
            * \param classPtr - the object instance to enumerate
            * \param mapper - function object which is invoked to retrieve an id mapped from the originalId
            * \param context - The serialize context for enumerating the @classPtr elements
            */
            template<typename T>
            static unsigned int ReplaceIdsAndIdRefs(T* classPtr, const IdMapper& mapper, AZ::SerializeContext* context = nullptr)
            {
                return ReplaceIdsAndIdRefs(classPtr, AZ::SerializeTypeInfo<T>::GetUuid(classPtr), mapper, context);
            }

            /**
            * Convenience function for remapping ids in a single pass. Requires that there will be no new id generation, only mapping
            * \param classPtr - the object instance to enumerate
            * \param classUuid - the object's type id
            * \param mapper - function object which is invoked to retrieve an id mapped from the originalId
            * \param context - The serialize context for enumerating the @classPtr elements
            */
            static unsigned int RemapIdsAndIdRefs(void* classPtr, const AZ::Uuid& classUuid, const IdReplacer& mapper, AZ::SerializeContext* context = nullptr);

            /**
            * Template version of RemapIdsAndIdRefs which retrieves the type uuid from the supplied object and forwards to the non-template method
            * Convenience function for remapping ids in a single pass. Requires that there will be no new id generation, only mapping
            * \param classPtr - the object instance to enumerate
            * \param mapper - function object which is invoked to retrieve an id mapped from the originalId
            * \param context - The serialize context for enumerating the @classPtr elements
            */
            template<typename T>
            static unsigned int RemapIdsAndIdRefs(T* classPtr, const IdReplacer& mapper, AZ::SerializeContext* context = nullptr)
            {
                return RemapIdsAndIdRefs(classPtr, AZ::SerializeTypeInfo<T>::GetUuid(classPtr), mapper, context);
            }

            /**
            * Enumerates object and generates new ids of IdType using the newIdMap as a data source for remapping ids as well as updating the newIdMap
            * with the remapped Ids
            * \param object - the object to enumerate
            * \param newIdMap - An input/output map container for storing mappings of old IdType values to new IdType values.
            * \param context - The serialize context for enumerating the @classPtr elements
            */
            template<typename T, typename MapType>
            static void GenerateNewIdsAndFixRefs(T* object, MapType& newIdMap, AZ::SerializeContext* context = nullptr)
            {
                if (!context)
                {
                    AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
                    if (!context)
                    {
                        AZ_Error("Serialization", false, "No serialize context provided! Failed to get component application default serialize context! ComponentApp is not started or input serialize context should not be null!");
                        return;
                    }
                }

                auto idMapper = [&newIdMap](const IdType& originalId, bool replaceId, const IdGenerator& idGenerator) -> IdType
                {
                    if (replaceId)
                    {
                        if (idGenerator)
                        {
                            auto it = newIdMap.emplace(originalId, idGenerator());
                            return it.first->second;
                        }
                        return originalId;
                    }
                    else
                    {
                        auto findIt = newIdMap.find(originalId);
                        if (findIt == newIdMap.end())
                        {
                            return originalId;
                        }
                        return findIt->second;
                    }
                };

                ReplaceIdsAndIdRefs(object, idMapper, context);
            }

            /**
            * Clones the supplied object, enumerate over the cloned object and generate new ids of IdType using the newIdMap as both a data source and data sink for remapping ids
            * \param object - the object to clone
            * \param newIdMap - An input/output map container for storing mappings of old IdType values to new IdType values.
            * \param context - The serialize context for enumerating the @classPtr elements
            * \return - cloned object with newly remapped ids of type IdType
            */
            template<class T, class MapType>
            static T* CloneObjectAndGenerateNewIdsAndFixRefs(const T* object, MapType& newIdMap, AZ::SerializeContext* context = nullptr)
            {
                if (!context)
                {
                    AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
                    if (!context)
                    {
                        AZ_Error("Serialization", false, "No serialize context provided! Failed to get component application default serialize context! ComponentApp is not started or input serialize context should not be null!");
                        return nullptr;
                    }
                }

                T* clonedObject = context->CloneObject(object);

                GenerateNewIdsAndFixRefs(clonedObject, newIdMap, context);

                return clonedObject;
            }
        };

        template<typename IdType>
        using IdMappingCB = typename Remapper<IdType>::IdMapper;
    } // namespace IdUtils
} // namespace AZ

#include <AzCore/Serialization/IdUtils.inl>
