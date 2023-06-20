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
#include <AzToolsFramework/Metadata/UuidUtils.h>
#include <AzCore/UnitTest/MockComponentApplication.h>

namespace UnitTest
{
    struct UuidUtilTests
        : LeakDetectionFixture
    {
        void SetUp() override
        {
            UnitTest::TestRunner::Instance().m_suppressPrintf = false;
            UnitTest::TestRunner::Instance().m_suppressAsserts = true;
            UnitTest::TestRunner::Instance().m_suppressErrors = true;
            UnitTest::TestRunner::Instance().m_suppressOutput = false;
            UnitTest::TestRunner::Instance().m_suppressWarnings = false;

            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();

            m_applicationMock = AZStd::make_unique<testing::NiceMock<UnitTest::MockComponentApplication>>();

            ON_CALL(*m_applicationMock, GetSerializeContext())
                .WillByDefault(
                    [this]()
                    {
                        return m_serializeContext.get();
                    });

            ON_CALL(*m_applicationMock, GetJsonRegistrationContext())
                .WillByDefault(
                    [this]()
                    {
                        return m_jsonRegistrationContext.get();
                    });

            AZ::JsonSystemComponent::Reflect(m_jsonRegistrationContext.get());

            AzToolsFramework::UuidUtilComponent::Reflect(m_serializeContext.get());
            AzToolsFramework::MetadataManager::Reflect(m_serializeContext.get());

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

            m_utilInterface = AZ::Interface<AzToolsFramework::IUuidUtil>::Get();

            ASSERT_TRUE(m_utilInterface);
        }

        void TearDown() override
        {
            m_jsonRegistrationContext->EnableRemoveReflection();
            AZ::JsonSystemComponent::Reflect(m_jsonRegistrationContext.get());
            m_jsonRegistrationContext->DisableRemoveReflection();

            m_jsonRegistrationContext.reset();
            m_serializeContext.reset();

            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);

            UnitTest::TestRunner::Instance().ResetSuppressionSettingsToDefault();
        }

        AZStd::unique_ptr<testing::NiceMock<UnitTest::MockComponentApplication>> m_applicationMock;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonRegistrationContext;
        AZStd::unique_ptr<testing::NiceMock<AZ::IO::MockFileIOBase>> m_fileIOMock;
        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZStd::unordered_map<AZ::IO::HandleType, AZStd::string> m_mockFiles;
        AzToolsFramework::MetadataManager m_manager;
        AzToolsFramework::UuidUtilComponent m_uuidUtil;

        // Cached pointer, no need to clean up
        AzToolsFramework::IUuidUtil* m_utilInterface{};
    };

    TEST_F(UuidUtilTests, CreateSourceUuid_Random_ReturnsRandomUuid)
    {
        auto result = m_utilInterface->CreateSourceUuid("mockfile");

        ASSERT_TRUE(result);
        EXPECT_FALSE(result.GetValue().IsNull());
    }

    TEST_F(UuidUtilTests, CreateSourceUuid_SpecifyUuid_ReturnsTrue)
    {
        auto uuid = AZ::Uuid::CreateRandom();
        EXPECT_TRUE(m_utilInterface->CreateSourceUuid("mockfile", uuid));
    }

    TEST_F(UuidUtilTests, CreateSourceUuidRandom_AlreadyAssigned_Fails)
    {
        m_utilInterface->CreateSourceUuid("mockfile");
        auto result = m_utilInterface->CreateSourceUuid("mockfile");

        EXPECT_FALSE(result);
    }

    TEST_F(UuidUtilTests, CreateSourceUuid_AlreadyAssigned_Fails)
    {
        auto uuid = AZ::Uuid::CreateRandom();
        m_utilInterface->CreateSourceUuid("mockfile");

        EXPECT_FALSE(m_utilInterface->CreateSourceUuid("mockfile", uuid));
    }
}
