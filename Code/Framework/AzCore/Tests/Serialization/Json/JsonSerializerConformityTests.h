/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <iostream> // Used for pretty printing.
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>

// It is difficult to isolate individual conformity tests or types with gtest_filter.
// Use the macros below to disable all tests except the one you need to work on.
#define JSON_CONFORMITY_ENABLED

#if defined JSON_CONFORMITY_ENABLED
#define IF_JSON_CONFORMITY_ENABLED(X) X
#else
#define IF_JSON_CONFORMITY_ENABLED(X) 
#endif


namespace JsonSerializationTests
{
    static constexpr char DefaultPath[] = "Test";
    using ResultCode = AZ::JsonSerializationResult::ResultCode;

    class JsonSerializerConformityTestDescriptorFeatures
    {
    public:
        inline void EnableJsonType(rapidjson::Type type)
        {
            AZ_Assert(type < AZ_ARRAY_SIZE(m_typesSupported), "RapidJSON Type not known.");
            m_typesSupported[type] = true;;
        }
        inline bool SupportsJsonType(rapidjson::Type type)
        {
            AZ_Assert(type < AZ_ARRAY_SIZE(m_typesSupported), "RapidJSON Type not known.");
            return m_typesSupported[type];
        }

        //! A list of fields the serializer always needs.
        //! It's recommended to avoid mandatory fields, but in some cases this might be unavoidable. In such cases
        //! the mandatory fields names can be added so test for empty defaults are adjusted to expect the required fields.
        AZStd::vector<AZStd::string> m_mandatoryFields;

        //! This type will be used for additional files or to corrupt a json file. Use a type
        //! that the serializer under test doesn't support.
        rapidjson::Type m_typeToInject = rapidjson::kStringType;
        //! Set to true if the rapidjson::kArrayType is expected to be a fixed size.
        bool m_fixedSizeArray{ false };
        //! Set to false if the type the serializer targets can't be partially initialized.
        bool m_supportsPartialInitialization{ true };
        //! Set to false if partial defaults are supported, but not reported as a PartialDefaults. This can be true in container/pointer cases.
        bool m_partialDefaultReportingIsStrict{ true };
        //! The serializer will look up the type id in the Serialize Context.
        //! Normally serializer only get the exact type they support, but some serializers support
        //! multiple types such as templated code. By settings this option to true tests will be added
        //! give an unreflected type to make sure an invalid case is handled.
        bool m_requiresTypeIdLookups{ false };
        //! Several tests inject unsupported types into a json files or corrupt the file with unsupported
        //! values. If the serializer under test supports all json types then GetInjectedJson/GetCorruptedJson
        //! can be used to manually create these documents. If that is also not an option the tests can be
        //! disabled by setting this flag to false.
        bool m_supportsInjection{ true };
        //! Enables the check that tries to determine if variables are initialized and if not whether they have the
        //! OperationFlags::ManualDefault set. This applies for instance to integers, which won't be initialized if
        //! constructed a new instance is created for pointers.
        bool m_enableInitializationTest{ true };
        //! Enable the test that creates a new instance of the provided test type through the factory that's found in
        //! the Serialize Context. This test is automatically disabled for classes that don't have a factory or
        //! have a null factory as well as for classes that have mandatory fields.
        bool m_enableNewInstanceTests{ true };
        //! Disable this test if the empty json is not equal to the default constructed object
        bool m_defaultIsEqualToEmpty{ true };

    private:
        // There's no way to retrieve the number of types from RapidJSON so they're hard-coded here.
        bool m_typesSupported[7] = { false, false, false, false, false, false, false };
    };

    namespace Internal
    {
        inline AZ::JsonSerializationResult::ResultCode VerifyCallback(AZStd::string_view message,
            AZ::JsonSerializationResult::ResultCode result, AZStd::string_view path)
        {
            EXPECT_LT(0, message.length());
            bool isValidPath =
                path.starts_with(DefaultPath) || // Format during serialization.
                (path.starts_with("/") && path.substr(1).starts_with(DefaultPath));  // Format during deserialization.
            EXPECT_TRUE(isValidPath);
            return result;
        }
    }

    // The word "default" has three meanings in the json serialization conformity tests. Each is reflected in an emum.
    // 1) Is JSON serializing keeping or dropping default values from serialization? (DefaultJSONStorage)
    // 2) How much default values remain in the serialized C++ object? (DefaultSerializedObjectStatus)
    // 3) Did JSON serialization supply a default object to the store/load call? (DefaultObjectSuppliedInSerialization)
    enum DefaultJSONStorage
    {
        Store,
        Drop
    };

    enum DefaultSerializedObjectStatus
    {
        Full,
        Partial,
        None
    };

    enum DefaultObjectSuppliedInSerialization
    {
        True,
        False
    };

    struct JSONRequestSpec
    {
        DefaultJSONStorage jsonStorage;
        DefaultSerializedObjectStatus objectStatus;
        DefaultObjectSuppliedInSerialization objectSupplied;
    };

    template<typename T>
    class JsonSerializerConformityTestDescriptor
    {
    public:
        using Type = T;

        virtual ~JsonSerializerConformityTestDescriptor() = default;

        virtual AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() = 0;

        //! Create an instance of the target type with all values set to default.
        virtual AZStd::shared_ptr<T> CreateDefaultInstance() = 0;
        //! Create an instance of the target type that constructed with default constructor.
        //! This will be the same instance that Json Serialization creates for dynamic types. Typically it's the same
        //! as from CreateDefaultInstance(), except of types, such as pointers, that need to do minimal (de)serialization
        //! to initialize an object.
        virtual AZStd::shared_ptr<T> CreateDefaultConstructedInstance() { return CreateDefaultInstance(); }
        //! Create an instance of the target type with some values set and some kept on defaults.
        //! If the target type doesn't support partial specialization this can be ignored and
        //! tests for partial support will be skipped.
        virtual AZStd::shared_ptr<T> CreatePartialDefaultInstance() { return nullptr; }
        //! Create an instance where all values are set to non-default values.
        virtual AZStd::shared_ptr<T> CreateFullySetInstance() = 0;
        //! Create an instance of the target array type with a single value that has all defaults.
        //! If the target type doesn't support arrays or requires more than one entry this can be ignored and
        //! tests using this value will be skipped.
        virtual AZStd::shared_ptr<T> CreateSingleArrayDefaultInstance() { return nullptr; }
        //! Use this function evaluating which JSON to return based on the full spec of the test
        virtual AZStd::optional<AZStd::string_view> GetJSON([[maybe_unused]] const JSONRequestSpec& requestSpec)
        {
            return AZStd::nullopt;
        }
        //! Get the json that represents the default instance.
        //! If the target type doesn't support partial specialization this can be ignored and
        //! tests for partial support will be skipped.
        virtual AZStd::string_view GetJsonForPartialDefaultInstance() { return ""; }
        //! Get the json that represents the instance with all values set.
        virtual AZStd::string_view GetJsonForFullySetInstance() = 0;
        //! Get the json that represents an array with a single value that has only defaults.
        //! If the target type doesn't support arrays or requires more than one entry this can be ignored and
        //! tests using this value will be skipped.
        virtual AZStd::string_view GetJsonForSingleArrayDefaultInstance() { return "[{}]"; }
        //! Get the json that represents an empty array that has only defaults.
        //! If the target type doesn't support arrays, this can be ignored and
        //! tests using this value will be skipped.
        virtual AZStd::string_view GetJsonForEmptyArrayDefaultInstance() { return "[]"; }
        //! Get the json where additional values are added to the json file.
        //! If this function is not overloaded, but features.m_supportsInjection is enabled then
        //! the Json Serializer Conformity Tests will inject extra values in the json for a fully.
        //! set instance. It's recommended to use the automatic injection.
        virtual AZStd::string_view GetInjectedJson() { return ""; };
        //! Get the json where one or more values are replaced with invalid values.
        //! If this function is not overloaded, but features.m_supportsInjection is enabled then
        //! the Json Serializer Conformity Tests will corrupt the json for a fully set instance.
        //! It's recommended to use the automatic corruption.
        virtual AZStd::string_view GetCorruptedJson() { return "";  };
        
