/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Uuid.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    class JsonRegistrationContext
        : public ReflectContext
    {
    public:
        AZ_RTTI(JsonRegistrationContext, "{5A763774-CA8B-4245-A897-A03C503DCD60}", ReflectContext);
        AZ_CLASS_ALLOCATOR(JsonRegistrationContext, SystemAllocator);

        class SerializerBuilder;
        using SerializerMap = AZStd::unordered_map<Uuid, AZStd::unique_ptr<BaseJsonSerializer>, AZStd::hash<Uuid>>;
        using HandledTypesMap = AZStd::unordered_map<Uuid, BaseJsonSerializer*, AZStd::hash<Uuid>>;

        ~JsonRegistrationContext() override;

        const HandledTypesMap& GetRegisteredSerializers() const;
        BaseJsonSerializer* GetSerializerForType(const Uuid& typeId) const;
        BaseJsonSerializer* GetSerializerForSerializerType(const Uuid& typeId) const;

        template <typename T>
        SerializerBuilder Serializer()
        {
            const Uuid& typeId = azrtti_typeid<T>();
            if (!IsRemovingReflection())
            {
                AZ_Assert(m_jsonSerializers.find(typeId) == m_jsonSerializers.end(), "Duplicate Serializer registered with typeid %s", typeId.ToString<AZStd::string>().c_str());
                auto insertIter = m_jsonSerializers.emplace(typeId, aznew T);
                return SerializerBuilder(this, insertIter.first);
            }
            else
            {
                [[maybe_unused]] size_t erased = m_jsonSerializers.erase(typeId);
                AZ_Assert(erased == 1, "Attempting to unregister a serializer that has not been registered yet with typeid %s", typeId.ToString<AZStd::string>().c_str());
                return SerializerBuilder(this, m_jsonSerializers.end());
            }
        }

        class SerializerBuilder
        {
            friend class JsonRegistrationContext;
        public:
            SerializerBuilder* operator->();

            template <typename T>
            SerializerBuilder* HandlesType(bool overwriteExisting = false)
            {
                return HandlesTypeId(azrtti_typeid<T>(), overwriteExisting);
            }

            template <template<typename...> class T>
            SerializerBuilder* HandlesType(bool overwriteExisting = false)
            {
                return HandlesTypeId(azrtti_typeid<T>(), overwriteExisting);
            }

#if defined(AZ_COMPILER_MSVC)
            // There is a bug with the MSVC compiler when using the 'auto' keyword here. It appears that MSVC is unable to distinguish between a template
            // template argument with a type variadic pack vs a template template argument with a non-type auto variadic pack.
            template<template<AZStd::size_t...> class T>
#else
            template<template<auto...> class T>
#endif // defined(AZ_COMPILER_MSVC)
            SerializerBuilder* HandlesType(bool overwriteExisting = false)
            {
                return HandlesTypeId(azrtti_typeid<T>(), overwriteExisting);
            }

            template<template<typename, AZStd::size_t> class T>
            SerializerBuilder* HandlesType(bool overwriteExisting = false)
            {
                return HandlesTypeId(azrtti_typeid<T>(), overwriteExisting);
            }

            template<template<typename, typename, AZStd::size_t> class T>
            SerializerBuilder* HandlesType(bool overwriteExisting = false)
            {
                return HandlesTypeId(azrtti_typeid<T>(), overwriteExisting);
            }

            template<template<typename, typename, typename, AZStd::size_t> class T>
            SerializerBuilder* HandlesType(bool overwriteExisting = false)
            {
                return HandlesTypeId(azrtti_typeid<T>(), overwriteExisting);
            }

            template<template<typename, AZStd::size_t, typename> class T>
            SerializerBuilder* HandlesType(bool overwriteExisting = false)
            {
                return HandlesTypeId(azrtti_typeid<T>(), overwriteExisting);
            }

        protected:
            SerializerBuilder(JsonRegistrationContext* context, SerializerMap::const_iterator serializerMapIter);
            SerializerBuilder* HandlesTypeId(const AZ::Uuid& uuid, bool overwriteExisting);

            JsonRegistrationContext* m_context = nullptr;
            SerializerMap::const_iterator m_serializerIter;
        };

    protected:
        SerializerMap m_jsonSerializers;
        HandledTypesMap m_handledTypesMap;
    };
} // namespace AZ
