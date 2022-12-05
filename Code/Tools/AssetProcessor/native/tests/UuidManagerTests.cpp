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
#include <native/utilities/UuidManager.h>
#include <AssetManager/SourceAssetReference.h>
#include <tests/UnitTestUtilities.h>

namespace UnitTests
{
    struct UuidManagerTests
        : UnitTest::LeakDetectionFixture
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

            AssetProcessor::UuidManager::Reflect(m_serializeContext.get());

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
                        handle = AZ::u32(AZStd::hash<AZStd::string>{}(filePath));
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

            ON_CALL(*m_fileIOMock, Rename(_, _))
                .WillByDefault(Invoke(
                    [this](const char* originalPath, const char* newPath)
                    {
                        auto originalHandle = AZ::u32(AZStd::hash<AZStd::string>{}(originalPath));
                        auto newHandle = AZ::u32(AZStd::hash<AZStd::string>{}(newPath));
                        auto itr = m_mockFiles.find(originalHandle);

                        if (itr != m_mockFiles.end())
                        {
                            m_mockFiles[newHandle] = itr->second;
                            m_mockFiles.erase(itr);

                            return AZ::IO::ResultCode::Success;
                        }

                        return AZ::IO::ResultCode::Error;
                    }));

            ON_CALL(*m_fileIOMock, Remove(_))
                .WillByDefault(Invoke(
                    [this](const char* path)
                    {
                        auto handle = AZ::u32(AZStd::hash<AZStd::string>{}(path));

                        m_mockFiles.erase(handle);

                        return AZ::IO::ResultCode::Success;
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

            m_uuidInterface = AZ::Interface<AssetProcessor::IUuidRequests>::Get();

            ASSERT_TRUE(m_uuidInterface);

            // Enable txt files by default for these tests
            m_uuidInterface->EnableGenerationForTypes({ ".txt" });
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
        }

        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonRegistrationContext;
        AZStd::unique_ptr<testing::NiceMock<AZ::IO::MockFileIOBase>> m_fileIOMock;
        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZStd::unordered_map<AZ::IO::HandleType, AZStd::string> m_mockFiles;
        AzToolsFramework::MetadataManager m_metadataManager;
        AssetProcessor::UuidManager m_uuidManager;
        MockPathConversion m_pathConversion;

        AssetProcessor::IUuidRequests* m_uuidInterface{};
    };

    TEST_F(UuidManagerTests, GetUuid_FirstTime_ReturnsRandomUuid)
    {
        static constexpr const char* TestFile = "c:/somepath/mockfile.txt";
        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        EXPECT_FALSE(uuid.IsNull());
        // Make sure a metadata file was created
        EXPECT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(AZStd::string::format("%s%s", TestFile, AzToolsFramework::MetadataManager::MetadataFileExtension).c_str()));
    }

    TEST_F(UuidManagerTests, GetUuidTwice_ReturnsSameUuid)
    {
        static constexpr const char* TestFile = "c:/somepath/Mockfile.txt";

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        EXPECT_FALSE(uuid.IsNull());

        auto uuid2 = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        EXPECT_EQ(uuid, uuid2);
    }