        virtual void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& /*features*/) = 0;
        virtual void SetUp() {}
        virtual void TearDown() {}
        virtual void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& /*context*/) {}
        virtual void Reflect(AZStd::unique_ptr<AZ::JsonRegistrationContext>& /*context*/) {}
        virtual void AddSystemComponentDescriptors([[maybe_unused]] BaseJsonSerializerFixture::ComponentContainer& systemComponents) {}

        virtual bool AreEqual(const T& lhs, const T& rhs) = 0;

        // The json for specific tests can be overloaded if needed. It's highly recommended to use the
        // default strings for consistency. This should only be needed for exceptional situations such
        // as testing particular pointer behaviors.

        virtual AZStd::string_view GetJsonFor_Load_DeserializeUnreflectedType() { return this->GetJsonForFullySetInstance(); }
        virtual AZStd::string_view GetJsonFor_Load_DeserializeFullySetInstance() { return this->GetJsonForFullySetInstance(); }
        virtual AZStd::string_view GetJsonFor_Load_DeserializePartialInstance() { return this->GetJsonForPartialDefaultInstance(); }
        virtual AZStd::string_view GetJsonFor_Load_DeserializeArrayWithDefaultValue() { return this->GetJsonForSingleArrayDefaultInstance(); }
        virtual AZStd::string_view GetJsonFor_Load_DeserializeFullInstanceOnTopOfPartialDefaulted() { return this->GetJsonForFullySetInstance(); }
        virtual AZStd::string_view GetJsonFor_Load_HaltedThroughCallback() { return this->GetJsonForFullySetInstance(); }
        virtual AZStd::string_view GetJsonFor_Store_SerializeWithDefaultsKept() { return this->GetJsonForFullySetInstance(); }
        virtual AZStd::string_view GetJsonFor_Store_SerializeFullySetInstance() { return this->GetJsonForFullySetInstance(); }
        virtual AZStd::string_view GetJsonFor_Store_SerializeWithoutDefault() { return this->GetJsonForFullySetInstance(); }
        virtual AZStd::string_view GetJsonFor_Store_SerializeWithoutDefaultAndDefaultsKept() { return this->GetJsonForFullySetInstance(); }
        virtual AZStd::string_view GetJsonFor_Store_SerializePartialInstance() { return this->GetJsonForPartialDefaultInstance(); }
        virtual AZStd::string_view GetJsonFor_Store_SerializeArrayWithSingleDefaultValue() { return this->GetJsonForSingleArrayDefaultInstance(); }
    };

    template<typename T>
    class JsonSerializerConformityTests
        : public BaseJsonSerializerFixture
    {
    public:
        using Description = T;
        using Type = typename T::Type;


        struct PointerWrapper
        {
            AZ_TYPE_INFO(PointerWrapper, "{32FA6645-074A-458A-B79C-B173D0BD4B42}");
            AZ_CLASS_ALLOCATOR(PointerWrapper, AZ::SystemAllocator);

            Type* m_value{ nullptr };

            ~PointerWrapper()
            {
                // Using free because not all types can safely use delete. Since this just to clear the memory to satisfy the memory
                // leak test, this is fine.
                azfree(m_value);
            }
        };

        void SetUp() override
        {
            using namespace AZ::JsonSerializationResult;

            JsonSerializerConformityTestDescriptor<Type>* descriptor = &(this->m_description);

            BaseJsonSerializerFixture::SetUp();
            descriptor->SetUp();
            descriptor->ConfigureFeatures(this->m_features);
            descriptor->Reflect(this->m_serializeContext);
            descriptor->Reflect(this->m_jsonRegistrationContext);
            this->m_serializeContext->template Class<PointerWrapper>()->Field("Value", &PointerWrapper::m_value);

            this->m_deserializationSettings->m_reporting = &Internal::VerifyCallback;
            this->m_serializationSettings->m_reporting = &Internal::VerifyCallback;
            this->ResetJsonContexts();
            this->m_jsonDeserializationContext->PushPath(DefaultPath);
            this->m_jsonSerializationContext->PushPath(DefaultPath);
        }

        void TearDown() override
        {
            this->m_features.m_mandatoryFields.clear();
            this->m_features.m_mandatoryFields.shrink_to_fit();

            JsonSerializerConformityTestDescriptor<Type>* descriptor = &(this->m_description);

            this->m_jsonRegistrationContext->EnableRemoveReflection();
            descriptor->Reflect(this->m_jsonRegistrationContext);
            this->m_jsonRegistrationContext->DisableRemoveReflection();

            this->m_serializeContext->EnableRemoveReflection();
            this->m_serializeContext->template Class<PointerWrapper>()->Field("Value", &PointerWrapper::m_value);
            descriptor->Reflect(this->m_serializeContext);
            this->m_serializeContext->DisableRemoveReflection();

            descriptor->TearDown();
            BaseJsonSerializerFixture::TearDown();
        }

        void AddSystemComponentDescriptors(ComponentContainer& systemComponents) override
        {
            BaseJsonSerializerFixture::AddSystemComponentDescriptors(systemComponents);
            this->m_description.AddSystemComponentDescriptors(systemComponents);
        }

    protected:
        void Load_InvalidType_ReturnsUnsupportedOrInvalid(rapidjson::Type type)
        {
            using namespace AZ::JsonSerializationResult;

            if (!this->m_features.SupportsJsonType(type))
            {
                rapidjson::Document testValue(type);
                if (type == rapidjson::kObjectType)
                {
                    // For objects add a random field so the object isn't mistaken for
                    // a default if ContinueLoad is called in the serializer.
                    testValue.AddMember(rapidjson::StringRef("A"), 42, testValue.GetAllocator());
                }

                auto serializer = this->m_description.CreateSerializer();
                auto original = this->m_description.CreateDefaultInstance();
                auto instance = this->m_description.CreateDefaultInstance();

                ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*original), 
                    testValue, *this->m_jsonDeserializationContext);

                if (this->m_features.m_mandatoryFields.empty())
                {
                    EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
                    EXPECT_EQ(Processing::Altered, result.GetProcessing());
                }
                else
                {
                    EXPECT_THAT(result.GetOutcome(), ::testing::AnyOf(Outcomes::Unsupported, Outcomes::Invalid));
                    EXPECT_THAT(result.GetProcessing(), ::testing::AnyOf(Processing::Altered, Processing::Halted));
                }
                EXPECT_TRUE(this->m_description.AreEqual(*original, *instance));
            }
        }

        T m_description;
        JsonSerializerConformityTestDescriptorFeatures m_features;
    };

    class IncorrectClass
    {
    public:
        AZ_RTTI(IncorrectClass, "{E201252B-D653-4753-93AD-4F13C5FA2246}");
    };

    TYPED_TEST_CASE_P(JsonSerializerConformityTests);

    TYPED_TEST_P(JsonSerializerConformityTests, Registration_SerializerIsRegisteredWithContext_SerializerFound)
    {
        auto serializer = this->m_description.CreateSerializer();
        AZ::Uuid serializerType = azrtti_typeid(*serializer);
        const AZ::JsonRegistrationContext::HandledTypesMap& serializers = this->m_jsonRegistrationContext->GetRegisteredSerializers();

        bool serializerRegistered = false;
        for (const auto& entry : serializers)
        {
            if (azrtti_typeid(entry.second) == serializerType)
            {
                serializerRegistered = true;
                break;
            }
        }

        EXPECT_TRUE(serializerRegistered);
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_InvalidTypeOfNullType_ReturnsUnsupported)
    { 
        this->Load_InvalidType_ReturnsUnsupportedOrInvalid(rapidjson::kNullType);
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_InvalidTypeOfFalseType_ReturnsUnsupported)
    { 
        this->Load_InvalidType_ReturnsUnsupportedOrInvalid(rapidjson::kFalseType);
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_InvalidTypeOfTrueType_ReturnsUnsupported)
    {
        this->Load_InvalidType_ReturnsUnsupportedOrInvalid(rapidjson::kTrueType);
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_InvalidTypeOfObjectType_ReturnsUnsupported)
    {
        this->Load_InvalidType_ReturnsUnsupportedOrInvalid(rapidjson::kObjectType);
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_InvalidTypeOfArrayType_ReturnsUnsupported)
    {
        this->Load_InvalidType_ReturnsUnsupportedOrInvalid(rapidjson::kArrayType);
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_InvalidTypeOfStringType_ReturnsUnsupported)
    {
        this->Load_InvalidType_ReturnsUnsupportedOrInvalid(rapidjson::kStringType);
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_InvalidTypeOfNumberType_ReturnsUnsupported) 
    {
        this->Load_InvalidType_ReturnsUnsupportedOrInvalid(rapidjson::kNumberType);
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_DeserializeUnreflectedType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_requiresTypeIdLookups)
        {
            this->m_jsonDocument->Parse(this->m_description.GetJsonFor_Load_DeserializeUnreflectedType().data());
            ASSERT_FALSE(this->m_jsonDocument->HasParseError());

            auto serializer = this->m_description.CreateSerializer();
            IncorrectClass instance;
            
            ResultCode result = serializer->Load(&instance, azrtti_typeid<IncorrectClass>(),
                *this->m_jsonDocument, *this->m_jsonDeserializationContext);

            EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
            EXPECT_EQ(Processing::Altered, result.GetProcessing());
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_DeserializeEmptyObject_SucceedsAndObjectMatchesDefaults)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.SupportsJsonType(rapidjson::kObjectType))
        {
            this->m_jsonDocument->Parse("{}");
            ASSERT_FALSE(this->m_jsonDocument->HasParseError());

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateDefaultConstructedInstance();
            auto original = this->m_description.CreateDefaultInstance();

            ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*instance),
                *this->m_jsonDocument, *this->m_jsonDeserializationContext);

            if (this->m_features.m_mandatoryFields.empty())
            {
                EXPECT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
                EXPECT_EQ(Processing::Completed, result.GetProcessing());
            }
            else
            {
                EXPECT_EQ(Outcomes::Missing, result.GetOutcome());
                bool validProcessing =
                    result.GetProcessing() == Processing::Altered ||
                    result.GetProcessing() == Processing::Halted;
                EXPECT_TRUE(validProcessing);
            }
            EXPECT_TRUE(this->m_description.AreEqual(*original, *instance));
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_DeserializeEmptyObjectThroughMainLoad_SucceedsAndObjectMatchesDefaults)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.SupportsJsonType(rapidjson::kObjectType))
        {
            this->m_jsonDocument->Parse("{}");
            ASSERT_FALSE(this->m_jsonDocument->HasParseError());

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateDefaultConstructedInstance();
            auto original = this->m_description.CreateDefaultInstance();

            AZ::JsonDeserializerSettings settings;
            settings.m_serializeContext = this->m_jsonDeserializationContext->GetSerializeContext();
            settings.m_registrationContext = this->m_jsonDeserializationContext->GetRegistrationContext();
            ResultCode result = AZ::JsonSerialization::Load(
                instance.get(), azrtti_typeid(*instance), *this->m_jsonDocument, settings);
            
            if (this->m_features.m_mandatoryFields.empty())
            {
                EXPECT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
                EXPECT_EQ(Processing::Completed, result.GetProcessing());
            }
            else
            {
                EXPECT_EQ(Outcomes::Missing, result.GetOutcome());
                bool validProcessing =
                    result.GetProcessing() == Processing::Altered ||
                    result.GetProcessing() == Processing::Halted;
                EXPECT_TRUE(validProcessing);
            }
            EXPECT_TRUE(this->m_description.AreEqual(*original, *instance));
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_DeserializeEmptyArray_SucceedsAndObjectMatchesDefaults)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.SupportsJsonType(rapidjson::kArrayType))
        {
            this->m_jsonDocument->Parse("[]");
            ASSERT_FALSE(this->m_jsonDocument->HasParseError());

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateDefaultConstructedInstance();
            auto original = this->m_description.CreateDefaultInstance();

            this->m_deserializationSettings->m_clearContainers = false;
            this->ResetJsonContexts();
            this->m_jsonDeserializationContext->PushPath(DefaultPath);

            ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*instance),
                *this->m_jsonDocument, *this->m_jsonDeserializationContext);

            if (this->m_features.m_fixedSizeArray)
            {
                EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
                EXPECT_EQ(Processing::Altered, result.GetProcessing());
            }
            else
            {
                EXPECT_EQ(Outcomes::Success, result.GetOutcome());
                EXPECT_EQ(Processing::Completed, result.GetProcessing());
            }
            EXPECT_EQ(Tasks::ReadField, result.GetTask());
            EXPECT_TRUE(this->m_description.AreEqual(*original, *instance));
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_DeserializeEmptyArrayWithClearEnabled_SucceedsAndObjectMatchesDefaults)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.SupportsJsonType(rapidjson::kArrayType))
        {
            this->m_jsonDocument->Parse("[]");
            ASSERT_FALSE(this->m_jsonDocument->HasParseError());

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateDefaultConstructedInstance();
            auto original = this->m_description.CreateDefaultInstance();

            this->m_deserializationSettings->m_clearContainers = true;
            this->ResetJsonContexts();
            this->m_jsonDeserializationContext->PushPath(DefaultPath);

            ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*instance),
                *this->m_jsonDocument, *this->m_jsonDeserializationContext);

            if (this->m_features.m_fixedSizeArray)
            {
                EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
                EXPECT_EQ(Processing::Altered, result.GetProcessing());
            }
            else
            {
                EXPECT_EQ(Outcomes::Success, result.GetOutcome());
                EXPECT_EQ(Processing::Completed, result.GetProcessing());
            }
            EXPECT_EQ(Tasks::ReadField, result.GetTask());
            EXPECT_TRUE(this->m_description.AreEqual(*original, *instance));
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_DeserializeEmptyArrayWithClearedTarget_SucceedsAndObjectMatchesDefaults)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.SupportsJsonType(rapidjson::kArrayType))
        {
            this->m_jsonDocument->Parse("[]");
            ASSERT_FALSE(this->m_jsonDocument->HasParseError());

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateFullySetInstance();
            auto original = this->m_description.CreateDefaultInstance();

            this->m_deserializationSettings->m_clearContainers = true;
            this->ResetJsonContexts();
            this->m_jsonDeserializationContext->PushPath(DefaultPath);

            ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*instance),
                *this->m_jsonDocument, *this->m_jsonDeserializationContext);

            if (this->m_features.m_fixedSizeArray)
            {
                EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
                EXPECT_EQ(Processing::Altered, result.GetProcessing());
                EXPECT_EQ(Tasks::ReadField, result.GetTask());
                EXPECT_FALSE(this->m_description.AreEqual(*original, *instance));
            }
            else
            {
                EXPECT_EQ(Outcomes::Success, result.GetOutcome());
                EXPECT_EQ(Processing::Completed, result.GetProcessing());
                EXPECT_EQ(Tasks::Clear, result.GetTask());
                EXPECT_TRUE(this->m_description.AreEqual(*original, *instance));
            }
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_DeserializeArrayWithDefaultValue_SucceedsAndReportPartialDefaults)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.SupportsJsonType(rapidjson::kArrayType))
        {
            this->m_jsonDocument->Parse(this->m_description.GetJsonFor_Load_DeserializeArrayWithDefaultValue().data());
            ASSERT_FALSE(this->m_jsonDocument->HasParseError());

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateDefaultInstance();
            
            this->m_jsonDeserializationContext->PushPath(DefaultPath);

            ResultCode result =
                serializer->Load(instance.get(), azrtti_typeid(*instance), *this->m_jsonDocument, *this->m_jsonDeserializationContext);

            if (this->m_features.m_fixedSizeArray)
            {
                EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
                EXPECT_EQ(Processing::Altered, result.GetProcessing());
            }
            else
            {
                EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
                EXPECT_EQ(Processing::Completed, result.GetProcessing());

                auto compare = this->m_description.CreateSingleArrayDefaultInstance();
                ASSERT_NE(nullptr, compare)
                    << "Conformity tests for variably sized arrays require an implementation of CreateSingleArrayDefaultInstance";
                EXPECT_TRUE(this->m_description.AreEqual(*compare, *instance));
            }
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_InterruptClearingTarget_ContainerIsNotCleared)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.SupportsJsonType(rapidjson::kArrayType) && !this->m_features.m_fixedSizeArray)
        {
            this->m_jsonDocument->Parse("[]");
            ASSERT_FALSE(this->m_jsonDocument->HasParseError());

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateFullySetInstance();
            auto original = this->m_description.CreateFullySetInstance();

            this->m_deserializationSettings->m_clearContainers = true;
            this->m_deserializationSettings->m_reporting = [](AZStd::string_view, ResultCode, AZStd::string_view) -> ResultCode
            {
                // Random task and outcome just to trigger the alternative path to clearing and to make sure the correct
                // result is returned.
                return ResultCode(Tasks::RetrieveInfo, Outcomes::Unsupported);
            };
            this->ResetJsonContexts();
            this->m_jsonDeserializationContext->PushPath(DefaultPath);

            ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*instance),
                *this->m_jsonDocument, *this->m_jsonDeserializationContext);

            EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
            EXPECT_EQ(Processing::Altered, result.GetProcessing());
            EXPECT_EQ(Tasks::RetrieveInfo, result.GetTask());
            EXPECT_TRUE(this->m_description.AreEqual(*original, *instance));
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_DeserializeFullySetInstance_SucceedsAndObjectMatchesFullySetInstance)
    {
        using namespace AZ::JsonSerializationResult;

        AZStd::string_view json = this->m_description.GetJsonFor_Load_DeserializeFullySetInstance();
        this->m_jsonDocument->Parse(json.data());
        ASSERT_FALSE(this->m_jsonDocument->HasParseError());

        auto serializer = this->m_description.CreateSerializer();
        auto instance = this->m_description.CreateDefaultConstructedInstance();
        auto compare = this->m_description.CreateFullySetInstance();
        
        ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*instance),
            *this->m_jsonDocument, *this->m_jsonDeserializationContext);

        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_TRUE(this->m_description.AreEqual(*instance, *compare));
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_DeserializeFullySetInstanceThroughMainLoad_SucceedsAndObjectMatchesFullySetInstance)
    {
        using namespace AZ::JsonSerializationResult;

        AZStd::string_view json = this->m_description.GetJsonFor_Load_DeserializeFullySetInstance();
        this->m_jsonDocument->Parse(json.data());
        ASSERT_FALSE(this->m_jsonDocument->HasParseError());

        auto instance = this->m_description.CreateDefaultConstructedInstance();
        auto compare = this->m_description.CreateFullySetInstance();

        AZ::JsonDeserializerSettings settings;
        settings.m_serializeContext = this->m_jsonDeserializationContext->GetSerializeContext();
        settings.m_registrationContext = this->m_jsonDeserializationContext->GetRegistrationContext();
        ResultCode result = AZ::JsonSerialization::Load(instance.get(), azrtti_typeid(*instance), *this->m_jsonDocument, settings);

        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_TRUE(this->m_description.AreEqual(*instance, *compare));
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_DeserializeWithMissingMandatoryField_LoadFailedAndMissingReported)
    {
        using namespace AZ::JsonSerializationResult;

        if (!this->m_features.m_mandatoryFields.empty())
        {
            AZStd::string_view json = this->m_description.GetJsonFor_Load_DeserializeFullySetInstance();
            auto serializer = this->m_description.CreateSerializer();
            auto compare = this->m_description.CreateDefaultInstance();

            for (const AZStd::string& mandatoryField : this->m_features.m_mandatoryFields)
            {
                this->m_jsonDocument->Parse(json.data());
                ASSERT_FALSE(this->m_jsonDocument->HasParseError());
                ASSERT_TRUE(this->m_jsonDocument->IsObject());
                auto memberToErase = this->m_jsonDocument->FindMember(mandatoryField.c_str());
                ASSERT_NE(this->m_jsonDocument->MemberEnd(), memberToErase);
                this->m_jsonDocument->RemoveMember(memberToErase);

                auto instance = this->m_description.CreateDefaultConstructedInstance();
                
                ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*instance),
                    *this->m_jsonDocument, *this->m_jsonDeserializationContext);

                EXPECT_EQ(Outcomes::Missing, result.GetOutcome());
                bool validProcessing =
                    result.GetProcessing() == Processing::Altered ||
                    result.GetProcessing() == Processing::Halted;
                EXPECT_TRUE(validProcessing);
                EXPECT_TRUE(this->m_description.AreEqual(*instance, *compare));
            }
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_DeserializePartialInstance_SucceedsAndObjectMatchesParialInstance)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_supportsPartialInitialization)
        {
            AZStd::string_view json = this->m_description.GetJsonFor_Load_DeserializePartialInstance();
            // If tests for partial initialization are enabled than json for the partial initialization is needed.
            ASSERT_FALSE(json.empty());
            this->m_jsonDocument->Parse(json.data());
            ASSERT_FALSE(this->m_jsonDocument->HasParseError());

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateDefaultConstructedInstance();
            auto compare = this->m_description.CreatePartialDefaultInstance();
            ASSERT_NE(nullptr, compare);

            ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*instance),
                *this->m_jsonDocument, *this->m_jsonDeserializationContext);

            if (this->m_features.m_partialDefaultReportingIsStrict)
            {
                EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
            }

            EXPECT_EQ(Processing::Completed, result.GetProcessing());
            EXPECT_TRUE(this->m_description.AreEqual(*instance, *compare));
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_DeserializeFullInstanceOnTopOfPartialDefaulted_SucceedsAndObjectMatchesParialInstance)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_supportsPartialInitialization)
        {
            AZStd::string_view json = this->m_description.GetJsonFor_Load_DeserializeFullInstanceOnTopOfPartialDefaulted();
            // If tests for partial initialization are enabled than json for the partial initialization is needed.
            ASSERT_FALSE(json.empty());
            this->m_jsonDocument->Parse(json.data());
            ASSERT_FALSE(this->m_jsonDocument->HasParseError());

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreatePartialDefaultInstance();
            auto compare = this->m_description.CreateFullySetInstance();
            ASSERT_NE(nullptr, compare);

            // Clear containers which should effectively turn them into default containers.
            this->m_deserializationSettings->m_clearContainers = true;
            this->ResetJsonContexts();
            this->m_jsonDeserializationContext->PushPath(DefaultPath);

            ResultCode result =
                serializer->Load(instance.get(), azrtti_typeid(*instance), *this->m_jsonDocument, *this->m_jsonDeserializationContext);

            EXPECT_EQ(Outcomes::Success, result.GetOutcome());
            EXPECT_EQ(Processing::Completed, result.GetProcessing());
            EXPECT_TRUE(this->m_description.AreEqual(*instance, *compare));
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_DefaultToPointer_SucceedsAndValueIsInitialized)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_enableNewInstanceTests
        && this->m_features.m_mandatoryFields.empty()
        && this->m_features.m_defaultIsEqualToEmpty)
        {
            AZ::SerializeContext* serializeContext = this->m_jsonDeserializationContext->GetSerializeContext();
            const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(azrtti_typeid<typename TypeParam::Type>());
            ASSERT_NE(nullptr, classData);
            // Skip this test if the target type doesn't have a factory to create a new instance with or if the factory explicitly
            // prohibits construction.
            if (classData->m_factory && classData->m_factory != AZ::Internal::NullFactory::GetInstance())
            {
                typename JsonSerializerConformityTests<TypeParam>::PointerWrapper instance;
                auto compare = this->m_description.CreateDefaultInstance();

                this->m_jsonDocument->Parse(R"({ "Value": {}})");
                ASSERT_FALSE(this->m_jsonDocument->HasParseError());

                AZ::JsonDeserializerSettings settings;
                settings.m_serializeContext = serializeContext;
                settings.m_registrationContext = this->m_jsonDeserializationContext->GetRegistrationContext();
                ResultCode result = AZ::JsonSerialization::Load(instance, *this->m_jsonDocument, settings);

                EXPECT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
                EXPECT_EQ(Processing::Completed, result.GetProcessing());
                ASSERT_NE(nullptr, instance.m_value);
                ASSERT_NE(nullptr, compare);
                EXPECT_TRUE(this->m_description.AreEqual(*instance.m_value, *compare));
            }
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_InitializeNewInstance_SucceedsAndValueIsInitialized)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_enableNewInstanceTests && this->m_features.m_mandatoryFields.empty())
        {
            auto serializer = this->m_description.CreateSerializer();
            if ((serializer->GetOperationsFlags() & BaseJsonSerializer::OperationFlags::InitializeNewInstance) ==
                BaseJsonSerializer::OperationFlags::InitializeNewInstance)
            {
                auto instance = this->m_description.CreateDefaultConstructedInstance();
                auto compare = this->m_description.CreateDefaultInstance();
                this->m_jsonDocument->SetObject();

                ResultCode result =
                    serializer->Load(instance.get(), azrtti_typeid(instance.get()), *this->m_jsonDocument, *this->m_jsonDeserializationContext);

                EXPECT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
                EXPECT_EQ(Processing::Completed, result.GetProcessing());
                ASSERT_NE(nullptr, instance);
                ASSERT_NE(nullptr, compare);
                EXPECT_TRUE(this->m_description.AreEqual(*instance, *compare));
            }
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_HaltedThroughCallback_LoadFailsAndHaltReported)
    {
        using namespace AZ::JsonSerializationResult;

        this->m_jsonDocument->Parse(this->m_description.GetJsonFor_Load_HaltedThroughCallback().data());
        ASSERT_FALSE(this->m_jsonDocument->HasParseError());

        auto serializer = this->m_description.CreateSerializer();
        auto instance = this->m_description.CreateDefaultConstructedInstance();
        
        AZ::ScopedContextReporter reporter(*this->m_jsonDeserializationContext,
            [](AZStd::string_view message, ResultCode result, AZStd::string_view path) -> ResultCode
            {
                Internal::VerifyCallback(message, result, path); 
                return ResultCode(result.GetTask(), Outcomes::Catastrophic);
            });

        ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*instance),
            *this->m_jsonDocument, *this->m_jsonDeserializationContext);

        EXPECT_EQ(Outcomes::Catastrophic, result.GetOutcome());
        EXPECT_EQ(Processing::Halted, result.GetProcessing());
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_InsertAdditionalData_SucceedsAndObjectMatchesFullySetInstance)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_supportsInjection)
        {
            AZStd::string_view injectedJson = this->m_description.GetInjectedJson();
            if (injectedJson.empty())
            {
                this->m_jsonDocument->Parse(this->m_description.GetJsonForFullySetInstance().data());
                ASSERT_FALSE(this->m_jsonDocument->HasParseError());
                this->InjectAdditionalFields(*(this->m_jsonDocument), this->m_features.m_typeToInject,
                    this->m_jsonDocument->GetAllocator());
            }
            else
            {
                this->m_jsonDocument->Parse(injectedJson.data());
                ASSERT_FALSE(this->m_jsonDocument->HasParseError());
            }

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateDefaultConstructedInstance();
            auto compare = this->m_description.CreateFullySetInstance();

            ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*instance),
                *this->m_jsonDocument, *this->m_jsonDeserializationContext);

            EXPECT_GE(result.GetOutcome(), Outcomes::Skipped);
            EXPECT_TRUE(this->m_description.AreEqual(*instance, *compare));
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Load_CorruptedValue_SucceedsWithDefaultValuesUsed)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_supportsInjection)
        {
            AZStd::string_view corruptedJson = this->m_description.GetCorruptedJson();
            if (corruptedJson.empty())
            {
                this->m_jsonDocument->Parse(this->m_description.GetJsonForFullySetInstance().data());
                ASSERT_FALSE(this->m_jsonDocument->HasParseError());
                this->CorruptFields(*(this->m_jsonDocument), this->m_features.m_typeToInject);
            }
            else
            {
                this->m_jsonDocument->Parse(corruptedJson.data());
                ASSERT_FALSE(this->m_jsonDocument->HasParseError());
            }

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateDefaultConstructedInstance();
            
            ResultCode result = serializer->Load(instance.get(), azrtti_typeid(*instance),
                *this->m_jsonDocument, *this->m_jsonDeserializationContext);

            EXPECT_GE(result.GetOutcome(), Outcomes::Unavailable);
            EXPECT_GE(result.GetProcessing(), Processing::PartialAlter);
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Store_SerializeUnreflectedType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_requiresTypeIdLookups)
        {
            auto serializer = this->m_description.CreateSerializer();
            IncorrectClass instance;

            ResultCode result = serializer->Store(*this->m_jsonDocument, &instance, nullptr, azrtti_typeid<IncorrectClass>(),
                *this->m_jsonSerializationContext);
                
            EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
            EXPECT_EQ(Processing::Altered, result.GetProcessing());
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Store_SerializeDefaultInstance_EmptyJsonReturned)
    {
        using namespace AZ::JsonSerializationResult;

        if (!this->m_features.m_defaultIsEqualToEmpty)
        {
            return;
        }

        this->m_serializationSettings->m_keepDefaults = false;
        this->ResetJsonContexts();
        this->m_jsonSerializationContext->PushPath(DefaultPath);
        
        auto serializer = this->m_description.CreateSerializer();
        auto instance = this->m_description.CreateDefaultInstance();
        rapidjson::Value convertedValue = this->CreateExplicitDefault();

        ResultCode result = serializer->Store(convertedValue, instance.get(), instance.get(), azrtti_typeid(*instance),
            *this->m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        if (convertedValue.IsObject() && !this->m_features.m_mandatoryFields.empty())
        {
            ASSERT_EQ(convertedValue.MemberCount(), this->m_features.m_mandatoryFields.size());
            for (const AZStd::string& mandatoryField : this->m_features.m_mandatoryFields)
            {
                EXPECT_NE(convertedValue.MemberEnd(), convertedValue.FindMember(mandatoryField.c_str()));
            }
        }
        else
        {
            EXPECT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
            this->Expect_ExplicitDefault(convertedValue);
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Store_SerializeDefaultInstanceThroughMainStore_EmptyJsonReturned)
    {
        using namespace AZ::JsonSerializationResult;

        if (!this->m_features.m_defaultIsEqualToEmpty)
        {
            return;
        }

        auto serializer = this->m_description.CreateSerializer();
        auto instance = this->m_description.CreateDefaultInstance();
        rapidjson::Value convertedValue = this->CreateExplicitDefault();

        AZ::JsonSerializerSettings settings;
        settings.m_serializeContext = this->m_jsonDeserializationContext->GetSerializeContext();
        settings.m_registrationContext = this->m_jsonDeserializationContext->GetRegistrationContext();
        ResultCode result = AZ::JsonSerialization::Store(
            convertedValue, this->m_jsonDocument->GetAllocator(), instance.get(), instance.get(), azrtti_typeid(*instance), settings);
        
        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        if (convertedValue.IsObject() && !this->m_features.m_mandatoryFields.empty())
        {
            ASSERT_EQ(convertedValue.MemberCount(), this->m_features.m_mandatoryFields.size());
            for (const AZStd::string& mandatoryField : this->m_features.m_mandatoryFields)
            {
                EXPECT_NE(convertedValue.MemberEnd(), convertedValue.FindMember(mandatoryField.c_str()));
            }
        }
        else
        {
            EXPECT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
            this->Expect_ExplicitDefault(convertedValue);
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Store_SerializeWithDefaultsKept_FullyWrittenJson)
    {
        using namespace AZ::JsonSerializationResult;

        this->m_serializationSettings->m_keepDefaults = true;
        this->ResetJsonContexts();
        this->m_jsonSerializationContext->PushPath(DefaultPath);

        auto serializer = this->m_description.CreateSerializer();
        auto instance = this->m_description.CreateFullySetInstance();
        auto defaultInstance = this->m_description.CreateDefaultInstance();
        ResultCode result = serializer->Store(*this->m_jsonDocument, instance.get(), instance.get(), azrtti_typeid(*instance),
            *this->m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());

        this->Expect_DocStrEq(this->m_description.GetJsonFor_Store_SerializeWithDefaultsKept());
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Store_SerializeFullySetInstance_StoredSuccessfullyAndJsonMatches)
    {
        using namespace AZ::JsonSerializationResult;

        this->m_serializationSettings->m_keepDefaults = false;
        this->ResetJsonContexts();
        this->m_jsonSerializationContext->PushPath(DefaultPath);

        auto serializer = this->m_description.CreateSerializer();
        auto instance = this->m_description.CreateFullySetInstance();
        auto defaultInstance = this->m_description.CreateDefaultInstance();
        
        ResultCode result = serializer->Store(*this->m_jsonDocument, instance.get(), defaultInstance.get(), azrtti_typeid(*instance),
            *this->m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());

        JSONRequestSpec spec;
        spec.jsonStorage = this->m_jsonSerializationContext->ShouldKeepDefaults() ? DefaultJSONStorage::Store : DefaultJSONStorage::Drop;
        spec.objectStatus = DefaultSerializedObjectStatus::None;
        spec.objectSupplied = DefaultObjectSuppliedInSerialization::True;

        if (auto json = this->m_description.GetJSON(spec))
        {
            this->Expect_DocStrEq(*json);

        }
        else
        {
            this->Expect_DocStrEq(this->m_description.GetJsonFor_Store_SerializeFullySetInstance());
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Store_SerializeWithoutDefault_StoredSuccessfullyAndJsonMatches)
    {
        using namespace AZ::JsonSerializationResult;

        this->m_serializationSettings->m_keepDefaults = false;
        this->ResetJsonContexts();
        this->m_jsonSerializationContext->PushPath(DefaultPath);

        auto serializer = this->m_description.CreateSerializer();
        auto instance = this->m_description.CreateFullySetInstance();
        
        ResultCode result = serializer->Store(*this->m_jsonDocument, instance.get(), nullptr, azrtti_typeid(*instance),
            *this->m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());

        JSONRequestSpec spec;
        spec.jsonStorage = this->m_jsonSerializationContext->ShouldKeepDefaults() ? DefaultJSONStorage::Store : DefaultJSONStorage::Drop;
        spec.objectStatus = DefaultSerializedObjectStatus::None;
        spec.objectSupplied = DefaultObjectSuppliedInSerialization::False;

        if (auto json = this->m_description.GetJSON(spec))
        {
            this->Expect_DocStrEq(*json);
        }
        else
        {
            this->Expect_DocStrEq(this->m_description.GetJsonFor_Store_SerializeWithoutDefault());
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Store_SerializeWithoutDefaultAndDefaultsKept_StoredSuccessfullyAndJsonMatches)
    {
        using namespace AZ::JsonSerializationResult;

        this->m_serializationSettings->m_keepDefaults = true;
        this->ResetJsonContexts();
        this->m_jsonSerializationContext->PushPath(DefaultPath);

        auto serializer = this->m_description.CreateSerializer();
        auto instance = this->m_description.CreateFullySetInstance();

        ResultCode result = serializer->Store(*this->m_jsonDocument, instance.get(), nullptr, azrtti_typeid(*instance),
            *this->m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        this->Expect_DocStrEq(this->m_description.GetJsonFor_Store_SerializeWithoutDefaultAndDefaultsKept());
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Store_SerializePartialInstance_StoredSuccessfullyAndJsonMatches)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_supportsPartialInitialization)
        {
            this->m_serializationSettings->m_keepDefaults = false;
            this->ResetJsonContexts();
            this->m_jsonSerializationContext->PushPath(DefaultPath);

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreatePartialDefaultInstance();
            ASSERT_NE(nullptr, instance);
            auto defaultInstance = this->m_description.CreateDefaultInstance();
            
            ResultCode result = serializer->Store(*this->m_jsonDocument, instance.get(), defaultInstance.get(), azrtti_typeid(*instance),
                *this->m_jsonSerializationContext);

            EXPECT_EQ(Processing::Completed, result.GetProcessing());
            EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
            this->Expect_DocStrEq(this->m_description.GetJsonFor_Store_SerializePartialInstance());
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Store_SerializeEmptyArray_StoredSuccessfullyAndJsonMatches)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.SupportsJsonType(rapidjson::kArrayType) && !this->m_features.m_fixedSizeArray)
        {
            this->m_serializationSettings->m_keepDefaults = true;
            this->ResetJsonContexts();
            this->m_jsonSerializationContext->PushPath(DefaultPath);

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateDefaultInstance();
            
            ResultCode result = serializer->Store(*this->m_jsonDocument, instance.get(), instance.get(), azrtti_typeid(*instance),
                *this->m_jsonSerializationContext);

            EXPECT_EQ(Processing::Completed, result.GetProcessing());
            EXPECT_EQ(Outcomes::Success, result.GetOutcome());
            this->Expect_DocStrEq(this->m_description.GetJsonForEmptyArrayDefaultInstance());
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Store_SerializeArrayWithSingleDefaultValue_StoredSuccessfullyAndJsonMatches)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.SupportsJsonType(rapidjson::kArrayType) && !this->m_features.m_fixedSizeArray)
        {
            this->m_jsonSerializationContext->PushPath(DefaultPath);

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateSingleArrayDefaultInstance();
            ASSERT_NE(nullptr, instance)
                << "Conformity tests for variably sized arrays require an implementation of CreateSingleArrayDefaultInstance";

            ResultCode result = serializer->Store(
                *this->m_jsonDocument, instance.get(), instance.get(), azrtti_typeid(*instance), *this->m_jsonSerializationContext);

            EXPECT_EQ(Processing::Completed, result.GetProcessing());
            EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
            this->Expect_DocStrEq(this->m_description.GetJsonFor_Store_SerializeArrayWithSingleDefaultValue());
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, Store_HaltedThroughCallback_StoreFailsAndHaltReported)
    {
        using namespace AZ::JsonSerializationResult;

        auto serializer = this->m_description.CreateSerializer();
        auto instance = this->m_description.CreateFullySetInstance();
        auto defaultInstance = this->m_description.CreateDefaultInstance();

        AZ::ScopedContextReporter reporter(*this->m_jsonSerializationContext,
            [](AZStd::string_view message, ResultCode result, AZStd::string_view) -> ResultCode
            {
                EXPECT_FALSE(message.empty());
                return ResultCode(result.GetTask(), Outcomes::Catastrophic);
            });

        ResultCode result = serializer->Store(*this->m_jsonDocument, instance.get(), defaultInstance.get(), azrtti_typeid(*instance),
            *this->m_jsonSerializationContext);

        EXPECT_EQ(Outcomes::Catastrophic, result.GetOutcome());
        EXPECT_EQ(Processing::Halted, result.GetProcessing());
    }

    TYPED_TEST_P(JsonSerializerConformityTests, StoreLoad_RoundTripWithPartialDefault_IdenticalInstances)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_supportsPartialInitialization)
        {
            this->m_serializationSettings->m_keepDefaults = false;
            this->ResetJsonContexts();
            this->m_jsonSerializationContext->PushPath(DefaultPath);
            this->m_jsonDeserializationContext->PushPath(DefaultPath);

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreatePartialDefaultInstance();
            auto defaultInstance = this->m_description.CreateDefaultInstance();

            rapidjson::Value convertedValue = this->CreateExplicitDefault();
            ResultCode result = serializer->Store(convertedValue, instance.get(), defaultInstance.get(), azrtti_typeid(*instance),
                *this->m_jsonSerializationContext);
            ASSERT_EQ(Processing::Completed, result.GetProcessing());

            auto loadedInstance = this->m_description.CreateDefaultInstance();
            result = serializer->Load(loadedInstance.get(), azrtti_typeid(*loadedInstance),
                convertedValue, *this->m_jsonDeserializationContext);
            ASSERT_EQ(Processing::Completed, result.GetProcessing());

            EXPECT_TRUE(this->m_description.AreEqual(*instance, *loadedInstance));
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, StoreLoad_RoundTripWithFullSet_IdenticalInstances)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_supportsPartialInitialization)
        {
            this->m_serializationSettings->m_keepDefaults = false;
            this->ResetJsonContexts();
            this->m_jsonSerializationContext->PushPath(DefaultPath);
            this->m_jsonDeserializationContext->PushPath(DefaultPath);

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateFullySetInstance();
            auto defaultInstance = this->m_description.CreateDefaultInstance();

            rapidjson::Value convertedValue = this->CreateExplicitDefault();
            ResultCode result = serializer->Store(convertedValue, instance.get(), defaultInstance.get(), azrtti_typeid(*instance),
                *this->m_jsonSerializationContext);
            ASSERT_EQ(Processing::Completed, result.GetProcessing());

            auto loadedInstance = this->m_description.CreateDefaultInstance();
            result = serializer->Load(loadedInstance.get(), azrtti_typeid(*loadedInstance),
                convertedValue, *this->m_jsonDeserializationContext);
            ASSERT_EQ(Processing::Completed, result.GetProcessing());

            EXPECT_TRUE(this->m_description.AreEqual(*instance, *loadedInstance));
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, StoreLoad_RoundTripWithDefaultsKept_IdenticalInstances)
    {
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_supportsPartialInitialization)
        {
            this->m_serializationSettings->m_keepDefaults = true;
            this->ResetJsonContexts();
            this->m_jsonSerializationContext->PushPath(DefaultPath);
            this->m_jsonDeserializationContext->PushPath(DefaultPath);

            auto serializer = this->m_description.CreateSerializer();
            auto instance = this->m_description.CreateFullySetInstance();
            
            rapidjson::Value convertedValue = this->CreateExplicitDefault();
            ResultCode result = serializer->Store(convertedValue, instance.get(), instance.get(), azrtti_typeid(*instance),
                *this->m_jsonSerializationContext);
            ASSERT_EQ(Processing::Completed, result.GetProcessing());

            auto loadedInstance = this->m_description.CreateDefaultInstance();
            result = serializer->Load(loadedInstance.get(), azrtti_typeid(*loadedInstance),
                convertedValue, *this->m_jsonDeserializationContext);
            ASSERT_EQ(Processing::Completed, result.GetProcessing());

            EXPECT_TRUE(this->m_description.AreEqual(*instance, *loadedInstance));
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, GetOperationFlags_RequiresExplicitInit_ObjectsThatDoNotConstructHaveExplicitInitOption)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        if (this->m_features.m_enableInitializationTest)
        {
            auto instance = this->m_description.CreateDefaultInstance();
            AZ_PUSH_DISABLE_WARNING(4701, "-Wuninitialized-const-reference")
            auto compare = this->m_description.CreateDefaultConstructedInstance();
            ASSERT_NE(compare, nullptr);
            ASSERT_NE(instance, nullptr);
            if (!this->m_description.AreEqual(*instance, *compare))
            AZ_POP_DISABLE_WARNING
            {
                auto serializer = this->m_description.CreateSerializer();
                BaseJsonSerializer::OperationFlags flags = serializer->GetOperationsFlags();
                bool hasManualDefaultSet =
                    (flags & BaseJsonSerializer::OperationFlags::ManualDefault) == BaseJsonSerializer::OperationFlags::ManualDefault ||
                    (flags & BaseJsonSerializer::OperationFlags::InitializeNewInstance) ==
                        BaseJsonSerializer::OperationFlags::InitializeNewInstance;   
                EXPECT_TRUE(hasManualDefaultSet);
            }
        }
    }

    TYPED_TEST_P(JsonSerializerConformityTests, GetOperationFlags_ManualDefaultSetIfNeeded_ManualDefaultOperationSetIfMandatoryFieldsAreDeclared)
    {
        if (this->m_features.SupportsJsonType(rapidjson::kObjectType))
        {
            if (!this->m_features.m_mandatoryFields.empty())
            {
                auto serializer = this->m_description.CreateSerializer();
                bool manuallyHandlesDefaults = (serializer->GetOperationsFlags() & AZ::BaseJsonSerializer::OperationFlags::ManualDefault) ==
                    AZ::BaseJsonSerializer::OperationFlags::ManualDefault;
                EXPECT_TRUE(manuallyHandlesDefaults);
            }
        }
    }

    REGISTER_TYPED_TEST_CASE_P(JsonSerializerConformityTests,
        Registration_SerializerIsRegisteredWithContext_SerializerFound,

        Load_InvalidTypeOfNullType_ReturnsUnsupported,
        Load_InvalidTypeOfFalseType_ReturnsUnsupported,
        Load_InvalidTypeOfTrueType_ReturnsUnsupported,
        Load_InvalidTypeOfObjectType_ReturnsUnsupported,
        Load_InvalidTypeOfArrayType_ReturnsUnsupported,
        Load_InvalidTypeOfStringType_ReturnsUnsupported,
        Load_InvalidTypeOfNumberType_ReturnsUnsupported,

        Load_DeserializeUnreflectedType_ReturnsUnsupported,
        Load_DeserializeEmptyObject_SucceedsAndObjectMatchesDefaults,
        Load_DeserializeEmptyObjectThroughMainLoad_SucceedsAndObjectMatchesDefaults,
        Load_DeserializeEmptyArray_SucceedsAndObjectMatchesDefaults,
        Load_DeserializeEmptyArrayWithClearEnabled_SucceedsAndObjectMatchesDefaults,
        Load_DeserializeEmptyArrayWithClearedTarget_SucceedsAndObjectMatchesDefaults,
        Load_DeserializeArrayWithDefaultValue_SucceedsAndReportPartialDefaults,
        Load_InterruptClearingTarget_ContainerIsNotCleared,
        Load_DeserializeFullySetInstance_SucceedsAndObjectMatchesFullySetInstance,
        Load_DeserializeFullySetInstanceThroughMainLoad_SucceedsAndObjectMatchesFullySetInstance,
        Load_DeserializePartialInstance_SucceedsAndObjectMatchesParialInstance,
        Load_DeserializeFullInstanceOnTopOfPartialDefaulted_SucceedsAndObjectMatchesParialInstance,
        Load_DefaultToPointer_SucceedsAndValueIsInitialized,
        Load_InitializeNewInstance_SucceedsAndValueIsInitialized,
        Load_DeserializeWithMissingMandatoryField_LoadFailedAndMissingReported,
        Load_InsertAdditionalData_SucceedsAndObjectMatchesFullySetInstance,
        Load_HaltedThroughCallback_LoadFailsAndHaltReported,
        Load_CorruptedValue_SucceedsWithDefaultValuesUsed,

        Store_SerializeUnreflectedType_ReturnsUnsupported,
        Store_SerializeDefaultInstance_EmptyJsonReturned,
        Store_SerializeDefaultInstanceThroughMainStore_EmptyJsonReturned,
        Store_SerializeWithDefaultsKept_FullyWrittenJson,
        Store_SerializeFullySetInstance_StoredSuccessfullyAndJsonMatches,
        Store_SerializeWithoutDefault_StoredSuccessfullyAndJsonMatches,
        Store_SerializeWithoutDefaultAndDefaultsKept_StoredSuccessfullyAndJsonMatches,
        Store_SerializePartialInstance_StoredSuccessfullyAndJsonMatches,
        Store_SerializeEmptyArray_StoredSuccessfullyAndJsonMatches,
        Store_SerializeArrayWithSingleDefaultValue_StoredSuccessfullyAndJsonMatches,
        Store_HaltedThroughCallback_StoreFailsAndHaltReported,

        StoreLoad_RoundTripWithPartialDefault_IdenticalInstances,
        StoreLoad_RoundTripWithFullSet_IdenticalInstances,
        StoreLoad_RoundTripWithDefaultsKept_IdenticalInstances,

        GetOperationFlags_RequiresExplicitInit_ObjectsThatDoNotConstructHaveExplicitInitOption,
        GetOperationFlags_ManualDefaultSetIfNeeded_ManualDefaultOperationSetIfMandatoryFieldsAreDeclared);
} // namespace JsonSerializationTests

