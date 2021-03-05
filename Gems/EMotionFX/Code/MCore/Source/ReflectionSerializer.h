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

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Serialization/Utils.h>
#include <MCore/Source/CommandLine.h>

namespace MCore
{
    class ReflectionSerializer
    {
    public:
        template <typename TClass>
        static AZ::Outcome<AZStd::string> SerializeMember(const TClass* classPtr, const char* memberName)
        {
            return SerializeMember(azrtti_typeid(classPtr), classPtr, memberName);
        }

        // Returns a serialized unordered_map with all the memberName->memberValue pairs of an object except the passed
        // excluded members
        template <typename TClass>
        static AZ::Outcome<AZStd::string> SerializeMembersExcept(const TClass* classPtr, const AZStd::vector<AZStd::string>& excludeMembers)
        {
            return SerializeMembersExcept(azrtti_typeid(classPtr), classPtr, excludeMembers);
        }

        template <typename TClass>
        static bool DeserializeIntoMember(TClass* classPtr, const char* memberName, const AZStd::string& value)
        {
            return DeserializeIntoMember(azrtti_typeid(classPtr), classPtr, memberName, value);
        }

        template <typename TClass>
        static AZ::Outcome<AZStd::string> Serialize(const TClass* classPtr)
        {
            return Serialize(azrtti_typeid(classPtr), classPtr);
        }

        template <typename TClass>
        static bool Deserialize(TClass* classPtr, const AZStd::string& sourceBuffer)
        {
            return Deserialize(azrtti_typeid(classPtr), classPtr, sourceBuffer);
        }

        template <typename TClass>
        static TClass* Deserialize(const AZStd::string& sourceBuffer)
        {
            AZ::IO::ByteContainerStream<const AZStd::string> byteStream(&sourceBuffer);
            return AZ::Utils::LoadObjectFromStream<TClass>(byteStream);
        }

        template <typename TClass>
        static AZ::Outcome<AZStd::unordered_map<AZStd::string, AZStd::string>> SerializeIntoMap(const TClass* classPtr)
        {
            return SerializeIntoMap(azrtti_typeid(classPtr), classPtr);
        }

        template <typename TClass>
        static AZ::Outcome<AZStd::string> SerializeIntoCommandLine(const TClass* classPtr)
        {
            return SerializeIntoCommandLine(azrtti_typeid(classPtr), classPtr);
        }

        template <typename TClass>
        static AZ::Outcome<AZStd::string> SerializeValue(const TClass* classPtr)
        {
            return SerializeValue(azrtti_typeid(classPtr), classPtr);
        }
        template <typename TClass>
        static bool Deserialize(TClass* classPtr, const MCore::CommandLine& sourceCommandLine)
        {
            return Deserialize(azrtti_typeid(classPtr), classPtr, sourceCommandLine);
        }

        // Deserializes a serialized unordered_map of values into the classPtr object. memberValuesMap is the returned
        // string of the SerializeMembersExcept method
        template <typename TClass>
        static bool DeserializeMembers(TClass* classPtr, const AZStd::string& memberValuesMap)
        {
            return DeserializeMembers(azrtti_typeid(classPtr), classPtr, memberValuesMap);
        }

        template <typename TClass>
        static TClass* Clone(const TClass* classPtr)
        {
            return reinterpret_cast<TClass*>(Clone(azrtti_typeid(classPtr), classPtr));
        }


        template <typename TClass>
        static void CloneInplace(TClass& dest, const TClass* ptr)
        {
            CloneInplace(&dest, ptr, azrtti_typeid(dest));
        }

        template <typename TClass, typename TValue>
        static bool SetIntoMember(AZ::SerializeContext* context, TClass* classPtr, const char* memberName, const TValue& value)
        {
            void* ptrToMember = GetPointerToMember(context, azrtti_typeid(classPtr), classPtr, memberName);
            if (ptrToMember)
            {
                *reinterpret_cast<TValue*>(ptrToMember) = value;
                return true;
            }
            return false;
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        static AZ::Outcome<AZStd::string> SerializeMember(const AZ::TypeId& classTypeId, const void* classPtr, const char* memberName);

        static AZ::Outcome<AZStd::string> SerializeMembersExcept(const AZ::TypeId& classTypeId, const void* classPtr, const AZStd::vector<AZStd::string>& excludeMembers);

        static bool DeserializeIntoMember(const AZ::TypeId& classTypeId, void* classPtr, const char* memberName, const AZStd::string& value);

        static AZ::Outcome<AZStd::string> Serialize(const AZ::TypeId& classTypeId, const void* classPtr);

        static bool Deserialize(const AZ::TypeId& classTypeId, void* classPtr, const AZStd::string& sourceBuffer);

        static AZ::Outcome<AZStd::unordered_map<AZStd::string, AZStd::string>> SerializeIntoMap(const AZ::TypeId& classTypeId, const void* classPtr);

        static AZ::Outcome<AZStd::string> SerializeIntoCommandLine(const AZ::TypeId& classTypeId, const void* classPtr);

        static AZ::Outcome<AZStd::string> SerializeValue(const AZ::TypeId& classTypeId, const void* classPtr);

        static bool Deserialize(const AZ::TypeId& classTypeId, void* classPtr, const MCore::CommandLine& sourceCommandLine);

        static bool DeserializeMembers(const AZ::TypeId& classTypeId, void* classPtr, const AZStd::string& memberValuesMap);

        static void* Clone(const AZ::TypeId& classTypeId, const void* classPtr);
        static void CloneInplace(void* dest, const void* src, const AZ::Uuid& classId);

        static void* GetPointerToMember(AZ::SerializeContext* context, const AZ::TypeId& classTypeId, void* classPtr, const char* memberName);
    };
}
