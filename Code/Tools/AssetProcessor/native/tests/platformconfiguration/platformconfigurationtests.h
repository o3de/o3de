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
#include <QCoreApplication>
#include "native/tests/AssetProcessorTest.h"
#include "native/unittests/UnitTestRunner.h"
#include "native/utilities/PlatformConfiguration.h"
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
    UnitTestUtils::AssertAbsorber m_absorber;
    AssetProcessor::FileStatePassthrough m_fileStateCache;

private:
    int         m_argc;
    char**      m_argv;
    QCoreApplication* m_qApp;
    
};

