/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <native/unittests/AssetProcessorUnitTests.h>

class FileWatcherUnitTest
    : public QObject
    , public UnitTest::AssetProcessorUnitTestBase
{
    Q_OBJECT

public:
    void SetUp() override;
    void TearDown() override;

protected:
    AZStd::unique_ptr<FileWatcher> m_fileWatcher;
    QString m_assetRootPath;
};
