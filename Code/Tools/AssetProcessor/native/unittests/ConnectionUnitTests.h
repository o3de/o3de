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

#if !defined(Q_MOC_RUN)
#include "native/connection/connection.h"
#include <AzTest/AzTest.h>
#include "UnitTestRunner.h"
#include <native/utilities/PlatformConfiguration.h>
#endif

namespace AssetProcessor
{
    class PlatformConfiguration;
}

class ConnectionForSendTest : public Connection
{
public:
    ConnectionForSendTest() : Connection() {}
    ~ConnectionForSendTest() = default;

    ConnectionForSendTest(ConnectionForSendTest&) = delete;
    ConnectionForSendTest(ConnectionForSendTest&&) = delete;
    MOCK_METHOD2(Send, size_t(unsigned int /*serial*/, const AzFramework::AssetSystem::BaseAssetProcessorMessage& /*message*/));
};

class ConnectionUnitTest
    : public UnitTestRun
{
    Q_OBJECT
public:
    ConnectionUnitTest() = default;
    virtual void StartTest() override;
    virtual int UnitTestPriority() const override;
    virtual ~ConnectionUnitTest();

private:
    ::testing::NiceMock<ConnectionForSendTest> m_testConnection;
};

