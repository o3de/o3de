/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AZ::IO::PathView prefabFilePath,
            const AzToolsFramework::Prefab::PrefabDom& prefabFileContentDom,
            AZ::IO::ResultCode expectedReadResultCode = AZ::IO::ResultCode::Success,
            AZ::IO::ResultCode expectedOpenResultCode = AZ::IO::ResultCode::Success,
            AZ::IO::ResultCode expectedSizeResultCode = AZ::IO::ResultCode::Success,
            AZ::IO::ResultCode expectedCloseResultCode = AZ::IO::ResultCode::Success);

        void ReadPrefabDom(
            AZ::IO::PathView prefabFilePath,
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
