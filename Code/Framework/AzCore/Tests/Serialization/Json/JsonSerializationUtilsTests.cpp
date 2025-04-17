/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzTest/AzTest.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

namespace UnitTest
{
    using namespace AZ;

    namespace Test1
    {
        class TestClass
        {
        public:
            AZ_TYPE_INFO(TestClass, "{731F8B22-086E-4CDE-9645-23078C6277C1}");
            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<TestClass>()
                        ->Version(1)
                        ->Field("int", &TestClass::m_int)
                        ->Field("float", &TestClass::m_float)
                        ->Field("string", &TestClass::m_string)
                        ->Field("unordered_map", &TestClass::m_unorderedMap)
                        ->Field("array", &TestClass::m_array)
                        ->Field("vector", &TestClass::m_vector)
                        ;
                }
            }

            int m_int = 0;
            float m_float = 0;
            AZStd::string m_string = "TestClass";
            AZStd::unordered_map<int, AZStd::string> m_unorderedMap;
            AZStd::array<AZStd::string, 2> m_array;
            AZStd::vector<AZStd::string> m_vector;

            void Init()
            {
                m_unorderedMap.emplace(1, "one");
                m_unorderedMap.emplace(5, "five");
                m_array[1] = "ONE";
                m_vector.push_back("anything");
                m_vector.push_back("something");
            }

