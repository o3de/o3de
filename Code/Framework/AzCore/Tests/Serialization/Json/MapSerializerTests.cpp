/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Serialization/Json/MapSerializer.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>
#include <Tests/Serialization/Json/TestCases_Classes.h>

namespace JsonSerializationTests
{
    template<typename Map, typename Serializer>
    class MapBaseTestDescription :
        public JsonSerializerConformityTestDescriptor<Map>
    {
    public:
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<Serializer>();
        }

        AZStd::shared_ptr<Map> CreateDefaultInstance() override
        {
            return AZStd::make_shared<Map>();
        }

        AZStd::string_view GetJsonForSingleArrayDefaultInstance() override
        {
            return R"({ "{}": {} })";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kArrayType);
            features.EnableJsonType(rapidjson::kObjectType);
            features.m_requiresTypeIdLookups = true;
            features.m_supportsPartialInitialization = false;
        }

        using JsonSerializerConformityTestDescriptor<Map>::Reflect;
        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            context->RegisterGenericType<Map>();
        }

        bool AreEqual(const Map& lhs, const Map& rhs) override
        {
            return lhs == rhs;
        }
    };

    template<template<typename...> class T, typename Serializer, bool IsMultiMap>
    class MapArrayTestDescription final :
        public MapBaseTestDescription<T<int, double>, Serializer>
    {
    public:
        using Map = T<int, double>;

        AZStd::shared_ptr<Map> CreateSingleArrayDefaultInstance() override
        {
            auto instance = AZStd::make_shared<Map>();
            instance->emplace(AZStd::make_pair(0, 0.0));
            return instance;
        }

        AZStd::shared_ptr<Map> CreateFullySetInstance() override
        {
            auto instance = AZStd::make_shared<Map>();
            instance->emplace(AZStd::make_pair(142, 142.0));
            instance->emplace(AZStd::make_pair(242, 242.0));
            instance->emplace(AZStd::make_pair(342, 342.0));
            return instance;
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            if constexpr (IsMultiMap)
            {
                return R"(
                    [ 
                        { "Key": 142, "Value": [ 142.0 ] },
                        { "Key": 242, "Value": [ 242.0 ] },
                        { "Key": 342, "Value": [ 342.0 ] }
                    ])";
            }
            else
            {
                return R"(
                    [ 
                        { "Key": 142, "Value": 142.0 },
                        { "Key": 242, "Value": 242.0 },
                        { "Key": 342, "Value": 342.0 }
                    ])";
            }
        }
    };

    template<template<typename...> class T, typename Serializer, bool IsMultiMap>
    class MapObjectTestDescription final :
        public MapBaseTestDescription<T<AZStd::string, double>, Serializer>
    {
    public:
        using Map = T<AZStd::string, double>;

        AZStd::shared_ptr<Map> CreateSingleArrayDefaultInstance() override
        {
            auto instance = AZStd::make_shared<Map>();
            instance->emplace(AZStd::make_pair(AZStd::string(), 0.0));
            return instance;
        }

        AZStd::shared_ptr<Map> CreateFullySetInstance() override
        {
            auto instance = AZStd::make_shared<Map>();
            instance->emplace(AZStd::make_pair(AZStd::string("a"), 142.0));
            instance->emplace(AZStd::make_pair(AZStd::string("b"), 242.0));
            instance->emplace(AZStd::make_pair(AZStd::string("c"), 342.0));
            return instance;
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            if constexpr (IsMultiMap)
            {
                return R"(
                    {
                        "a": [ 142.0 ],
                        "b": [ 242.0 ],
                        "c": [ 342.0 ]
                    })";
            }
            else
            {
                return R"(
                    {
                        "a": 142.0,
                        "b": 242.0,
                        "c": 342.0
                    })";
            }
        }
    };

    template<template<typename...> class T, typename Serializer, bool IsMultiMap>
    class MapPointerTestDescription final :
        public MapBaseTestDescription<T<SimpleClass*, SimpleClass*>, Serializer>
    {
    public:
        using Map = T<SimpleClass*, SimpleClass*>;

        static void Delete(Map* map)
        {
            for (auto& entry : *map)
            {
                delete entry.first;
                delete entry.second;
            }
            delete map;
        }

        AZStd::shared_ptr<Map> CreateDefaultInstance() override
        {
            return AZStd::shared_ptr<Map>(new Map{}, &Delete);
        }

        AZStd::shared_ptr<Map> CreatePartialDefaultInstance() override
        {
            auto instance = AZStd::shared_ptr<Map>(new Map{}, &Delete);
            instance->emplace(AZStd::make_pair(aznew SimpleClass(), aznew SimpleClass(188, 188.0)));
            instance->emplace(AZStd::make_pair(aznew SimpleClass(242, 242.0), aznew SimpleClass()));
            instance->emplace(AZStd::make_pair(aznew SimpleClass(342, 342.0), aznew SimpleClass(388, 388.0)));
            return instance;
        }

        AZStd::shared_ptr<Map> CreateSingleArrayDefaultInstance() override
        {
            auto instance = AZStd::shared_ptr<Map>(new Map{}, &Delete);
            instance->emplace(AZStd::make_pair(aznew SimpleClass(), aznew SimpleClass()));
            return instance;
        }


        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            if constexpr (IsMultiMap)
            {
                return R"(
                    [ 
                        { "Key": {                            }, "Value": [ { "var1": 188, "var2": 188.0 } ] },
                        { "Key": { "var1": 242, "var2": 242.0 }, "Value": [ {                            } ] },
                        { "Key": { "var1": 342, "var2": 342.0 }, "Value": [ { "var1": 388, "var2": 388.0 } ] }
                    ])";
            }
            else
            {
                return R"(
                    [ 
                        { "Key": {                            }, "Value": { "var1": 188, "var2": 188.0 } },
                        { "Key": { "var1": 242, "var2": 242.0 }, "Value": {                            } },
                        { "Key": { "var1": 342, "var2": 342.0 }, "Value": { "var1": 388, "var2": 388.0 } }
                    ])";
            }
        }

        AZStd::shared_ptr<Map> CreateFullySetInstance() override
        {
            auto instance = AZStd::shared_ptr<Map>(new Map{}, &Delete);
            instance->emplace(AZStd::make_pair(aznew SimpleClass(142, 142.0), aznew SimpleClass(188, 188.0)));
            instance->emplace(AZStd::make_pair(aznew SimpleClass(242, 242.0), aznew SimpleClass(288, 288.0)));
            instance->emplace(AZStd::make_pair(aznew SimpleClass(342, 342.0), aznew SimpleClass(388, 388.0)));
            return instance;
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            if constexpr (IsMultiMap)
            {
                return R"(
                    [ 
                        { "Key": { "var1": 142, "var2": 142.0 }, "Value": [ { "var1": 188, "var2": 188.0 } ] },
                        { "Key": { "var1": 242, "var2": 242.0 }, "Value": [ { "var1": 288, "var2": 288.0 } ] },
                        { "Key": { "var1": 342, "var2": 342.0 }, "Value": [ { "var1": 388, "var2": 388.0 } ] }
                    ]
                )";
            }
            else
            {
                return R"(
                    [ 
                        { "Key": { "var1": 142, "var2": 142.0 }, "Value": { "var1": 188, "var2": 188.0 } },
                        { "Key": { "var1": 242, "var2": 242.0 }, "Value": { "var1": 288, "var2": 288.0 } },
                        { "Key": { "var1": 342, "var2": 342.0 }, "Value": { "var1": 388, "var2": 388.0 } }
                    ]
                )";
            }
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            MapBaseTestDescription<T<SimpleClass*, SimpleClass*>, Serializer>::ConfigureFeatures(features);
            features.m_supportsPartialInitialization = true;
        }

        using MapBaseTestDescription<T<SimpleClass*, SimpleClass*>, Serializer>::Reflect;
        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            SimpleClass::Reflect(context, true);
            MapBaseTestDescription<T<SimpleClass*, SimpleClass*>, Serializer>::Reflect(context);
        }

        bool AreEqual(const Map& lhs, const Map& rhs) override
        {
            if (lhs.size() != rhs.size())
            {
                return false;
            }

            // Naive compare to avoid having to split up the test because comparing for ordered and unordered maps would need to be
            // different.
            for (auto&& [key, value] : lhs)
            {
                for (auto&& [keyCompare, valueCompare] : rhs)
                {
                    if (key->Equals(*keyCompare, true))
                    {
                        if (!value->Equals(*valueCompare, true))
                        {
                            return false;
                        }
                        break;
                    }
                }
            }
            return true;
        }
    };

    using MapSerializerConformityTestTypes = ::testing::Types<
        // Tests for storing to arrays.
        MapArrayTestDescription<AZStd::map, AZ::JsonMapSerializer, false>,
        MapArrayTestDescription<AZStd::unordered_map, AZ::JsonUnorderedMapSerializer, false>,
        MapArrayTestDescription<AZStd::unordered_multimap, AZ::JsonUnorderedMultiMapSerializer, true>,
        // Tests for storing to strings.
        MapObjectTestDescription<AZStd::map, AZ::JsonMapSerializer, false>,
        MapObjectTestDescription<AZStd::unordered_map, AZ::JsonUnorderedMapSerializer, false>,
        MapObjectTestDescription<AZStd::unordered_multimap, AZ::JsonUnorderedMultiMapSerializer, true>,
        // Test for storing pointers for both key and value.
        MapPointerTestDescription<AZStd::map, AZ::JsonMapSerializer, false>,
        MapPointerTestDescription<AZStd::unordered_map, AZ::JsonUnorderedMapSerializer, false>,
        MapPointerTestDescription<AZStd::unordered_multimap, AZ::JsonUnorderedMultiMapSerializer, true>
    >;
    INSTANTIATE_TYPED_TEST_CASE_P(JsonMapSerializer, JsonSerializerConformityTests, MapSerializerConformityTestTypes);


    struct TestString
    {
        AZ_TYPE_INFO(TestString, "{BBC6FCD2-9FED-425E-8523-0598575B960A}");
        AZStd::string m_value;
        TestString() : m_value("Hello") {}
        explicit TestString(AZStd::string_view value) : m_value(value) {}

        bool operator==(const TestString& rhs) const { return m_value.compare(rhs.m_value) == 0; }
        bool operator!=(const TestString& rhs) const { return m_value.compare(rhs.m_value) != 0; }
    };

    class TestStringSerializer
        : public AZ::BaseJsonSerializer
    {
    public:
        AZ_RTTI(TestStringSerializer, "{05012877-0A5C-4514-8AC2-695E753C77A2}", BaseJsonSerializer);
        AZ_CLASS_ALLOCATOR(TestStringSerializer, AZ::SystemAllocator, 0);

        AZ::JsonSerializationResult::Result Load(void* outputValue, const AZ::Uuid&, const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context) override
        {
            using namespace AZ::JsonSerializationResult;
            *reinterpret_cast<AZStd::string*>(outputValue) = AZStd::string_view(inputValue.GetString(), inputValue.GetStringLength());
            return context.Report(Tasks::ReadField, Outcomes::Success, "Test load");
        }
        AZ::JsonSerializationResult::Result Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            const AZ::Uuid& valueTypeId, AZ::JsonSerializerContext& context) override
        {
            AZ_UNUSED(valueTypeId);

            using namespace AZ::JsonSerializationResult;
            const AZStd::string* inputString = reinterpret_cast<const AZStd::string*>(inputValue);
            const AZStd::string* defaultString = reinterpret_cast<const AZStd::string*>(defaultValue);
            if (defaultString && *inputString == *defaultString)
            {
                return context.Report(Tasks::WriteValue, Outcomes::DefaultsUsed, "Test store default");
            }
            outputValue.SetString(inputString->c_str(), context.GetJsonAllocator());
            return context.Report(Tasks::WriteValue, Outcomes::Success, "Test store");
        }
    };

    struct TestStringHasher
    {
        AZ_TYPE_INFO(TestStringHasher, "{A59EE3DA-5BB2-4F0B-8390-0832E69F5089}");

        AZStd::size_t operator()(const TestString& value) const
        {
            return AZStd::hash<AZStd::string>{}(value.m_value);
        }
    };

    struct SimpleClassHasher
    {
        AZ_TYPE_INFO(SimpleClassHasher, "{79CF2662-36BF-4183-9400-408F0CFB2AA6}");

        AZStd::size_t operator()(const SimpleClass& value) const
        {
            return
                AZStd::hash<decltype(value.m_var1)>{}(value.m_var1) << 32 |
                AZStd::hash<decltype(value.m_var2)>{}(value.m_var2);
        }
    };

    class JsonMapSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        using StringMap = AZStd::map<AZStd::string, AZStd::string>;
        using StringMultiMap = AZStd::unordered_multimap<AZStd::string, AZStd::string>;
        using TestStringMap = AZStd::unordered_map<TestString, TestString, TestStringHasher>;
        using TestStringMultiMap = AZStd::unordered_multimap<TestString, TestString, TestStringHasher>;
        using SimpleClassMap = AZStd::unordered_map<SimpleClass, SimpleClass, SimpleClassHasher>;
        using SimpleClassMultiMap = AZStd::unordered_multimap<SimpleClass, SimpleClass, SimpleClassHasher>;

        enum TestBitFlags : int64_t
        {
            BitFlag0 = 0x1,
            BitFlag1 = 0x2,
            BitFlag2 = 0x4,
            BitFlag3 = 0x8
        };

        using BitFlagMap = AZStd::unordered_map<TestBitFlags, int>;

        void RegisterAdditional(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            SimpleClass::Reflect(context, true);
            
            context->Class<TestString>()->Field("Value", &TestString::m_value);

            context->RegisterGenericType<StringMap>();
            context->RegisterGenericType<StringMultiMap>();
            context->RegisterGenericType<TestStringMap>();
            context->RegisterGenericType<TestStringMultiMap>();
            context->RegisterGenericType<SimpleClassMap>();
            context->RegisterGenericType<SimpleClassMultiMap>();
            context->RegisterGenericType<BitFlagMap>();

            context->Enum<TestBitFlags>()
                ->Value("BitFlag0", BitFlag0)
                ->Value("BitFlag1", BitFlag1)
                ->Value("BitFlag2", BitFlag2)
                ->Value("BitFlag3", BitFlag3);
        }

        void RegisterAdditional(AZStd::unique_ptr<AZ::JsonRegistrationContext>& context) override
        {
            context->Serializer<TestStringSerializer>()->HandlesType<TestString>();
        }

    protected:
        AZ::JsonMapSerializer m_mapSerializer;
        AZ::JsonUnorderedMapSerializer m_unorderedMapSerializer;
        AZ::JsonUnorderedMultiMapSerializer m_unorderedMultiMapSerializer;
    };
 }

 namespace AZ
 {
     AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::JsonMapSerializerTests::TestBitFlags, "{9859EDCD-1F5A-4893-8188-E691569DF2DA}");
 }

 namespace JsonSerializationTests
 {
    TEST_F(JsonMapSerializerTests, Load_DefaultForStringKey_LoadedBackWithDefaults)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse(R"(
        {
            "{}": {}
        })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        TestStringMap values;
        ResultCode result = m_unorderedMapSerializer.Load(&values, azrtti_typeid(&values), *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());

        EXPECT_EQ(1, values.size());

        auto defaultKey = values.find(TestString());
        EXPECT_NE(values.end(), defaultKey);
        EXPECT_STRCASEEQ(TestString().m_value.c_str(), defaultKey->second.m_value.c_str());
    }

    TEST_F(JsonMapSerializerTests, Load_DefaultForStringKeyAndAdditionalValue_LoadedBackWithDefaults)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse(R"(
            {
                "{}": {},
                "World": "value_42"
            })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        TestStringMap values;
        ResultCode result = m_unorderedMapSerializer.Load(&values, azrtti_typeid(&values), *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());

        EXPECT_EQ(2, values.size());

        auto defaultKey = values.find(TestString());
        EXPECT_NE(values.end(), defaultKey);
        EXPECT_STRCASEEQ(TestString().m_value.c_str(), defaultKey->second.m_value.c_str());

        auto worldKey = values.find(TestString("World"));
        EXPECT_NE(values.end(), worldKey);
        EXPECT_STRCASEEQ("value_42", worldKey->second.m_value.c_str());
    }

    TEST_F(JsonMapSerializerTests, Load_DuplicateKey_EntryIgnored)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse(R"(
            {
                "Hello": "World",
                "Hello": "Other"
            })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        StringMap values;
        ResultCode result = m_unorderedMapSerializer.Load(&values, azrtti_typeid(&values), *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(Processing::PartialAlter, result.GetProcessing());
        EXPECT_EQ(Outcomes::Unavailable, result.GetOutcome());

        auto entry = values.find("Hello");
        ASSERT_NE(values.end(), entry);
        EXPECT_STRCASEEQ("World", entry->second.c_str());
    }

    TEST_F(JsonMapSerializerTests, Load_DuplicateMultiKey_LoadEverything)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse(R"(
            {
                "Hello": [ "Other" ],
                "Hello": [ "World" ]
            })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        StringMultiMap values;
        ResultCode result = m_unorderedMultiMapSerializer.Load(&values,
            azrtti_typeid(&values), *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());

        auto range = values.equal_range("Hello");
        ASSERT_EQ(2, AZStd::distance(range.first, range.second));

        auto entry = range.first;
        EXPECT_TRUE(entry->second == "Other" || entry->second == "World");
        ++entry;
        EXPECT_TRUE(entry->second == "Other" || entry->second == "World");
    }

    TEST_F(JsonMapSerializerTests, Load_KeyAlreadyInContainer_EntryIgnored)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse(R"({ "Hello": "World" })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        StringMap values;
        values.emplace("Hello", "World");
        ResultCode result = m_unorderedMapSerializer.Load(&values,
            azrtti_typeid(&values), *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(Processing::Altered, result.GetProcessing());
        EXPECT_EQ(Outcomes::Unavailable, result.GetOutcome());

        auto entry = values.find("Hello");
        ASSERT_NE(values.end(), entry);
        EXPECT_STRCASEEQ("World", entry->second.c_str());
    }

    TEST_F(JsonMapSerializerTests, Load_KeyAlreadyInMultiContainer_DuplicateAdded)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse(R"({ "Hello": [ "World" ] })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        StringMultiMap values;
        values.emplace("Hello", "World");
        ResultCode result = m_unorderedMultiMapSerializer.Load(&values,
            azrtti_typeid(&values), *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());

        auto entry = values.find("Hello");
        ASSERT_NE(values.end(), entry);
        EXPECT_STRCASEEQ("World", entry->second.c_str());
        ++entry;
        EXPECT_STRCASEEQ("World", entry->second.c_str());
    }

    TEST_F(JsonMapSerializerTests, Load_ExplicitlyFailValueLoading_CatastrophicEventPropagated)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse(R"({ "Hello": "World" })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        int counter = 0;
        AZ::ScopedContextReporter reporter(*m_jsonDeserializationContext,[&counter](
            AZStd::string_view message, ResultCode result, AZStd::string_view path) -> ResultCode
        {
            AZ_UNUSED(message);
            AZ_UNUSED(path);

            return ++counter == 2
                ? ResultCode(result.GetTask(), Outcomes::Catastrophic)
                : result;
           
        });

        StringMap values;
        ResultCode result = m_unorderedMapSerializer.Load(&values, azrtti_typeid(&values), *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(Processing::Halted, result.GetProcessing());
        EXPECT_EQ(Outcomes::Catastrophic, result.GetOutcome());
    }

    TEST_F(JsonMapSerializerTests, Load_ExplicitlyFailValueLoadingInMultiContainer_CatastrophicEventPropagated)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse(R"({ "Hello": [ "World" ] })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        int counter = 0;
        AZ::ScopedContextReporter reporter(*m_jsonDeserializationContext, [&counter](
            AZStd::string_view message, ResultCode result, AZStd::string_view path) -> ResultCode
        {
            AZ_UNUSED(message);
            AZ_UNUSED(path);

            return ++counter == 2
                ? ResultCode(result.GetTask(), Outcomes::Catastrophic)
                : result;

        });

        StringMap values;
        ResultCode result = m_unorderedMultiMapSerializer.Load(&values, azrtti_typeid(&values),
            *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(Processing::Halted, result.GetProcessing());
        EXPECT_EQ(Outcomes::Catastrophic, result.GetOutcome());
    }

    TEST_F(JsonMapSerializerTests, Load_DefaultObjectInMultiMap_DefaultUsed)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse(R"(
            {
                "World": {}
            })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        TestStringMultiMap values;
        ResultCode result = m_unorderedMultiMapSerializer.Load(&values,
            azrtti_typeid(&values), *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());

        ASSERT_FALSE(values.empty());
        EXPECT_STREQ("World", values.begin()->first.m_value.c_str());
        EXPECT_STREQ(TestString().m_value.c_str(), values.begin()->second.m_value.c_str());
    }

    TEST_F(JsonMapSerializerTests, Load_FullDefaultObjectInMultiMap_DefaultUsed)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse(R"(
            {
                "{}": {}
            })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        TestStringMultiMap values;
        ResultCode result =
            m_unorderedMultiMapSerializer.Load(&values, azrtti_typeid(&values), *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());

        ASSERT_FALSE(values.empty());
        EXPECT_STREQ("Hello", values.begin()->first.m_value.c_str());
        EXPECT_STREQ("Hello", values.begin()->second.m_value.c_str());
        EXPECT_STREQ(TestString().m_value.c_str(), values.begin()->second.m_value.c_str());
    }

    TEST_F(JsonMapSerializerTests, Load_DefaultObjectValueInMultiMap_DefaultUsed)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse(R"(
            {
                "World": [{}]
            })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        TestStringMultiMap values;
        ResultCode result = m_unorderedMultiMapSerializer.Load(&values,
            azrtti_typeid(&values), *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());

        ASSERT_FALSE(values.empty());
        EXPECT_STREQ("World", values.begin()->first.m_value.c_str());
        EXPECT_STREQ("Hello", values.begin()->second.m_value.c_str());
        EXPECT_STREQ(TestString().m_value.c_str(), values.begin()->second.m_value.c_str());
    }

    TEST_F(JsonMapSerializerTests, Store_DefaultsWithStringKey_InitializedAsDefault)
    {
        using namespace AZ::JsonSerializationResult;

        TestStringMap values;
        values.emplace(TestString(), TestString("World"));

        ResultCode result = m_unorderedMapSerializer.Store(*m_jsonDocument, &values, nullptr,
            azrtti_typeid(&values), *m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
        Expect_DocStrEq(R"(
            {
                "{}": "World"
            })");
    }

    TEST_F(JsonMapSerializerTests, Store_DefaultsWithStringKeyAndValue_InitializedAsDefault)
    {
        using namespace AZ::JsonSerializationResult;
        
        TestStringMap values;
        values.emplace(TestString(), TestString());

        ResultCode result = m_unorderedMapSerializer.Store(*m_jsonDocument, &values, nullptr,
            azrtti_typeid(&values), *m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
        Expect_DocStrEq(R"(
            {
                "{}": {}
            })");
    }

    TEST_F(JsonMapSerializerTests, Store_DefaultsWithMultiStringKeyAndValue_InitializedAsDefault)
    {
        using namespace AZ::JsonSerializationResult;

        TestStringMultiMap values;
        values.emplace(TestString(), TestString());

        ResultCode result = m_unorderedMultiMapSerializer.Store(*m_jsonDocument, &values, nullptr,
            azrtti_typeid(&values), *m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
        Expect_DocStrEq(R"(
            {
                "{}": {}
            })");
    }

    TEST_F(JsonMapSerializerTests, Store_SingleAllDefaulValue_InitializedWithDefaults)
    {
        using namespace AZ::JsonSerializationResult;

        SimpleClassMap values;
        values.emplace(SimpleClass(), SimpleClass());
        
        ResultCode result =
            m_unorderedMapSerializer.Store(*m_jsonDocument, &values, nullptr, azrtti_typeid(&values), *m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
        Expect_DocStrEq(R"(
            {
                "{}": {}
            })");
    }

    TEST_F(JsonMapSerializerTests, Store_DefaultsWithObjectKey_InitializedWithDefaults)
    {
        using namespace AZ::JsonSerializationResult;

        SimpleClassMap values;
        values.emplace(SimpleClass(          ), SimpleClass(          ));
        values.emplace(SimpleClass(142       ), SimpleClass(188       ));
        values.emplace(SimpleClass(242, 342.0), SimpleClass(288, 388.0));

        ResultCode result = m_unorderedMapSerializer.Store(*m_jsonDocument, &values, nullptr, azrtti_typeid(&values), *m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
        Expect_DocStrEq(R"(
            [
                { "Key": {                          }, "Value": {                          } },
                { "Key": {"var1": 142               }, "Value": {"var1": 188               } },
                { "Key": {"var1": 242, "var2": 342.0}, "Value": {"var1": 288, "var2": 388.0} }
            ])");
    }

    TEST_F(JsonMapSerializerTests, Store_DefaultsWithObjectMultiKey_InitializedWithDefaults)
    {
        using namespace AZ::JsonSerializationResult;

        SimpleClassMultiMap values;
        values.emplace(SimpleClass(), SimpleClass());
        values.emplace(SimpleClass(), SimpleClass(188));
        values.emplace(SimpleClass(), SimpleClass(288, 388.0));
        values.emplace(SimpleClass(142), SimpleClass());
        values.emplace(SimpleClass(142), SimpleClass(188));
        values.emplace(SimpleClass(142), SimpleClass(288, 388.0));
        values.emplace(SimpleClass(242, 342.0), SimpleClass());
        values.emplace(SimpleClass(242, 342.0), SimpleClass(188));
        values.emplace(SimpleClass(242, 342.0), SimpleClass(288, 388.0));

        ResultCode result = m_unorderedMultiMapSerializer.Store(*m_jsonDocument, &values, nullptr,
            azrtti_typeid(&values), *m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
        Expect_DocStrEq(R"(
            [
                {
                    "Key": {},
                    "Value":
                    [
                        {},
                        { "var1": 188 },
                        { "var1": 288, "var2": 388.0 }
                    ]
                },
                {
                    "Key": {"var1": 142},
                    "Value":
                    [
                        {},
                        { "var1": 188 },
                        { "var1": 288, "var2": 388.0 }
                    ]
                },
                {
                    "Key": {"var1": 242, "var2": 342.0},
                    "Value":
                    [
                        {},
                        { "var1": 188 },
                        { "var1": 288, "var2": 388.0 }
                    ]
                }
            ])");
    }

    TEST_F(JsonMapSerializerTests, Store_MultiStringKeyValuesAreCombined_SingleFieldWithMultipleValuesInArray)
    {
        using namespace AZ::JsonSerializationResult;

        StringMultiMap values;
        values.emplace("Hello", "Big");
        values.emplace("Hello", "Another");
        values.emplace("Hello", "Compiler");

        ResultCode result = m_unorderedMultiMapSerializer.Store(*m_jsonDocument, &values, nullptr,
            azrtti_typeid(&values), *m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        Expect_DocStrEq(R"(
            {
                "Hello": [ "Another", "Big", "Compiler" ]
            })");
    }

    TEST_F(JsonMapSerializerTests, Store_MultiKeyDefaultValue_NoArrayUsed)
    {
        using namespace AZ::JsonSerializationResult;

        SimpleClassMultiMap values;
        values.emplace(SimpleClass(142, 242.0), SimpleClass());
        
        ResultCode result = m_unorderedMultiMapSerializer.Store(*m_jsonDocument, &values, nullptr,
            azrtti_typeid(&values), *m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
        Expect_DocStrEq(R"(
            [
                { "Key": {"var1": 142, "var2": 242.0}, "Value": {} }
            ])");
    }

    TEST_F(JsonMapSerializerTests, Store_MultiStringKeyDefaultValue_NoArrayUsed)
    {
        using namespace AZ::JsonSerializationResult;

        TestStringMultiMap values;
        values.emplace(TestString("World"), TestString());

        ResultCode result = m_unorderedMultiMapSerializer.Store(*m_jsonDocument, &values, nullptr,
            azrtti_typeid(&values), *m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
        Expect_DocStrEq(R"(
            {
                "World": {}
            })");
    }

    TEST_F(JsonMapSerializerTests, Store_MixOfStringAndOtherKeys_ArrayIsNotConvertedToObject)
    {
        using namespace AZ::JsonSerializationResult;
        BitFlagMap values;
        values.emplace(BitFlag0, 1);
        values.emplace(aznumeric_caster(BitFlag1 | BitFlag2), 6);
        values.emplace(aznumeric_caster(32), 32);

        ResultCode result = m_unorderedMapSerializer.Store(*m_jsonDocument, &values, nullptr, azrtti_typeid(&values), *m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        Expect_DocStrEq(R"(
            [
                { "Key": ["BitFlag2", "BitFlag1"], "Value": 6 },
                { "Key": "BitFlag0", "Value": 1 },
                { "Key": 32, "Value": 32 }
            ])");
    }
} // namespace JsonSerializationTests
