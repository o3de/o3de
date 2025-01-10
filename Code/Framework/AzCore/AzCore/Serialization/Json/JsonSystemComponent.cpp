/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathReflection.h>
#include <AzCore/Serialization/Json/AnySerializer.h>
#include <AzCore/Serialization/Json/ArraySerializer.h>
#include <AzCore/Serialization/Json/BasicContainerSerializer.h>
#include <AzCore/Serialization/Json/BoolSerializer.h>
#include <AzCore/Serialization/Json/ByteStreamSerializer.h>
#include <AzCore/Serialization/Json/DoubleSerializer.h>
#include <AzCore/Serialization/Json/IntSerializer.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/MapSerializer.h>
#include <AzCore/Serialization/Json/PointerJsonSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/SmartPointerSerializer.h>
#include <AzCore/Serialization/Json/StringSerializer.h>
#include <AzCore/Serialization/Json/TupleSerializer.h>
#include <AzCore/Serialization/Json/UnorderedSetSerializer.h>
#include <AzCore/Serialization/Json/UnsupportedTypesSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EnumConstantJsonSerializer.h>
#include <AzCore/Serialization/PointerObject.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/ConfigurableStack.h>
#include <AzCore/std/any.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/tuple.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/forward_list.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>


namespace AZ
{
    void JsonSystemComponent::Activate()
    {}

    void JsonSystemComponent::Deactivate()
    {}

    void JsonSystemComponent::Reflect(ReflectContext* reflectContext)
    {
        JsonConfigurableStackSerializer::Reflect(reflectContext);

        if (JsonRegistrationContext* jsonContext = azrtti_cast<JsonRegistrationContext*>(reflectContext))
        {
            jsonContext->Serializer<JsonBoolSerializer>()->HandlesType<bool>();
            jsonContext->Serializer<JsonDoubleSerializer>()->HandlesType<double>();
            jsonContext->Serializer<JsonFloatSerializer>()->HandlesType<float>();
            jsonContext->Serializer<JsonCharSerializer>()->HandlesType<char>();
            jsonContext->Serializer<JsonShortSerializer>()->HandlesType<short>();
            jsonContext->Serializer<JsonIntSerializer>()->HandlesType<int>();
            jsonContext->Serializer<JsonLongSerializer>()->HandlesType<long>();
            jsonContext->Serializer<JsonLongLongSerializer>()->HandlesType<long long>();
            jsonContext->Serializer<JsonUnsignedCharSerializer>()->HandlesType<unsigned char>();
            jsonContext->Serializer<JsonUnsignedShortSerializer>()->HandlesType<unsigned short>();
            jsonContext->Serializer<JsonUnsignedIntSerializer>()->HandlesType<unsigned int>();
            jsonContext->Serializer<JsonUnsignedLongSerializer>()->HandlesType<unsigned long>();
            jsonContext->Serializer<JsonUnsignedLongLongSerializer>()->HandlesType<unsigned long long>();

            jsonContext->Serializer<JsonStringSerializer>()->HandlesType<AZStd::string>();
            jsonContext->Serializer<JsonOSStringSerializer>()->HandlesType<OSString>();

            jsonContext->Serializer<JsonByteStreamSerializer<AZStd::allocator>>()->HandlesType<JsonByteStream<AZStd::allocator>>();

            jsonContext->Serializer<JsonBasicContainerSerializer>()
                ->HandlesType<AZStd::fixed_vector>()
                ->HandlesType<AZStd::forward_list>()
                ->HandlesType<AZStd::list>()
                ->HandlesType<AZStd::set>()
                ->HandlesType<AZStd::vector>();
            jsonContext->Serializer<JsonMapSerializer>()
                ->HandlesType<AZStd::map>();
            jsonContext->Serializer<JsonUnorderedMapSerializer>()
                ->HandlesType<AZStd::unordered_map>();
            jsonContext->Serializer<JsonUnorderedMultiMapSerializer>()
                ->HandlesType<AZStd::unordered_multimap>();
            jsonContext->Serializer<JsonUnorderedSetContainerSerializer>()
                ->HandlesType<AZStd::unordered_set>()
                ->HandlesType<AZStd::unordered_multiset>();
            jsonContext->Serializer<JsonTupleSerializer>()
                ->HandlesType<AZStd::pair>()
                ->HandlesType<AZStd::tuple>();

            jsonContext->Serializer<JsonSmartPointerSerializer>()
                ->HandlesType<AZStd::unique_ptr>()
                ->HandlesType<AZStd::shared_ptr>()
                ->HandlesType<AZStd::intrusive_ptr>();

            jsonContext->Serializer<JsonArraySerializer>()
                ->HandlesType<AZStd::array>();

            jsonContext->Serializer<JsonAnySerializer>()
                ->HandlesType<AZStd::any>();
            jsonContext->Serializer<JsonVariantSerializer>()
                ->HandlesType<AZStd::variant>();
            jsonContext->Serializer<JsonOptionalSerializer>()
                ->HandlesType<AZStd::optional>();
            jsonContext->Serializer<JsonBitsetSerializer>()
                ->HandlesType<AZStd::bitset>();
            jsonContext->Serializer<EnumConstantJsonSerializer>()
                ->HandlesType<AZ::Edit::EnumConstant>();

            jsonContext->Serializer<PointerJsonSerializer>()
                ->HandlesType<PointerObject>();

            MathReflect(jsonContext);
        }
        else if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflectContext))
        {
            serializeContext->Class<JsonSystemComponent, AZ::Component>();
        }
    }
    
} // namespace AZ
