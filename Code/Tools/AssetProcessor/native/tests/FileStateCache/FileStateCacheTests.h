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

#include <AzTest/AzTest.h>
#include <AssetManager/FileStateCache.h>
#include <QTemporaryDir>

namespace UnitTests
{
    using namespace testing;
    using ::testing::NiceMock;
    using namespace AssetProcessor;

    class FileStateCacheTests : public ::testing::Test
    {
    public:
        void SetUp() override;
        void TearDown() override;

        void CheckForFile(QString path, bool shouldExist);

    protected:
        QTemporaryDir m_temporaryDir;
        QDir m_temporarySourceDir;
        AZStd::unique_ptr<FileStateBase> m_fileStateCache;
    };
}
