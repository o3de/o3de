/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/base.h>

#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AudioControlsLoader.h>
#include <ATLControlsModel.h>

using ::testing::NiceMock;
using namespace AudioControls;

namespace CustomMocks
{
    class AudioControlsEditorTest_FileIOMock
        : public AZ::IO::MockFileIOBase
    {
    public:

        AudioControlsEditorTest_FileIOMock()
        {
        }

        bool IsDirectory([[maybe_unused]] const char* path) override
        {
            return false;
        }

        AZ::IO::Result FindFiles(
            [[maybe_unused]] const char* path,
            [[maybe_unused]] const char* filter,
            AZ::IO::FileIOBase::FindFilesCallbackType callback) override
        {
            if (callback)
            {
                callback(m_levelName.c_str());
                return AZ::IO::ResultCode::Success;
            }
            return AZ::IO::ResultCode::Error;
        }

        AZStd::string m_levelName;
    };

} // namespace CustomMocks


class AudioControlsEditorTestEnvironment
    : public AZ::Test::ITestEnvironment
{
public:
    ~AudioControlsEditorTestEnvironment() override = default;

protected:
    void SetupEnvironment() override
    {
    }

    void TeardownEnvironment() override
    {
    }

private:
};

AZ_UNIT_TEST_HOOK(new AudioControlsEditorTestEnvironment);

class AudioControlsEditorTest
    : public UnitTest::LeakDetectionFixture
{
public:
    void SetUp() override
    {
        UnitTest::LeakDetectionFixture::SetUp();

        // Store and remove the existing fileIO...
        m_prevFileIO = AZ::IO::FileIOBase::GetInstance();
        if (m_prevFileIO)
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
        }

        // Replace with a new FileIO Mock...
        m_fileIO = AZStd::make_unique<CustomMocks::AudioControlsEditorTest_FileIOMock>();
        AZ::IO::FileIOBase::SetInstance(m_fileIO.get());
    }

    void TearDown() override
    {
        // Destroy our LocalFileIO...
        m_fileIO.reset();

        // Replace the old fileIO (set instance to null first)...
        AZ::IO::FileIOBase::SetInstance(nullptr);
        if (m_prevFileIO)
        {
            AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
            m_prevFileIO = nullptr;
        }
        UnitTest::LeakDetectionFixture::TearDown();
    }

protected:
    AZ::IO::FileIOBase* m_prevFileIO = nullptr;
    AZStd::unique_ptr<CustomMocks::AudioControlsEditorTest_FileIOMock> m_fileIO;
};

TEST_F(AudioControlsEditorTest, AudioControlsLoader_LoadScopes_ScopesAreAdded)
{
    CATLControlsModel atlModel;
    CAudioControlsLoader loader(&atlModel, nullptr, nullptr);

    m_fileIO->m_levelName = "ly_extension.ly";
    loader.LoadScopes();
    EXPECT_TRUE(atlModel.ScopeExists("ly_extension"));

    m_fileIO->m_levelName = "cry_extension.cry";
    loader.LoadScopes();
    EXPECT_TRUE(atlModel.ScopeExists("cry_extension"));

    m_fileIO->m_levelName = "prefab_extension.prefab";
    loader.LoadScopes();
    EXPECT_TRUE(atlModel.ScopeExists("prefab_extension"));

    m_fileIO->m_levelName = "spawnable_extension.spawnable";
    loader.LoadScopes();
    EXPECT_FALSE(atlModel.ScopeExists("spawnable_extension"));

    atlModel.ClearScopes();
}
