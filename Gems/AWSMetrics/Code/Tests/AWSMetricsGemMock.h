/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AWSMetrics
{
    class AWSMetricsGemAllocatorFixture
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    protected:
        void SetUp() override
        {
            UnitTest::ScopedAllocatorSetupFixture::SetUp();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();

            // Set up the file IO and alias
            m_localFileIO = aznew AZ::IO::LocalFileIO();
            m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            // we need to set it to nullptr first because otherwise the 
            // underneath code assumes that we might be leaking the previous instance
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_localFileIO);

            const AZStd::string engineRoot = AZ::Test::GetEngineRootPath();
            m_localFileIO->SetAlias("@devroot@", engineRoot.c_str());
            m_localFileIO->SetAlias("@root@", engineRoot.c_str());
            m_localFileIO->SetAlias("@user@", GetTestFolderPath().c_str());

            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_registrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();

            AZ::JsonSystemComponent::Reflect(m_registrationContext.get());

            m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();

            m_settingsRegistry->SetContext(m_serializeContext.get());
            m_settingsRegistry->SetContext(m_registrationContext.get());

            AZ::SettingsRegistry::Register(m_settingsRegistry.get());
        }

        void TearDown() override
        {
            AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());

            m_registrationContext->EnableRemoveReflection();
            AZ::JsonSystemComponent::Reflect(m_registrationContext.get());
            m_registrationContext->DisableRemoveReflection();

            m_settingsRegistry.reset();
            m_serializeContext.reset();
            m_registrationContext.reset();

            AZ::IO::FileIOBase::SetInstance(nullptr);
            delete m_localFileIO;
            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);

            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            UnitTest::ScopedAllocatorSetupFixture::TearDown();
        }

        AZStd::string CreateClientConfigFile(bool offlineRecordingEnabled, double maxQueueSizeInMb, int queueFlushPeriodInSeconds, int MaxNumRetries)
        {
            AZStd::string offlineRecordingEnabledStr = offlineRecordingEnabled ? "true" : "false";
            AZStd::string configFilePath = GetDefaultTestFilePath();

            AZStd::string settings = AZStd::string::format("{\"Amazon\":{\"Gems\":{\"AWSMetrics\":{\"OfflineRecording\":%s,\"MaxQueueSizeInMb\":%f,\"QueueFlushPeriodInSeconds\":%d,\"MaxNumRetries\":%d}}}}",
                offlineRecordingEnabledStr.c_str(),
                maxQueueSizeInMb,
                queueFlushPeriodInSeconds,
                MaxNumRetries);

            CreateFile(configFilePath, settings);

            return configFilePath;
        }

        bool CreateFile(const AZStd::string& filePath, const AZStd::string& content)
        {
            AZ::IO::HandleType fileHandle;
            if (!m_localFileIO->Open(filePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText, fileHandle))
            {
                return false;
            }

            m_localFileIO->Write(fileHandle, content.c_str(), content.size());
            m_localFileIO->Close(fileHandle);
            return true;
        }

        AZStd::string GetDefaultTestFilePath()
        {
            AZStd::string testFilePath = GetTestFolderPath();
            AzFramework::StringFunc::Path::Join(testFilePath.c_str(), "Test.json", testFilePath);

            return testFilePath;
        }

        bool RemoveFile(const AZStd::string& filePath)
        {
            if (m_localFileIO->Exists(filePath.c_str()))
            {
                return m_localFileIO->Remove(filePath.c_str());
            }

            return true;
        }

        bool RemoveDirectory(const AZStd::string& directory)
        {
            return AZ::IO::SystemFile::DeleteDir(directory.c_str());
        }

        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZ::IO::FileIOBase* m_localFileIO = nullptr;

        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_registrationContext;
        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_settingsRegistry;

    private:
        AZStd::string GetTestFolderPath()
        {
            return AZ_TRAIT_TEST_ROOT_FOLDER;
        }
    };
}