namespace AZ
{
    namespace JsonSerializationResult
    {
        inline std::ostream& operator<<(std::ostream& stream, Tasks task)
        {
            switch (task)
            {
            case Tasks::RetrieveInfo:   stream << "Tasks::RetrieveInfo";    break;
            case Tasks::CreateDefault:  stream << "Tasks::CreateDefault";   break;
            case Tasks::Convert:        stream << "Tasks::Convert";         break;
            case Tasks::Clear:          stream << "Tasks::Clear";           break;
            case Tasks::ReadField:      stream << "Tasks::ReadField";       break;
            case Tasks::WriteValue:     stream << "Tasks::WriteValue";      break;
            case Tasks::Merge:          stream << "Tasks::Merge";           break;
            case Tasks::CreatePatch:    stream << "Tasks::CreatePatch";     break;
            default:                    stream << "Incorrect task type";
            }
            return stream;
        }

        inline std::ostream& operator<<(std::ostream& stream, Processing processing)
        {
            switch (processing)
            {
            case Processing::Completed:     stream << "Processing::Completed";      break;
            case Processing::Altered:       stream << "Processing::Altered";        break;
            case Processing::PartialAlter:  stream << "Processing::PartialAlter";   break;
            case Processing::Halted:        stream << "Processing::Halted";         break;
            default:                        stream << "Incorrect processing type";
            }
            return stream;
        }

