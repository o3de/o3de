/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "native/tests/AssetProcessorTest.h"
#include <QCoreApplication>
#include "native/assetprocessor.h"
#include <AzFramework/Asset/AssetSystemTypes.h>
#include <native/tests/UnitTestUtilities.h>

class RCcontrollerTest
    : public AssetProcessor::AssetProcessorTest
{
public:
    RCcontrollerTest()
        : m_argc(0)
        , m_argv(0)
    {
        m_qApp = new QCoreApplication(m_argc, m_argv);

        qRegisterMetaType<AzFramework::AssetSystem::AssetStatus>("AzFramework::AssetSystem::AssetStatus");
        qRegisterMetaType<AssetProcessor::NetworkRequestID>("NetworkRequestID");
    }
    virtual ~RCcontrollerTest()
    {
        delete m_qApp;
    }
private:
    int         m_argc;
    char**      m_argv;

    QCoreApplication* m_qApp;
    UnitTests::MockPathConversion m_mockPathConversion;
};
