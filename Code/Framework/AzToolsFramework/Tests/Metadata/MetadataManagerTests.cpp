/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/Metadata/MetadataManager.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Utils/Utils.h>

namespace UnitTest
{
    struct MyTestType
    {
        AZ_TYPE_INFO(MyTestType, "{48ABC814-9E03-4738-BB5A-7BE07F28BBD8}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                if (s_version == 1)
                {
                    serializeContext->Class<MyTestType>()
                        ->Version(s_version)
                        ->Field("int", &MyTestType::m_int)
                        ->Field("string", &MyTestType::m_string);
                }
                else
                {
                    serializeContext->Class<MyTestType>()
                        ->Version(s_version)
                        ->Field("float", &MyTestType::m_float)
                        ->Field("string", &MyTestType::m_string);
                }
            }
        }

        // Used to switch reflected version
        static inline int s_version = 1;

        int m_int;
        AZStd::string m_string;
        float m_float;
    };

    struct MetadataManagerTests
        : LeakDetectionFixture
        , AZ::ComponentApplicationBus::Handler
    {
        //////////////////////////////////////////////////////////////////////////
        // ComponentApplicationMessages
        AZ::ComponentApplication* GetApplication() override { return nullptr; }
        void RegisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
        void UnregisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
        void RegisterEntityAddedEventHandler(AZ::EntityAddedEvent::Handler&) override { }
        void RegisterEntityRemovedEventHandler(AZ::EntityRemovedEvent::Handler&) override { }
        void RegisterEntityActivatedEventHandler(AZ::EntityActivatedEvent::Handler&) override { }
        void RegisterEntityDeactivatedEventHandler(AZ::EntityDeactivatedEvent::Handler&) override { }
        void SignalEntityActivated(AZ::Entity*) override { }
        void SignalEntityDeactivated(AZ::Entity*) override { }
        bool AddEntity(AZ::Entity*) override { return false; }
        bool RemoveEntity(AZ::Entity*) override { return false; }
        bool DeleteEntity(const AZ::EntityId&) override { return false; }
        AZ::Entity* FindEntity(const AZ::EntityId&) override { return nullptr; }
        AZ::SerializeContext* GetSerializeContext() override { return m_serializeContext.get(); }
        AZ::BehaviorContext*  GetBehaviorContext() override { return nullptr; }
        AZ::JsonRegistrationContext* GetJsonRegistrationContext() override { return m_jsonRegistrationContext.get(); }
        const char* GetEngineRoot() const override { return nullptr; }
        const char* GetExecutableFolder() const override { return nullptr; }
        void EnumerateEntities(const EntityCallback& /*callback*/) override {}
        void QueryApplicationType(AZ::ApplicationTypeQuery& /*appType*/) const override {}
        //////////////////////////////////////////////////////////////////////////

        void SetUp() override
        {
            UnitTest::TestRunner::Instance().m_suppressPrintf = false;
            UnitTest::TestRunner::Instance().m_suppressAsserts = false;
            UnitTest::TestRunner::Instance().m_suppressErrors = false;
            UnitTest::TestRunner::Instance().m_suppressOutput = false;
            UnitTest::TestRunner::Instance().m_suppressWarnings = false;

            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();

            AZ::ComponentApplicationBus::Handler::BusConnect();

            AZ::JsonSystemComponent::Reflect(m_jsonRegistrationContext.get());

            MyTestType::Reflect(m_serializeContext.get());

            // Cache the existing file io instance and build our mock file io
            m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            m_fileIOMock = AZStd::make_unique<testing::NiceMock<AZ::IO::MockFileIOBase>>();

            // Swap out current file io instance for our mock
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_fileIOMock.get());

            // Setup the default returns for our mock file io calls
            AZ::IO::MockFileIOBase::InstallDefaultReturns(*m_fileIOMock.get());

            using namespace ::testing;
            using namespace AZ;

            ON_CALL(*m_fileIOMock, Open(_, _, _))
                .WillByDefault(Invoke(
                    [](auto filePath, auto, IO::HandleType& handle)
                    {
                        handle = AZ::u32(AZStd::hash<AZStd::string>{}( filePath ));
                        return AZ::IO::Result(AZ::IO::ResultCode::Success);
                    }));

            ON_CALL(*m_fileIOMock, Size(An<AZ::IO::HandleType>(), _))
                .WillByDefault(Invoke(
                    [this](auto handle, AZ::u64& size)
                    {
                        size = m_mockFiles[handle].size();
                        return AZ::IO::ResultCode::Success;
                    }));

            ON_CALL(*m_fileIOMock, Size(An<const char*>(), _))
                .WillByDefault(Invoke(
                    [this](const char* filePath, AZ::u64& size)
                    {
                        auto handle = AZ::u32(AZStd::hash<AZStd::string>{}(filePath));
                        size = m_mockFiles[handle].size();
                        return AZ::IO::ResultCode::Success;
                    }));

            ON_CALL(*m_fileIOMock, Exists(_))
                .WillByDefault(Invoke(
                    [this](const char* filePath)
                    {
                        auto handle = AZ::u32(AZStd::hash<AZStd::string>{}(filePath));
                        auto itr = m_mockFiles.find(handle);
                        return itr != m_mockFiles.end() && itr->second.size() > 0;
                    }));

            ON_CALL(*m_fileIOMock, Read(_, _, _, _, _))
                .WillByDefault(Invoke(
                    [this](auto handle, void* buffer, auto, auto, AZ::u64* bytesRead)
                    {
                        auto itr = m_mockFiles.find(handle);

                        if (itr == m_mockFiles.end())
                        {
                            return AZ::IO::ResultCode::Error;
                        }

                        memcpy(buffer, itr->second.c_str(), itr->second.size());
                        *bytesRead = itr->second.size();
                        return AZ::IO::ResultCode::Success;
                    }));

            ON_CALL(*m_fileIOMock, Write(_, _, _, _))
                .WillByDefault(Invoke(
                    [this](IO::HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten)
                    {
                        AZStd::string& file = m_mockFiles[fileHandle];

                        file.resize(size);
                        memcpy((void*)file.c_str(), buffer, size);
                        *bytesWritten = size;
                        return AZ::IO::ResultCode::Success;
                    }));

            m_metadata = AZ::Interface<AzToolsFramework::IMetadataRequests>::Get();

            ASSERT_TRUE(m_metadata);
        }

        void TearDown() override
        {
            AZ::ComponentApplicationBus::Handler::BusDisconnect();

            m_jsonRegistrationContext->EnableRemoveReflection();
            AZ::JsonSystemComponent::Reflect(m_jsonRegistrationContext.get());
            m_jsonRegistrationContext->DisableRemoveReflection();

            m_jsonRegistrationContext.reset();
            m_serializeContext.reset();

            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);

            UnitTest::TestRunner::Instance().ResetSuppressionSettingsToDefault();
        }

        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonRegistrationContext;
        AZStd::unique_ptr<testing::NiceMock<AZ::IO::MockFileIOBase>> m_fileIOMock;
        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZStd::unordered_map<AZ::IO::HandleType, AZStd::string> m_mockFiles;
        AzToolsFramework::MetadataManager m_manager;

        AzToolsFramework::IMetadataRequests* m_metadata = nullptr;
    };

    TEST_F(MetadataManagerTests, Get_FileDoesNotExist_ReturnsFalse)
    {
        MyTestType test;
        auto result = m_metadata->GetValue("mockfile", "/Test", &test, azrtti_typeid<MyTestType>());

        ASSERT_TRUE(result);
        EXPECT_FALSE(result.GetValue());
    }

    TEST_F(MetadataManagerTests, Get_EmptyFile_ReturnsFalse)
    {
        AZ::Utils::WriteFile(
            "", AZStd::string("mockfile") + AzToolsFramework::MetadataManager::MetadataFileExtension);

        MyTestType test;

        auto result = m_metadata->GetValue("mockfile", "/Test", &test, azrtti_typeid<MyTestType>());
        ASSERT_TRUE(result);
        EXPECT_FALSE(result.GetValue());
    }

    TEST_F(MetadataManagerTests, Get_InvalidFile_ReturnsFalse)
    {
        AZ::Utils::WriteFile("This is not a metadata file", AZStd::string("mockfile") + AzToolsFramework::MetadataManager::MetadataFileExtension);

        MyTestType test;
        EXPECT_FALSE(m_metadata->GetValue("mockfile", "/Test", &test, azrtti_typeid<MyTestType>()));
    }

    TEST_F(MetadataManagerTests, Get_InvalidKey_ReturnsFalse)
    {
        MyTestType test;
        EXPECT_FALSE(m_metadata->GetValue("mockfile", "Test", &test, azrtti_typeid<MyTestType>()));
    }

    TEST_F(MetadataManagerTests, Set_ReturnsTrue)
    {
        MyTestType test;
        EXPECT_TRUE(m_metadata->SetValue("mockfile", "/Test", &test, azrtti_typeid<MyTestType>()));
    }

    TEST_F(MetadataManagerTests, Set_InvalidKey_ReturnsFalse)
    {
        MyTestType test;
        EXPECT_FALSE(m_metadata->SetValue("mockfile", "Test", &test, azrtti_typeid<MyTestType>()));
    }

    TEST_F(MetadataManagerTests, SetGet_ReadsValueCorrectly)
    {
        MyTestType outValue;
        MyTestType inValue;
        outValue.m_int = 23;
        outValue.m_string = "Hello World";

        EXPECT_TRUE(m_metadata->SetValue("mockfile", "/Test", &outValue, azrtti_typeid<MyTestType>()));
        EXPECT_TRUE(m_metadata->GetValue("mockfile", "/Test", &inValue, azrtti_typeid<MyTestType>()));

        EXPECT_EQ(outValue.m_int, inValue.m_int);
        EXPECT_EQ(outValue.m_string, inValue.m_string);
    }

    TEST_F(MetadataManagerTests, Get_FileExists_KeyDoesNotExist_ReturnsFalse)
    {
        MyTestType test;
        EXPECT_TRUE(m_metadata->SetValue("mockfile", "/Test", test));
        EXPECT_FALSE(m_metadata->GetValue("mockfile", "/DoesNotExist", test));
    }

    TEST_F(MetadataManagerTests, Get_FileVersion_ReturnsTrue)
    {
        MyTestType test;
        EXPECT_TRUE(m_metadata->SetValue("mockfile", "/Test", &test, azrtti_typeid<MyTestType>()));

        int version = 0;
        EXPECT_TRUE(m_metadata->GetValue("mockfile", AzToolsFramework::MetadataManager::MetadataVersionKey, &version, azrtti_typeid<int>()));
        EXPECT_EQ(version, AzToolsFramework::MetadataManager::MetadataVersion);
    }

    TEST_F(MetadataManagerTests, Set_InvalidMetadataFile_ReturnsFalse)
    {
        AZ::Utils::WriteFile("This is not a metadata file", AZStd::string("mockfile") + AzToolsFramework::MetadataManager::MetadataFileExtension);

        MyTestType test;
        EXPECT_FALSE(m_metadata->SetValue("mockfile", "/Test", &test, azrtti_typeid<MyTestType>()));
    }

    TEST_F(MetadataManagerTests, Get_OldVersion)
    {
        MyTestType test;
        test.m_int = 23;
        test.m_string = "Hello World";
        EXPECT_TRUE(m_metadata->SetValue("mockfile", "/Test", test));

        // Unregister the existing type
        m_serializeContext->EnableRemoveReflection();
        MyTestType::Reflect(m_serializeContext.get());
        m_serializeContext->DisableRemoveReflection();

        // "Upgrade" the type
        MyTestType::s_version = 2;
        MyTestType::Reflect(m_serializeContext.get());

        // Now try to read the old value
        int version;
        EXPECT_TRUE(m_metadata->GetValueVersion("mockfile", "/Test", version));
        EXPECT_EQ(version, 1);

        rapidjson::Document inValue;
        EXPECT_TRUE(m_metadata->GetJson("mockfile", "/Test", inValue));

        auto intItr = inValue.FindMember("int");
        auto stringItr = inValue.FindMember("string");

        EXPECT_NE(intItr, inValue.MemberEnd());
        EXPECT_NE(stringItr, inValue.MemberEnd());

        EXPECT_EQ(intItr->value.GetInt(), test.m_int);
        EXPECT_EQ(stringItr->value.GetString(), test.m_string);
    }
}
