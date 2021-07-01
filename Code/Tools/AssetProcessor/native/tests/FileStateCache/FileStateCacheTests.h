/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
