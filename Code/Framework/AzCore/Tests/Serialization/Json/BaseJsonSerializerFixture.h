/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/JSON/document.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace JsonSerializationTests
{
    class BaseJsonSerializerFixture
        : public UnitTest::LeakDetectionFixture
    {
    public:
        using ComponentContainer = AZStd::vector<AZ::ComponentDescriptor*>;

        void SetUp() override;
        void TearDown() override;

        //! Add one or more system component descriptors to the list of system components that are
        //! are reflected to the Serialize Context and Json Registration Context. By default the
        //! JsonSystemComponent will be added.
        virtual void AddSystemComponentDescriptors(ComponentContainer& systemComponents);
        //! Used to register additional reflected classes unique for tests.
        virtual void RegisterAdditional(AZStd::unique_ptr<AZ::SerializeContext>& serializeContext);
        //! Used to register additional serializers unique for tests.
        virtual void RegisterAdditional(AZStd::unique_ptr<AZ::JsonRegistrationContext>& serializeContext);

        virtual rapidjson::Value CreateExplicitDefault();
        virtual void Expect_ExplicitDefault(rapidjson::Value& value);

        virtual void Expect_DocStrEq(AZStd::string_view testString);
        virtual void Expect_DocStrEq(AZStd::string_view testString, bool stripWhitespace);
        virtual void Expect_DocStrEq(rapidjson::Value& lhs, rapidjson::Value& rhs);

        virtual void ResetJsonContexts();

        virtual rapidjson::Value TypeToInjectionValue(rapidjson::Type type);
        //! Injects a new value before every field/value in an Object/Array. This is recursively called.
        //! @param value The start value to inject into.
        //! @param typeToInject The type of the RapidJSON that will be injected before the original value.
        //! @param allocator The RapidJSON memory allocator associated with the documented that owns the value.
        virtual void InjectAdditionalFields(rapidjson::Value& value, rapidjson::Type typeToInject,
            rapidjson::Document::AllocatorType& allocator);
        //! Replaces a value with the indicated type. This is recursively called for arrays and objects.
        //! @param value The start value to corrupt.
        //! @param typeToInject The type of the RapidJSON that will be inject in the place of the original value.
        virtual void CorruptFields(rapidjson::Value& value, rapidjson::Type typeToInject);

        AZStd::vector<AZ::ComponentDescriptor*> m_systemComponents;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonRegistrationContext;
        AZStd::unique_ptr<rapidjson::Document> m_jsonDocument;

        AZStd::unique_ptr<AZ::JsonSerializerSettings> m_serializationSettings;
        AZStd::unique_ptr<AZ::JsonDeserializerSettings> m_deserializationSettings;
        AZStd::unique_ptr<AZ::JsonSerializerContext> m_jsonSerializationContext;
        AZStd::unique_ptr<AZ::JsonDeserializerContext> m_jsonDeserializationContext;
    };
} // namespace JsonSerializationTests

#include <Tests/Serialization/Json/BaseJsonSerializerFixture.cpp>
