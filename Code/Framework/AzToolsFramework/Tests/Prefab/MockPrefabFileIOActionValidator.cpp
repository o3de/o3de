/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/MockPrefabFileIOActionValidator.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/JSON/prettywriter.h>
#include <Prefab/PrefabLoaderInterface.h>

namespace UnitTest
{
    MockPrefabFileIOActionValidator::MockPrefabFileIOActionValidator()
    {
        // Cache the existing file io instance and build our mock file io
        m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
        m_fileIOMock = AZStd::make_unique<testing::NiceMock<AZ::IO::MockFileIOBase>>();

        // Swap out current file io instance for our mock
        AZ::IO::FileIOBase::SetInstance(nullptr);
        AZ::IO::FileIOBase::SetInstance(m_fileIOMock.get());

        // Setup the default returns for our mock file io calls
        AZ::IO::MockFileIOBase::InstallDefaultReturns(*m_fileIOMock.get());
    }

    MockPrefabFileIOActionValidator::~MockPrefabFileIOActionValidator()
    {
        // Restore our original file io instance
        AZ::IO::FileIOBase::SetInstance(nullptr);
        AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
    }

    void MockPrefabFileIOActionValidator::ReadPrefabDom(
        AZ::IO::PathView prefabFilePath,
        const AzToolsFramework::Prefab::PrefabDom& prefabFileContentDom,
        AZ::IO::ResultCode expectedReadResultCode,
        AZ::IO::ResultCode expectedOpenResultCode,
        AZ::IO::ResultCode expectedSizeResultCode,
        AZ::IO::ResultCode expectedCloseResultCode)
    {
        rapidjson::StringBuffer prefabFileContentBuffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(prefabFileContentBuffer);
        prefabFileContentDom.Accept(writer);

        AZStd::string prefabFileContent(prefabFileContentBuffer.GetString());

        ReadPrefabDom(prefabFilePath, prefabFileContent,
            expectedReadResultCode, expectedOpenResultCode, expectedSizeResultCode, expectedCloseResultCode);
    }

    void MockPrefabFileIOActionValidator::ReadPrefabDom(
        AZ::IO::PathView prefabFilePath,
        const AZStd::string& prefabFileContent,
        AZ::IO::ResultCode expectedReadResultCode,
        AZ::IO::ResultCode expectedOpenResultCode,
        AZ::IO::ResultCode expectedSizeResultCode,
        AZ::IO::ResultCode expectedCloseResultCode)
    {
        AZ::IO::HandleType fileHandle = m_fileHandleCounter++;

        AZ::IO::Path prefabFullPath = AZ::Interface<AzToolsFramework::Prefab::PrefabLoaderInterface>::Get()->GetFullPath(AZStd::string_view(prefabFilePath));

        EXPECT_CALL(*m_fileIOMock.get(), Open(
            testing::StrEq(prefabFullPath.c_str()), testing::_, testing::_))
            .WillRepeatedly(
                testing::DoAll(
                    testing::SetArgReferee<2>(fileHandle),
                    testing::Return(AZ::IO::Result(expectedOpenResultCode))));

        EXPECT_CALL(*m_fileIOMock.get(), Size(fileHandle, testing::_))
            .WillRepeatedly(
                testing::DoAll(
                    testing::SetArgReferee<1>(prefabFileContent.size()),
                    testing::Return(AZ::IO::Result(expectedSizeResultCode))));

        EXPECT_CALL(*m_fileIOMock.get(), Read(fileHandle, testing::_, prefabFileContent.size(), testing::_, testing::_))
            .WillRepeatedly(testing::Invoke([prefabFileContent, expectedReadResultCode](AZ::IO::HandleType, void* buffer, AZ::u64, bool, AZ::u64* bytesRead)
                {
                    memcpy(buffer, prefabFileContent.data(), prefabFileContent.size());
                    *bytesRead = prefabFileContent.size();

                    return AZ::IO::Result(expectedReadResultCode);
                }));

        EXPECT_CALL(*m_fileIOMock.get(), Close(fileHandle))
            .WillRepeatedly(testing::Return(AZ::IO::Result(expectedCloseResultCode)));
    }
}