        inline std::ostream& operator<<(std::ostream& stream, Outcomes outcome)
        {
            switch (outcome)
            {
            case Outcomes::Success:         stream << "Outcomes::Success";          break;
            case Outcomes::Skipped:         stream << "Outcomes::Skipped";          break;
            case Outcomes::PartialSkip:     stream << "Outcomes::PartialSkip";      break;
            case Outcomes::DefaultsUsed:    stream << "Outcomes::DefaultsUsed";     break;
            case Outcomes::PartialDefaults: stream << "Outcomes::PartialDefaults";  break;
            case Outcomes::Unavailable:     stream << "Outcomes::Unavailable";      break;
            case Outcomes::Unsupported:     stream << "Outcomes::Unsupported";      break;
            case Outcomes::TypeMismatch:    stream << "Outcomes::TypeMismatch";     break;
            case Outcomes::TestFailed:      stream << "Outcomes::TestFailed";       break;
            case Outcomes::Missing:         stream << "Outcomes::Missing";          break;
            case Outcomes::Invalid:         stream << "Outcomes::Invalid";          break;
            case Outcomes::Unknown:         stream << "Outcomes::Unknown";          break;
            case Outcomes::Catastrophic:    stream << "Outcomes::Catastrophic";     break;
            default:                        stream << "Incorrect outcome type";
            }
            return stream;
        }
    } // namespace JsonSerializationResult
} // namespace AZ
