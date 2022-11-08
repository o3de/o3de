/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/IO/LocalFileIO.h>

#include <Framework/JsonObjectHandler.h>
#include <Framework/JsonWriter.h>

#include <AWSNativeSDKTestManager.h>

namespace AWSCoreTestingUtils
{
    static const AZStd::string STRING_VALUE{"s"};
    static constexpr const char STRING_VALUE_STRING[] = "s";
    static constexpr const char STRING_VALUE_JSON[] = "\"s\"";

    static constexpr const char CHARPTR_VALUE[]{"s"};
    static constexpr const char CHARPTR_VALUE_STRING[] = "s";
    static constexpr const char CHARPTR_VALUE_JSON[] = "\"s\"";

    static constexpr bool BOOL_VALUE{true};
    static constexpr const char BOOL_VALUE_STRING[] = "true";

    static constexpr int INT_VALUE{-2};
    static constexpr const char INT_VALUE_STRING[] = "-2";

    static constexpr unsigned UINT_VALUE{2};
    static constexpr const char UINT_VALUE_STRING[] = "2";

    static constexpr unsigned UINT_VALUE_MAX{4294967295};
    static constexpr const char UINT_VALUE_MAX_STRING[] = "4294967295";

    static constexpr int64_t INT64_VALUE{INT64_C(-3000000000)}; // < INT_MIN
    static constexpr const char INT64_VALUE_STRING[] = "-3000000000";

    static constexpr uint64_t UINT64_VALUE{UINT64_C(5000000000)}; // > UINT_MAX and < INT64_MAX
    static constexpr const char UINT64_VALUE_STRING[] = "5000000000";

    static constexpr uint64_t UINT64_VALUE_MAX{18446744073709551615U};
    static constexpr const char UINT64_VALUE_MAX_STRING[] = "18446744073709551615";

    static constexpr double DOUBLE_VALUE{1.0};
    static constexpr const char DOUBLE_VALUE_STRING[] = "1.0";

    static constexpr const char OBJECT_VALUE_JSON[] = "{\"value\":\"s\"}";

    static constexpr const char ARRAY_VALUE_JSON[] = "[\"a\",\"b\",\"c\"]";

    static constexpr const char ARRAY_OF_ARRAY_VALUE_JSON[] = "[[\"a1\",\"b1\",\"c1\"],[\"a2\",\"b2\",\"c2\"]]";

    static constexpr const char ARRAY_OF_OBJECT_VALUE_JSON[] = "[{\"value\":\"s1\"},{\"value\":\"s2\"}]";

    static constexpr const char UNESCAPED[] = "abc !#$%&'()*+,/:;=?@[]123";
    static constexpr const char ESCAPED[] = "abc%20%21%23%24%25%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D123";

    inline AZStd::string TestObjectJson(const char* value)
    {
        return AZStd::string::format("{\"value\":%s}", value);
    }
}; // namespace AWSCoreTestingUtils

template<class T>
struct TestObject
{
    TestObject() = default;

    TestObject(const T& initialValue)
        : value{initialValue}
    {
    }

    T value;

    bool OnJsonKey(const char* key, AWSCore::JsonReader& reader)
    {
        if (strcmp(key, "value") == 0)
        {
            return reader.Accept(value);
        }
        return reader.Ignore();
    }

    bool WriteJson(AWSCore::JsonWriter& writer) const
    {
        bool ok = true;
        ok = ok && writer.StartObject();
        ok = ok && writer.Write("value", value);
        ok = ok && writer.EndObject();
        return ok;
    }

    bool operator==(const TestObject& other) const
    {
        return value == other.value;
    }
};

class AWSCoreFixture
    : public UnitTest::ScopedAllocatorSetupFixture
{
public:
    AWSCoreFixture() = default;
    ~AWSCoreFixture() override = default;

    void SetUp() override
    {
        SetUpFixture();
    }

    void SetUpFixture(bool mockSettingsRegistry = true)
    {
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();
        AZ::AllocatorInstance<AZ::PoolAllocator>::Create();

        m_localFileIO = aznew AZ::IO::LocalFileIO();
        m_otherFileIO = AZ::IO::FileIOBase::GetInstance();
        AZ::IO::FileIOBase::SetInstance(nullptr);
        AZ::IO::FileIOBase::SetInstance(m_localFileIO);

        if (mockSettingsRegistry)
        {
            m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            AZ::SettingsRegistry::Register(m_settingsRegistry.get());
        }
        else
        {
            m_app = AZStd::make_unique<AZ::ComponentApplication>();
        }

        // Add AWSCore as an active gem for unit test
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            auto projectPathKey =
                AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
            AZ::IO::FixedMaxPath enginePath;
            settingsRegistry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
            settingsRegistry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*settingsRegistry);

            // Merge in the o3de manifest files gem root paths to the Settings Registry
            // and set the AWSCore gem as an active gem which will add it to the active gem
            // section of the Settings Registry as well as add a @gemroot@ alias for it
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ManifestGemsPaths(*settingsRegistry);
            AZ::Test::AddActiveGem("AWSCore", *settingsRegistry, m_localFileIO);
        }

        AWSNativeSDKTestLibs::AWSNativeSDKTestManager::Init();
    }

    void TearDown() override
    {
        TearDownFixture();
    }

    void TearDownFixture(bool mockSettingsRegistry = true) 
    {
        AWSNativeSDKTestLibs::AWSNativeSDKTestManager::Shutdown();

        if (mockSettingsRegistry)
        {
            AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());
            m_settingsRegistry.reset();
        }
        else
        {
            m_app.reset();
        }

        AZ::IO::FileIOBase::SetInstance(nullptr);

        delete m_localFileIO;
        m_localFileIO = nullptr;

        if (m_otherFileIO)
        {
            AZ::IO::FileIOBase::SetInstance(m_otherFileIO);
        }

        AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
    }

    bool CreateFile(const AZStd::string& filePath, const AZStd::string& content)
    {
        AZ::IO::HandleType fileHandle;
        if (!m_localFileIO->Open(filePath.c_str(), AZ::IO::OpenMode::ModeCreatePath | AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText, fileHandle))
        {
            return false;
        }

        m_localFileIO->Write(fileHandle, content.c_str(), content.size());
        m_localFileIO->Close(fileHandle);
        return true;
    }

    bool RemoveFile(const AZStd::string& filePath)
    {
        if (m_localFileIO->Exists(filePath.c_str()))
        {
            return m_localFileIO->Remove(filePath.c_str());
        }

        return true;
    }

    AZ::IO::FileIOBase* m_localFileIO = nullptr;

private:
    AZ::IO::FileIOBase* m_otherFileIO = nullptr;

protected:
    AZ::IO::Path GetTestTempDirectoryPath()
    {
        AZ::IO::Path testTempDirPath{ m_testTempDirectory.GetDirectory() };
        return testTempDirPath;
    }

    AZ::Test::ScopedAutoTempDirectory m_testTempDirectory;
    AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_settingsRegistry;
    AZStd::unique_ptr<AZ::ComponentApplication> m_app;
};