            bool operator == (const TestClass& other) const
            {           
                return m_int == other.m_int
                    && m_float == other.m_float
                    && m_string == other.m_string
                    && m_unorderedMap == other.m_unorderedMap
                    && m_array == other.m_array
                    && m_vector == other.m_vector
                    ;
            }
        };
    }
    
    namespace Test2
    {
        // Test class which has same class name with TestClass but difference class id reflected in SerializeContext
        class TestClass
        {
        public:
            AZ_TYPE_INFO(TestClass, "{DAC825C5-AB14-4D9D-AAC2-124E56E1F8FD}");
            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<TestClass>()
                        ->Version(1)
                        ->Field("SomeData", &TestClass::m_someData)
                        ;
                }
            }

            int m_someData = 0;
        };
    }

    class JsonSerializationUtilsTests
        : public LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();
            m_jsonSystemComponent = AZStd::make_unique<AZ::JsonSystemComponent>();
                        
            m_serializationSettings.m_serializeContext = m_serializeContext.get();
            m_serializationSettings.m_registrationContext = m_jsonRegistrationContext.get();

            m_deserializationSettings.m_serializeContext = m_serializeContext.get();
            m_deserializationSettings.m_registrationContext = m_jsonRegistrationContext.get();

            m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());
            Test1::TestClass::Reflect(m_serializeContext.get());
            Test2::TestClass::Reflect(m_serializeContext.get());
        }

        void TearDown() override
        {
            m_jsonRegistrationContext->EnableRemoveReflection();
            m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());
            m_jsonRegistrationContext->DisableRemoveReflection();

            m_serializeContext->EnableRemoveReflection(); 
            Test1::TestClass::Reflect(m_serializeContext.get());
            Test2::TestClass::Reflect(m_serializeContext.get());
            m_serializeContext->DisableRemoveReflection();

            m_jsonRegistrationContext.reset();
            m_serializeContext.reset();
            m_jsonSystemComponent.reset();

            LeakDetectionFixture::TearDown();
        }

        AZStd::unique_ptr<SerializeContext> m_serializeContext;
        AZStd::unique_ptr<JsonRegistrationContext> m_jsonRegistrationContext;
        AZStd::unique_ptr<JsonSystemComponent> m_jsonSystemComponent;

        JsonSerializerSettings m_serializationSettings;
        JsonDeserializerSettings m_deserializationSettings; 
    };

    TEST_F(JsonSerializationUtilsTests, SaveLoadObjectToStream_Success)
    {
        char buffer[1024];
        IO::MemoryStream stream(buffer, 1024, 0);

        m_serializationSettings.m_keepDefaults = true;

        Test1::TestClass dataToSave;
        dataToSave.Init();
        dataToSave.m_float = 10;
        dataToSave.m_string = "SaveObjectToStreamSuccess";

        Outcome<void, AZStd::string> saveResult = JsonSerializationUtils::SaveObjectToStream(&dataToSave, stream, (Test1::TestClass*)nullptr, &m_serializationSettings);

        EXPECT_TRUE(saveResult.IsSuccess());

        Test1::TestClass loadedData;
        stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
        Outcome<void, AZStd::string> loadResult = JsonSerializationUtils::LoadObjectFromStream(loadedData, stream, &m_deserializationSettings);
        
        EXPECT_TRUE(loadResult.IsSuccess());

        EXPECT_TRUE(dataToSave == loadedData);
    }

    TEST_F(JsonSerializationUtilsTests, SaveLoadObjectToStream_WithCustomLocales_Success)
    {
        // This test is nearly identical to the above, but makes sure that if the system has a locale set
        // which uses a comma as a decimal separator, that the system can still serialize and deserialize
        // and does so in a way that is locale agnostic.  We do this by setting the locale to one that uses commas
        // and writing to the buffer (saving) and then setting it to a DIFFERENT locale and reading, in both directions
        // if the system is sensitive to locale, the test will fail becuase either one direction or the other will use the
        // system locale.

        auto testLocales = [&](const char* locale1, const char* locale2)
        {
            const char* priorLocale = setlocale(LC_ALL, nullptr);
            setlocale(LC_ALL, locale1);
            char buffer[1024];
            IO::MemoryStream stream(buffer, 1024, 0);

            m_serializationSettings.m_keepDefaults = true;

            Test1::TestClass dataToSave;
            dataToSave.Init();
            dataToSave.m_float = 10;
            dataToSave.m_string = "SaveObjectToStreamSuccess";

            Outcome<void, AZStd::string> saveResult = JsonSerializationUtils::SaveObjectToStream(&dataToSave, stream, (Test1::TestClass*)nullptr, &m_serializationSettings);

            EXPECT_TRUE(saveResult.IsSuccess());

            setlocale(LC_ALL, locale2);
            Test1::TestClass loadedData;
            stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            Outcome<void, AZStd::string> loadResult = JsonSerializationUtils::LoadObjectFromStream(loadedData, stream, &m_deserializationSettings);
        
            EXPECT_TRUE(loadResult.IsSuccess());

            EXPECT_TRUE(dataToSave == loadedData);

            setlocale(LC_ALL, priorLocale);
        };

        testLocales("en-US", "pl-PL");
        testLocales("pl-PL", "en-US");
        testLocales("en-US", "pl-PL");
        testLocales("pl-PL", "en-US");
    }

    TEST_F(JsonSerializationUtilsTests, SaveObjectToStream_Failed_NoSerializationContext)
    {
        char buffer[1024];
        IO::MemoryStream stream(buffer, 1024, 0);

        m_serializationSettings.m_keepDefaults = true;

        Test1::TestClass dataToSave;
        dataToSave.m_float = 10;

        Outcome<void, AZStd::string> saveResult = JsonSerializationUtils::SaveObjectToStream(&dataToSave, stream);

        EXPECT_TRUE(!saveResult.IsSuccess());
    }

    TEST_F(JsonSerializationUtilsTests, WriteJson)
    {
        rapidjson::Document document;
        document.SetObject();
        document.AddMember("a", 1, document.GetAllocator());
        document.AddMember("b", 2, document.GetAllocator());
        document.AddMember("c", 3, document.GetAllocator());

        const char* expectedJsonText =
            "{\n"
            "    \"a\": 1,\n"
            "    \"b\": 2,\n"
            "    \"c\": 3\n"
            "}";

        AZStd::string outString;
        AZ::Outcome<void, AZStd::string> result1 = JsonSerializationUtils::WriteJsonString(document, outString);
        EXPECT_TRUE(result1.IsSuccess());
        EXPECT_STREQ(expectedJsonText, outString.c_str());

        AZStd::vector<char> outBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char>> outStream{&outBuffer};
        AZ::Outcome<void, AZStd::string> result2 = JsonSerializationUtils::WriteJsonStream(document, outStream);
        EXPECT_TRUE(result2.IsSuccess());

        outBuffer.push_back(0);
        EXPECT_STREQ(expectedJsonText, outBuffer.data());

        // Unfortunately we can't unit test WriteJsonFile because core unit tests don't have access to the local file IO system.
    }

    TEST_F(JsonSerializationUtilsTests, ReadJsonString)
    {
        const char* jsonText =
            R"(
            {
                "a": 1,
                "b": 2,
                "c": 3
            })";

        AZ::Outcome<rapidjson::Document, AZStd::string> result = JsonSerializationUtils::ReadJsonString(jsonText);

        EXPECT_TRUE(result.IsSuccess());
        EXPECT_TRUE(result.GetValue().IsObject());
        EXPECT_TRUE(result.GetValue().HasMember("a"));
        EXPECT_TRUE(result.GetValue().HasMember("b"));
        EXPECT_TRUE(result.GetValue().HasMember("c"));
        EXPECT_EQ(result.GetValue()["a"].GetInt(), 1);
        EXPECT_EQ(result.GetValue()["b"].GetInt(), 2);
        EXPECT_EQ(result.GetValue()["c"].GetInt(), 3);
    }

    TEST_F(JsonSerializationUtilsTests, ReadJsonString_ErrorReportsLineNumber)
    {
        const char* jsonText =
            R"(
            {
                "a": "This line is missing a comma"
                "b": 2,
                "c": 3
            }
            )";

        AZ::Outcome<rapidjson::Document, AZStd::string> result = JsonSerializationUtils::ReadJsonString(jsonText);

        EXPECT_FALSE(result.IsSuccess());
        EXPECT_TRUE(result.GetError().find("JSON parse error at line 4:") == 0);
    }

    TEST_F(JsonSerializationUtilsTests, LoadJsonStream)
    {
        const char* jsonText =
            R"(
            {
                "a": 1,
                "b": 2,
                "c": 3
            })";

        IO::MemoryStream stream(jsonText, strlen(jsonText));

        AZ::Outcome<rapidjson::Document, AZStd::string> result = JsonSerializationUtils::ReadJsonStream(stream);

        EXPECT_TRUE(result.IsSuccess());
        EXPECT_TRUE(result.GetValue().IsObject());
        EXPECT_TRUE(result.GetValue().HasMember("a"));
        EXPECT_TRUE(result.GetValue().HasMember("b"));
        EXPECT_TRUE(result.GetValue().HasMember("c"));
        EXPECT_EQ(result.GetValue()["a"].GetInt(), 1);
        EXPECT_EQ(result.GetValue()["b"].GetInt(), 2);
        EXPECT_EQ(result.GetValue()["c"].GetInt(), 3);
    }

    TEST_F(JsonSerializationUtilsTests, LoadJsonStream_ErrorReportsLineNumber)
    {
        const char* jsonText =
            R"(
            {
                "a": 1,
                "b": "This line is missing a comma"
                "c": 3
            }
            )";

        IO::MemoryStream stream(jsonText, strlen(jsonText));

        AZ::Outcome<rapidjson::Document, AZStd::string> result = JsonSerializationUtils::ReadJsonStream(stream);

        EXPECT_FALSE(result.IsSuccess());
        EXPECT_TRUE(result.GetError().find("JSON parse error at line 5:") == 0);
    }
    
    TEST_F(JsonSerializationUtilsTests, LoadObjectFromStream_Failed_ParseError)
    {
        char buffer[1024] = "Not a Json";
        IO::MemoryStream stream(buffer, 1024);

        Test1::TestClass dataToLoad;

        Outcome<void, AZStd::string> loadResult = JsonSerializationUtils::LoadObjectFromStream(dataToLoad, stream, &m_deserializationSettings);

        EXPECT_TRUE(!loadResult.IsSuccess());
    }

    TEST_F(JsonSerializationUtilsTests, LoadObjectFromStream_Failed_NotJsonSerialization)
    {
        char buffer[1024] = "{}";
        IO::MemoryStream stream(buffer, 1024);

        Test1::TestClass dataToLoad;

        Outcome<void, AZStd::string> loadResult = JsonSerializationUtils::LoadObjectFromStream(dataToLoad, stream, &m_deserializationSettings);

        EXPECT_TRUE(!loadResult.IsSuccess());
    }

    TEST_F(JsonSerializationUtilsTests, LoadObjectFromStream_Failed_NoClassInfo)
    {
        char buffer[1024] =
            "{                                                   "
            "    \"Type\": \"JsonSerialization\"                "
            "}                                                   ";

        IO::MemoryStream stream(buffer, 1024);

        Test1::TestClass dataToLoad;

        Outcome<void, AZStd::string> loadResult = JsonSerializationUtils::LoadObjectFromStream(dataToLoad, stream, &m_deserializationSettings);

        EXPECT_TRUE(!loadResult.IsSuccess());
    }

    TEST_F(JsonSerializationUtilsTests, LoadObjectFromStream_Failed_MismatchClassName)
    {
        char buffer[1024] =
            "{                                                   "
            "    \"Type\": \"JsonSerialization\",                "
            "    \"ClassName\": \"NotTestClass\",                "
            "    \"ClassData\" : {}                              "
            "}                                                   ";
        IO::MemoryStream stream(buffer, 1024);

        Test1::TestClass dataToLoad;

        Outcome<void, AZStd::string> loadResult = JsonSerializationUtils::LoadObjectFromStream(dataToLoad, stream, &m_deserializationSettings);

        EXPECT_TRUE(!loadResult.IsSuccess());
    }

    TEST_F(JsonSerializationUtilsTests, LoadObjectFromStream_Failed_NoSerializeContext)
    {
        char buffer[1024] =
            "{                                                   "
            "    \"Type\": \"JsonSerialization\",                "
            "    \"ClassName\": \"TestClass\",                   "
            "    \"ClassData\" : {}                              "
            "}                                                   ";
        IO::MemoryStream stream(buffer, 1024);

        Test1::TestClass dataToLoad;

        Outcome<void, AZStd::string> loadResult = JsonSerializationUtils::LoadObjectFromStream(dataToLoad, stream);

        EXPECT_TRUE(!loadResult.IsSuccess());
    }
    

    TEST_F(JsonSerializationUtilsTests, LoadObjectFromStream_Failed_HaltMismatchClassMember)
    {
        char buffer[1024] =
            "{                                                   "
            "    \"Type\": \"JsonSerialization\",                "
            "    \"ClassName\": \"TestClass\",                   "
            "    \"ClassData\" : { \"uint\":\"10\",              "
            "                      \"bad name2\":\"blabla\"}     "
            "}                                                   ";
        IO::MemoryStream stream(buffer, 1024);

        Test1::TestClass dataToLoad;

        Outcome<void, AZStd::string> loadResult = JsonSerializationUtils::LoadObjectFromStream(dataToLoad, stream, &m_deserializationSettings);

        EXPECT_TRUE(!loadResult.IsSuccess());
    }
    
    TEST_F(JsonSerializationUtilsTests, LoadObjectFromStream_Failed_WrongValueType)
    {
        char buffer[1024] =
            "{                                                   "
            "    \"Type\": \"JsonSerialization\",                "
            "    \"ClassName\": \"TestClass\",                    "
            "    \"ClassData\" : { \"int\":\"Ten\"}               "
            "}                                                   ";
        IO::MemoryStream stream(buffer, 1024);

        Test1::TestClass dataToLoad;

        Outcome<void, AZStd::string> loadResult = JsonSerializationUtils::LoadObjectFromStream(dataToLoad, stream, &m_deserializationSettings);

        EXPECT_TRUE(!loadResult.IsSuccess());
    }

    TEST_F(JsonSerializationUtilsTests, LoadObjectFromStream_Success_LessField)
    {
        char buffer[1024] =
            "{                                                   "
            "    \"Type\": \"JsonSerialization\",                "
            "    \"ClassName\": \"TestClass\",                    "
            "    \"ClassData\" : { \"int\":\"10\"}               "
            "}                                                   ";
        IO::MemoryStream stream(buffer, 1024);

        Test1::TestClass dataToLoad;

        Outcome<void, AZStd::string> loadResult = JsonSerializationUtils::LoadObjectFromStream(dataToLoad, stream, &m_deserializationSettings);

        EXPECT_TRUE(loadResult.IsSuccess());
        EXPECT_TRUE(dataToLoad.m_int == 10);
    }
       
    TEST_F(JsonSerializationUtilsTests, LoadObjectFromStream_Success_CustomizeCallback)
    {
        char buffer[1024] =
            "{                                                   "
            "    \"Type\": \"JsonSerialization\",                "
            "    \"ClassName\": \"TestClass\",                    "
            "    \"ClassData\" : { \"int\":\"10\"}               "
            "}                                                   ";
        IO::MemoryStream stream(buffer, 1024);

        Test1::TestClass dataToLoad;

        AZStd::string callbackString;
        auto issueReportingCallback = [&callbackString](AZStd::string_view message, JsonSerializationResult::ResultCode result, AZStd::string_view target) -> JsonSerializationResult::ResultCode
        {
            using namespace JsonSerializationResult;
            AZ_UNUSED(message);
            AZ_UNUSED(target);
            callbackString = "issueReportingCallback";
            return result;
        };

        auto settings = m_deserializationSettings;
        settings.m_reporting = issueReportingCallback;
        Outcome<void, AZStd::string> loadResult = JsonSerializationUtils::LoadObjectFromStream(dataToLoad, stream, &settings);

        EXPECT_TRUE(loadResult.IsSuccess());
        EXPECT_TRUE(dataToLoad.m_int == 10);
        EXPECT_TRUE(!callbackString.empty());
    }
    
    TEST_F(JsonSerializationUtilsTests, LoadAnyObjectFromStream_Success)
    {
        char buffer[1024] =
            "{                                                   "
            "    \"Type\": \"JsonSerialization\",                "
            "    \"ClassName\": \"TestClass\",                   "
            "    \"ClassData\" : { \"int\":\"10\"}               "
            "}                                                   ";
        IO::MemoryStream stream(buffer, 1024);
        
        Outcome<AZStd::any, AZStd::string> loadResult = JsonSerializationUtils::LoadAnyObjectFromStream(stream, &m_deserializationSettings);

        EXPECT_TRUE(loadResult.IsSuccess());
        EXPECT_TRUE(loadResult.GetValue().type() == Test1::TestClass::TYPEINFO_Uuid());

        Test1::TestClass test = AZStd::any_cast<Test1::TestClass>(loadResult.GetValue());
        EXPECT_TRUE(test.m_int == 10);
    }

    TEST_F(JsonSerializationUtilsTests, LoadAnyObjectFromStream_Failed_WrongValueType)
    {
        char buffer[1024] =
            "{                                                   "
            "    \"Type\": \"JsonSerialization\",                "
            "    \"ClassName\": \"TestClass\",                    "
            "    \"ClassData\" : { \"int\":\"Ten\"}               "
            "}                                                   ";
        IO::MemoryStream stream(buffer, 1024);

        Outcome<AZStd::any, AZStd::string> loadResult = JsonSerializationUtils::LoadAnyObjectFromStream(stream, &m_deserializationSettings);

        EXPECT_TRUE(!loadResult.IsSuccess());
    }


} // namespace UnitTest

