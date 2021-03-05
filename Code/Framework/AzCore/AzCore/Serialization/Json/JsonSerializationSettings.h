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

#include <AzCore/Serialization/Json/JsonSerializationMetadata.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace AZ
{
    class JsonRegistrationContext;
    class SerializeContext;

    //! Optional settings used while loading a json value to an object.
    struct JsonDeserializerSettings final
    {
        //! Metadata can be used to pass additional information to serializers. Look at individual serializers for available options.
        JsonSerializationMetadata m_metadata;

        //! Optional callback when issues are encountered. If not provided reporting will be forwarded to the default issue reporting.
        //! This can also be used to change the returned result code to alter the behavior of the deserializer.
        JsonSerializationResult::JsonIssueCallback m_reporting;
        //! Optional serialize context. If not provided the default instance will be retrieved through an EBus call.
        SerializeContext* m_serializeContext = nullptr;
        //! Optional json registration context. If not provided the default instance will be retrieved through an EBus call.
        JsonRegistrationContext* m_registrationContext = nullptr;

        //! If true this will clear all containers in the object before applying the data from the json document. If set to false
        //! any values in the container will be kept and not overwritten.
        //! Note that this does not apply to containers where elements have a fixed location such as smart pointers or AZStd::tuple.
        bool m_clearContainers = false;
    };

    //! Optional settings used while storing an object to a json value.
    struct JsonSerializerSettings final
    {
        //! Metadata can be used to pass additional information to serializers. Look at individual serializers for available options.
        JsonSerializationMetadata m_metadata;

        //! Optional callback when issues are encountered. If not provided reporting will be forwarded to the default issue reporting.
        //! This can also be used to change the returned result code to alter the behavior of the serializer.
        JsonSerializationResult::JsonIssueCallback m_reporting;
        //! Optional serialize context. If not provided the default instance will be retrieved through an EBus call.
        SerializeContext* m_serializeContext = nullptr;
        //! Optional json registration context. If not provided the default instance will be retrieved through an EBus call.
        JsonRegistrationContext* m_registrationContext = nullptr;

        //! If true default value will be stored, otherwise only changed values will be stored. This will automatically be set to false
        //! if the Store function is given a default object.
        bool m_keepDefaults = false;
    };

    //! Optional settings used while applying a patch to a json value
    struct JsonApplyPatchSettings final
    {
        //! Optional callback when issues are encountered. If not provided reporting will be forwarded to the default issue reporting.
        //! This can also be used to change the returned result code to alter the behavior of the patch application process.
        JsonSerializationResult::JsonIssueCallback m_reporting;
    };

    //! Optional settings used while creating a patch for a json value
    struct JsonCreatePatchSettings final
    {
        //! Optional callback when issues are encountered. If not provided reporting will be forwarded to the default issue reporting.
        //! This can also be used to change the returned result code to alter the behavior of the patch creation process.
        JsonSerializationResult::JsonIssueCallback m_reporting;
    };
} // namespace AZ
