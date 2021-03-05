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

#include "native/tests/AssetProcessorTest.h"
#include <QCoreApplication>
#include "native/assetprocessor.h"
#include <AzFramework/Asset/AssetSystemTypes.h>

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
};
