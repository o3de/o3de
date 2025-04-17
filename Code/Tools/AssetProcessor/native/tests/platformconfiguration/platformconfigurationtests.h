/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <QCoreApplication>
#include <native/tests/AssetProcessorTest.h>
#include <native/unittests/UnitTestUtils.h>
#include <native/utilities/PlatformConfiguration.h>
#include <AssetManager/FileStateCache.h>

class PlatformConfigurationUnitTests
    : public AssetProcessor::AssetProcessorTest
{
public:
    PlatformConfigurationUnitTests();
    virtual ~PlatformConfigurationUnitTests()
    {
    }
protected:
    void SetUp() override;
    void TearDown() override;

private:
    int         m_argc;
    char**      m_argv;
    QCoreApplication* m_qApp;
};

