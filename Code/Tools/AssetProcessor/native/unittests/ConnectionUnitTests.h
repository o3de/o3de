/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

