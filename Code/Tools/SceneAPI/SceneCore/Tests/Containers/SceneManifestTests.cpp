/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/DataTypes/IManifestObject.h>
#include <string>

extern "C" AZ_DLL_EXPORT void CleanUpSceneCoreGenericClassInfo();

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            const int BUFFER_SIZE = 64 * 1024;

            static decltype(SceneManifest::s_invalidIndex) INVALID_INDEX(SceneManifest::s_invalidIndex); // gtest cannot compare static consts

            class MockManifestInt : public DataTypes::IManifestObject
            {
            public:
                AZ_RTTI(MockManifestInt, "{D6F96B49-4E6F-4EE8-A5A3-959B76F90DA8}", IManifestObject);
                AZ_CLASS_ALLOCATOR(MockManifestInt, AZ::SystemAllocator, 0);

                MockManifestInt()
                    : m_value(0)
                {
                }

                MockManifestInt(int64_t value)
                    : m_value(value)
                {
                }

                int64_t GetValue() const
                {
                    return m_value;
                }

                void SetValue(int64_t value)
                {
                    m_value = value;
                }

                static void Reflect(AZ::ReflectContext* context)
                {
                    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                    if (serializeContext)
                    {
                        serializeContext->
                            Class<MockManifestInt, IManifestObject>()->
                            Version(1)->
                            Field("value", &MockManifestInt::m_value);
                    }
                }

            protected:
                int64_t m_value;
            };

            class MockSceneManifest
                : public SceneManifest
            {
            public:
                AZ_RTTI(MockSceneManifest, "{E6B3247F-1B48-49F8-B514-18FAC77C0F94}", SceneManifest);
                AZ_CLASS_ALLOCATOR(MockSceneManifest, AZ::SystemAllocator, 0);

                AZ::Outcome<void, AZStd::string> LoadFromString(const AZStd::string& fileContents, SerializeContext* context, JsonRegistrationContext* registrationContext, bool loadXml = false)
                {
                    return SceneManifest::LoadFromString(fileContents, context, registrationContext, loadXml);
                }

                AZ::Outcome<rapidjson::Document, AZStd::string> SaveToJsonDocument(SerializeContext* context, JsonRegistrationContext* registrationContext)
                {
                    return SceneManifest::SaveToJsonDocument(context, registrationContext);
                }

                
                static void Reflect(AZ::ReflectContext* context)
                {
                    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                    if (serializeContext)
                    {
                        serializeContext->
                            Class<MockSceneManifest, SceneManifest>()->
                            Version(1);
                    }
                }
                
            };


            class SceneManifestTest
                : public UnitTest::AllocatorsTestFixture
                , public AZ::Debug::TraceMessageBus::Handler
            {
            public:
                SceneManifestTest()
                {
                    m_firstDataObject = AZStd::make_shared<MockManifestInt>(1);
                    m_secondDataObject = AZStd::make_shared<MockManifestInt>(2);
                    m_testDataObject = AZStd::make_shared<MockManifestInt>(3);

                    m_testManifest.AddEntry(m_firstDataObject);
                    m_testManifest.AddEntry(m_secondDataObject);
                    m_testManifest.AddEntry(m_testDataObject);
                }

                void SetUp() override
                {
                    m_serializeContext = AZStd::make_unique<SerializeContext>();
                    m_jsonRegistrationContext = AZStd::make_unique<JsonRegistrationContext>();
                    m_jsonSystemComponent = AZStd::make_unique<JsonSystemComponent>();

                    m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());

                    DataTypes::IManifestObject::Reflect(m_serializeContext.get());
                    MockManifestInt::Reflect(m_serializeContext.get());
                    SceneManifest::Reflect(m_serializeContext.get());
                    MockSceneManifest::Reflect(m_serializeContext.get());

                    BusConnect();
                }

                void TearDown() override
                {
                    BusDisconnect();

                    m_jsonRegistrationContext->EnableRemoveReflection();
                    m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());
                    m_jsonRegistrationContext->DisableRemoveReflection();

                    m_serializeContext->EnableRemoveReflection();
                    DataTypes::IManifestObject::Reflect(m_serializeContext.get());
                    MockManifestInt::Reflect(m_serializeContext.get());
                    SceneManifest::Reflect(m_serializeContext.get());
                    MockSceneManifest::Reflect(m_serializeContext.get());
                    m_serializeContext->DisableRemoveReflection();

                    m_serializeContext.reset();
                    m_jsonRegistrationContext.reset();
                    m_jsonSystemComponent.reset();

                    CleanUpSceneCoreGenericClassInfo();
                }

                bool OnPreAssert(const char* /*message*/, int /*line*/, const char* /*func*/, const char* /*message*/) override
                {
                    m_assertTriggered = true;
                    return true;
                }
                bool m_assertTriggered = false;

                AZStd::shared_ptr<MockManifestInt> m_firstDataObject;
                AZStd::shared_ptr<MockManifestInt> m_secondDataObject;
                AZStd::shared_ptr<MockManifestInt> m_testDataObject;
                MockSceneManifest m_testManifest;
                AZStd::unique_ptr<SerializeContext> m_serializeContext;
                AZStd::unique_ptr<JsonRegistrationContext> m_jsonRegistrationContext;
                AZStd::unique_ptr<JsonSystemComponent> m_jsonSystemComponent;
            };

            TEST_F(SceneManifestTest, IsEmpty_Empty_True)
            {
                SceneManifest testManifest;
                EXPECT_TRUE(testManifest.IsEmpty());
            }

            TEST_F(SceneManifestTest, AddEntry_AddNewValue_ResultTrue)
            {
                SceneManifest testManifest;
                AZStd::shared_ptr<MockManifestInt> testDataObject = AZStd::make_shared<MockManifestInt>(100);
                bool result = testManifest.AddEntry(testDataObject);
                EXPECT_TRUE(result);
            }

            TEST_F(SceneManifestTest, AddEntry_MoveNewValue_ResultTrueAndPointerClear)
            {
                SceneManifest testManifest;
                AZStd::shared_ptr<MockManifestInt> testDataObject = AZStd::make_shared<MockManifestInt>(100);
                bool result = testManifest.AddEntry(AZStd::move(testDataObject));
                EXPECT_TRUE(result);
                EXPECT_EQ(nullptr, testDataObject.get());
            }

            //dependent on AddEntry
            TEST_F(SceneManifestTest, IsEmpty_NotEmpty_False)
            {
                SceneManifest testManifest;
                AZStd::shared_ptr<MockManifestInt> testDataObject = AZStd::make_shared<MockManifestInt>(100);
                testManifest.AddEntry(AZStd::move(testDataObject));
                EXPECT_FALSE(testManifest.IsEmpty());
            }

            //dependent on AddEntry and IsEmpty
            TEST_F(SceneManifestTest, Clear_NotEmpty_EmptyTrue)
            {
                SceneManifest testManifest;
                AZStd::shared_ptr<MockManifestInt> testDataObject = AZStd::make_shared<MockManifestInt>(100);
                testManifest.AddEntry(AZStd::move(testDataObject));
                EXPECT_FALSE(testManifest.IsEmpty());
                testManifest.Clear();
                EXPECT_TRUE(testManifest.IsEmpty());
            }

            //RemoveEntry
            TEST_F(SceneManifestTest, RemoveEntry_NameInList_ResultTrueAndNotStillInList)
            {
                SceneManifest testManifest;
                AZStd::shared_ptr<MockManifestInt> testDataObject = AZStd::make_shared<MockManifestInt>(1);
                testManifest.AddEntry(testDataObject);

                bool result = testManifest.RemoveEntry(testDataObject);
                EXPECT_TRUE(result);
            }

            TEST_F(SceneManifestTest, RemoveEntry_NameNotInList_ResultFalse)
            {
                SceneManifest testManifest;
                AZStd::shared_ptr<MockManifestInt> testDataObject = AZStd::make_shared<MockManifestInt>(1);
                
                testManifest.RemoveEntry(testDataObject);
                EXPECT_TRUE(m_assertTriggered);
            }

            // GetEntryCount
            TEST_F(SceneManifestTest, GetEntryCount_EmptyManifest_CountIsZero)
            {
                SceneManifest testManifest;
                EXPECT_TRUE(testManifest.IsEmpty());
                EXPECT_EQ(0, testManifest.GetEntryCount());
            }

            TEST_F(SceneManifestTest, GetEntryCount_FilledManifest_CountIsThree)
            {
                EXPECT_EQ(3, m_testManifest.GetEntryCount());
            }

            // GetValue
            TEST_F(SceneManifestTest, GetValue_ValidIndex_ReturnsInt2)
            {
                AZStd::shared_ptr<MockManifestInt> result = azrtti_cast<MockManifestInt*>(m_testManifest.GetValue(1));
                ASSERT_TRUE(result);
                EXPECT_EQ(2, result->GetValue());
            }

            TEST_F(SceneManifestTest, GetValue_InvalidIndex_ReturnsNullPtr)
            {
                EXPECT_EQ(nullptr, m_testManifest.GetValue(42));
            }

            // FindIndex
            TEST_F(SceneManifestTest, FindIndex_ValidValue_ResultIsOne)
            {
                EXPECT_EQ(1, m_testManifest.FindIndex(m_secondDataObject.get()));
            }

            TEST_F(SceneManifestTest, FindIndex_InvalidValueFromSharedPtr_ResultIsInvalidIndex)
            {
                AZStd::shared_ptr<DataTypes::IManifestObject> invalid = AZStd::make_shared<MockManifestInt>(42);
                EXPECT_EQ(INVALID_INDEX, m_testManifest.FindIndex(invalid.get()));
            }

            TEST_F(SceneManifestTest, FindIndex_InvalidValueFromNullptr_ResultIsInvalidIndex)
            {
                DataTypes::IManifestObject* invalid = nullptr;
                EXPECT_EQ(INVALID_INDEX, m_testManifest.FindIndex(invalid));
            }

            // RemoveEntry - continued
            TEST_F(SceneManifestTest, RemoveEntry_IndexAdjusted_IndexReduced)
            {
                SceneManifest testManifest;
                AZStd::shared_ptr<MockManifestInt> testDataObject1 = AZStd::make_shared<MockManifestInt>(1);
                AZStd::shared_ptr<MockManifestInt> testDataObject2 = AZStd::make_shared<MockManifestInt>(2);
                AZStd::shared_ptr<MockManifestInt> testDataObject3 = AZStd::make_shared<MockManifestInt>(3);
                testManifest.AddEntry(testDataObject1);
                testManifest.AddEntry(testDataObject2);
                testManifest.AddEntry(testDataObject3);

                bool result = testManifest.RemoveEntry(testDataObject2);
                ASSERT_TRUE(result);

                EXPECT_EQ(1, azrtti_cast<MockManifestInt*>(testManifest.GetValue(0))->GetValue());
                EXPECT_EQ(3, azrtti_cast<MockManifestInt*>(testManifest.GetValue(1))->GetValue());

                EXPECT_EQ(0, testManifest.FindIndex(testDataObject1));
                EXPECT_EQ(INVALID_INDEX, testManifest.FindIndex(testDataObject2));
                EXPECT_EQ(1, testManifest.FindIndex(testDataObject3));
            }

            // SaveToJsonObject
            TEST_F(SceneManifestTest, SaveToJsonDocument_SaveFilledManifestToString_ReturnsTrue)
            {

                auto result = m_testManifest.SaveToJsonDocument(m_serializeContext.get(), m_jsonRegistrationContext.get());
                EXPECT_TRUE(result.IsSuccess());
            }

            TEST_F(SceneManifestTest, SaveToJsonDocument_SaveEmptyManifestToString_ReturnsTrue)
            {
                MockSceneManifest empty;
                auto result = empty.SaveToJsonDocument(m_serializeContext.get(), m_jsonRegistrationContext.get());
                EXPECT_TRUE(result.IsSuccess());
            }

            // LoadFromString
            TEST_F(SceneManifestTest, LoadFromString_LoadEmptyManifestFromString_ReturnsTrue)
            {
                MockSceneManifest empty;
                auto writeToJsonResult = empty.SaveToJsonDocument(m_serializeContext.get(), m_jsonRegistrationContext.get());
                ASSERT_TRUE(writeToJsonResult.IsSuccess());

                AZStd::string jsonText;
                auto writeToStringResult = AZ::JsonSerializationUtils::WriteJsonString(writeToJsonResult.GetValue(), jsonText);
                ASSERT_TRUE(writeToStringResult.IsSuccess());

                MockSceneManifest loaded;
                auto loadFromStringResult = loaded.LoadFromString(jsonText, m_serializeContext.get(), m_jsonRegistrationContext.get());
                EXPECT_TRUE(loadFromStringResult.IsSuccess());
                EXPECT_TRUE(loaded.IsEmpty());
            }

            TEST_F(SceneManifestTest, LoadFromString_LoadFilledManifestFromString_ReturnsTrue)
            {
                auto writeToJsonResult = m_testManifest.SaveToJsonDocument(m_serializeContext.get(), m_jsonRegistrationContext.get());
                ASSERT_TRUE(writeToJsonResult.IsSuccess());

                AZStd::string jsonText;
                auto writeToStringResult = AZ::JsonSerializationUtils::WriteJsonString(writeToJsonResult.GetValue(), jsonText);
                ASSERT_TRUE(writeToStringResult.IsSuccess());

                MockSceneManifest loaded;
                auto loadFromStringResult = loaded.LoadFromString(jsonText, m_serializeContext.get(), m_jsonRegistrationContext.get());
                EXPECT_TRUE(loadFromStringResult.IsSuccess());
                EXPECT_FALSE(loaded.IsEmpty());

                ASSERT_EQ(loaded.GetEntryCount(), m_testManifest.GetEntryCount());
            }

            TEST_F(SceneManifestTest, LoadFromString_LoadFromXml_ObjectIdenticalToJsonLoadedObject)
            {
                // Write out the test Scene Manifest to XML string
                char buffer[BUFFER_SIZE];
                IO::MemoryStream xmlStream(IO::MemoryStream(buffer, sizeof(buffer), 0));
                ASSERT_TRUE(AZ::Utils::SaveObjectToStream<SceneManifest>(xmlStream, ObjectStream::ST_XML, &m_testManifest, m_serializeContext.get()));
                xmlStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                AZStd::string xmlString(buffer);

                // Deserialize XML
                MockSceneManifest xmlSceneManifest;
                AZ::Outcome<void, AZStd::string> result = xmlSceneManifest.LoadFromString(xmlString, m_serializeContext.get(), m_jsonRegistrationContext.get(), true);
                ASSERT_TRUE(result.IsSuccess());
                ASSERT_FALSE(xmlSceneManifest.IsEmpty());

                // Write out the test Scene Manifest to JSON string
                auto writeToJsonResult = m_testManifest.SaveToJsonDocument(m_serializeContext.get(), m_jsonRegistrationContext.get());
                ASSERT_TRUE(writeToJsonResult.IsSuccess());

                AZStd::string jsonText;
                auto writeToStringResult = AZ::JsonSerializationUtils::WriteJsonString(writeToJsonResult.GetValue(), jsonText);
                ASSERT_TRUE(writeToStringResult.IsSuccess());

                // Deserialize JSON
                MockSceneManifest jsonSceneManifest;
                result = jsonSceneManifest.LoadFromString(jsonText, m_serializeContext.get(), m_jsonRegistrationContext.get());
                ASSERT_TRUE(result.IsSuccess());
                ASSERT_FALSE(jsonSceneManifest.IsEmpty());

                // Ensure both deserialized Scene Manifests are identical
                ASSERT_EQ(xmlSceneManifest.GetEntryCount(), jsonSceneManifest.GetEntryCount());
            }
            
        }
    }
}
