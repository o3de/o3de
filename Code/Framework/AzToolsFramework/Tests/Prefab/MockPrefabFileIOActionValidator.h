/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/std/functional.h>
#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace UnitTest
{
    class MockPrefabFileIOActionValidator
    {
    public:
        MockPrefabFileIOActionValidator();
        ~MockPrefabFileIOActionValidator();

        void ReadPrefabDom(
            const AZStd::string& prefabFilePath,
            const AzToolsFramework::Prefab::PrefabDom& prefabFileContentDom,
            AZ::IO::ResultCode expectedReadResultCode = AZ::IO::ResultCode::Success,
            AZ::IO::ResultCode expectedOpenResultCode = AZ::IO::ResultCode::Success,
            AZ::IO::ResultCode expectedSizeResultCode = AZ::IO::ResultCode::Success,
            AZ::IO::ResultCode expectedCloseResultCode = AZ::IO::ResultCode::Success);

        void ReadPrefabDom(
            const AZStd::string& prefabFilePath,
            const AZStd::string& prefabFileContent,
            AZ::IO::ResultCode expectedReadResultCode = AZ::IO::ResultCode::Success,
            AZ::IO::ResultCode expectedOpenResultCode = AZ::IO::ResultCode::Success,
            AZ::IO::ResultCode expectedSizeResultCode = AZ::IO::ResultCode::Success,
            AZ::IO::ResultCode expectedCloseResultCode = AZ::IO::ResultCode::Success);

    private:
        // A counter for creating new file handles.
        AZStd::atomic<AZ::IO::HandleType> m_fileHandleCounter = 1u;

        // A mock file io for testing.
        AZStd::unique_ptr<testing::NiceMock<AZ::IO::MockFileIOBase>> m_fileIOMock;

        // A cache for the existing file io.
        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
    };
}
