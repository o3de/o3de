/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/base.h>
#include <AzCore/Memory/AllocatorScope.h>

#include <AudioControlsLoader.h>
#include <ATLControlsModel.h>

#include <platform.h>
#include <ISystem.h>
#include <Mocks/ICryPakMock.h>

using ::testing::NiceMock;
using namespace AudioControls;

namespace CustomMocks
{
    class AudioControlsEditorTest_CryPakMock
        : public CryPakMock
    {
    public:
        AZ_TEST_CLASS_ALLOCATOR(AudioControlsEditorTest_CryPakMock)

        AudioControlsEditorTest_CryPakMock(const char* levelName)
            : m_levelName(levelName)
        {}

        AZ::IO::ArchiveFileIterator FindFirst([[maybe_unused]] AZStd::string_view dir, AZ::IO::IArchive::EFileSearchType) override
        {
            AZ::IO::FileDesc fileDesc;
            fileDesc.nSize = sizeof(AZ::IO::FileDesc);
            // Add a filename and file description reference to the TestFindData map to make sure the file iterator is valid
            m_findData = new TestFindData();
            m_findData->m_fileStack.emplace_back(AZ::IO::ArchiveFileIterator{ static_cast<AZ::IO::FindData*>(m_findData.get()), m_levelName, fileDesc });
            return m_findData->Fetch();
        }

        AZ::IO::ArchiveFileIterator FindNext(AZ::IO::ArchiveFileIterator iter) override
        {
            return ++iter;
        }

        // public: for easy resetting...
        AZStd::string m_levelName;

        // Add an inherited FindData class to control the adding of a mapfile which indicates that a FileIterator is valid
        struct TestFindData
            : AZ::IO::FindData
        {
            using AZ::IO::FindData::m_fileStack;
        };

        AZStd::intrusive_ptr<TestFindData> m_findData;
    };

} // namespace CustomMocks


class AudioControlsEditorTestEnvironment
    : public AZ::Test::ITestEnvironment
{
public:
    AZ_TEST_CLASS_ALLOCATOR(AudioControlsEditorTestEnvironment)

    ~AudioControlsEditorTestEnvironment() override = default;

protected:
    void SetupEnvironment() override
    {
        m_allocatorScope.ActivateAllocators();

        m_stubEnv.pCryPak = nullptr;
        m_stubEnv.pFileIO = nullptr;
        gEnv = &m_stubEnv;
    }

    void TeardownEnvironment() override
    {
        m_allocatorScope.DeactivateAllocators();
    }

private:
    AZ::AllocatorScope<AZ::OSAllocator, AZ::SystemAllocator, AZ::LegacyAllocator, CryStringAllocator> m_allocatorScope;
    SSystemGlobalEnvironment m_stubEnv;
};

AZ_UNIT_TEST_HOOK(new AudioControlsEditorTestEnvironment);

TEST(AudioControlsEditorTest, AudioControlsLoader_LoadScopes_ScopesAreAdded)
{
    ASSERT_TRUE(gEnv != nullptr);
    ASSERT_TRUE(gEnv->pCryPak == nullptr);

    NiceMock<CustomMocks::AudioControlsEditorTest_CryPakMock> m_cryPakMock("ly_extension.ly");
    gEnv->pCryPak = &m_cryPakMock;

    CATLControlsModel atlModel;
    CAudioControlsLoader loader(&atlModel, nullptr, nullptr);

    loader.LoadScopes();
    EXPECT_TRUE(atlModel.ScopeExists("ly_extension"));

    m_cryPakMock.m_levelName = "cry_extension.cry";
    loader.LoadScopes();
    EXPECT_TRUE(atlModel.ScopeExists("cry_extension"));

    atlModel.ClearScopes();
    gEnv->pCryPak = nullptr;
}
