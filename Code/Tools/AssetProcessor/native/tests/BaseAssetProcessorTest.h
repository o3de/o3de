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

#include <qlogging.h>
#include <QString>

// Environments subclass from AZ::Test::ITestEnvironment
class BaseAssetProcessorTestEnvironment : public AZ::Test::ITestEnvironment
{
public:
    virtual ~BaseAssetProcessorTestEnvironment() {}

protected:
    // Any time Qt emits a warning, critical, or fatal, consider the test to have failed!
    static void UnitTestMessageHandler(QtMsgType type, const QMessageLogContext& /*context*/, const QString& msg)
    {
        switch (type)
        {
        case QtDebugMsg:
            break;
        case QtWarningMsg:
            EXPECT_FALSE("QtWarningMsg") << msg.toUtf8().constData();
            break;
        case QtCriticalMsg:
            EXPECT_FALSE("QtCriticalMsg") << msg.toUtf8().constData();
            break;
        case QtFatalMsg:
            EXPECT_FALSE("QtFatalMsg") << msg.toUtf8().constData();
            break;
        }
    }

    // There are two pure-virtual functions to implement, setup and teardown
    void SetupEnvironment() override
    {
        // Setup code
        qInstallMessageHandler(UnitTestMessageHandler);
    }

    void TeardownEnvironment() override
    {
        qInstallMessageHandler(nullptr);
    }

private:
    // Put members that need to be maintained throughout testing lifecycle here
    // Don't declare them in the setup/teardown functions!
};