    TEST_F(UuidManagerTests, GetUuid_DifferentFiles_ReturnsDifferentUuid)
    {
        static constexpr const char* FileA = "c:/somepath/fileA.txt";
        static constexpr const char* FileB = "c:/somepath/fileB.txt";

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileA));

        EXPECT_FALSE(uuid.IsNull());

        auto uuid2 = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileB));

        EXPECT_NE(uuid, uuid2);
    }

    TEST_F(UuidManagerTests, GetLegacyUuids_UppercaseFileName_ReturnsTwoDifferentUuids)
    {
        auto uuids = m_uuidInterface->GetLegacyUuids(AssetProcessor::SourceAssetReference("c:/somepath/Mockfile.txt"));

        ASSERT_EQ(uuids.size(), 2);
        EXPECT_NE(*uuids.begin(), *++uuids.begin());
    }

    TEST_F(UuidManagerTests, GetLegacyUuids_LowercaseFileName_ReturnsOneUuid)
    {
        auto uuids = m_uuidInterface->GetLegacyUuids(AssetProcessor::SourceAssetReference("c:/somepath/mockfile.txt"));

        EXPECT_EQ(uuids.size(), 1);
    }

    TEST_F(UuidManagerTests, GetLegacyUuids_DifferentFromCanonicalUuid)
    {
        static constexpr const char* TestFile = "c:/somepath/Mockfile.txt";

        auto legacyUuids = m_uuidInterface->GetLegacyUuids(AssetProcessor::SourceAssetReference(TestFile));

        ASSERT_EQ(legacyUuids.size(), 2);

        auto canonicalUuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        EXPECT_THAT(legacyUuids, ::testing::Not(::testing::Contains(canonicalUuid)));
    }

    TEST_F(UuidManagerTests, MoveFile_UuidRemainsTheSame)
    {
        static constexpr const char* FileA = "c:/somepath/mockfile.txt";
        static constexpr const char* FileB = "c:/somepath/newfile.txt";

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileA));

        AZ::IO::FileIOBase::GetInstance()->Rename(
            (AZStd::string(FileA) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str(),
            (AZStd::string(FileB) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        m_uuidInterface->FileRemoved((AZStd::string(FileA) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        auto movedUuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileB));

        EXPECT_EQ(uuid, movedUuid);
    }

    TEST_F(UuidManagerTests, MoveFileWithComplexName_UuidRemainsTheSame)
    {
        static constexpr const char* FileA = "c:/somepath/mockfile.ext1.ext2.txt";
        static constexpr const char* FileB = "c:/somepath/newfile.txt";

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileA));

        AZ::IO::FileIOBase::GetInstance()->Rename(
            (AZStd::string(FileA) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str(),
            (AZStd::string(FileB) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        m_uuidInterface->FileRemoved((AZStd::string(FileA) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        auto movedUuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileB));

        EXPECT_EQ(uuid, movedUuid);
    }

    TEST_F(UuidManagerTests, MetadataRemoved_NewUuidAssigned)
    {
        static constexpr const char* TestFile = "c:/somepath/mockfile.txt";

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        AZ::IO::FileIOBase::GetInstance()->Remove(
            (AZStd::string(TestFile) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        m_uuidInterface->FileRemoved((AZStd::string(TestFile) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        auto movedUuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));

        EXPECT_NE(uuid, movedUuid);
    }

    TEST_F(UuidManagerTests, MetadataUpdated_NewUuidAssigned)
    {
        static constexpr const char* FileA = "c:/somepath/mockfile.test.txt";
        static constexpr const char* FileB = "c:/somepath/someotherfile.txt";

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileA));

        // Generate another metadata file, its the easiest way to "change" a UUID in the metadata file for this test
        m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileB));

        // Copy FileB's metadata onto the FileA metadata
        AZ::IO::FileIOBase::GetInstance()->Rename(
            (AZStd::string(FileB) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str(),
            (AZStd::string(FileA) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        m_uuidInterface->FileChanged((AZStd::string(FileA) + AzToolsFramework::MetadataManager::MetadataFileExtension).c_str());

        auto newUuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(FileA));

        EXPECT_NE(uuid, newUuid);
    }

    TEST_F(UuidManagerTests, RequestUuid_DisabledType_ReturnsLegacyUuid)
    {
        static constexpr const char* TestFile = "c:/somepath/mockfile.png";

        auto uuid = m_uuidInterface->GetUuid(AssetProcessor::SourceAssetReference(TestFile));
        auto legacyUuids = m_uuidInterface->GetLegacyUuids(AssetProcessor::SourceAssetReference(TestFile));

        EXPECT_THAT(legacyUuids, ::testing::Contains(uuid));

        // Make sure no metadata file was created
        EXPECT_FALSE(AZ::IO::FileIOBase::GetInstance()->Exists(
            AZStd::string::format("%s%s", TestFile, AzToolsFramework::MetadataManager::MetadataFileExtension).c_str()));
    }
}
