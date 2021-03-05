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

#include <native/tests/AssetProcessorTest.h>
#include <QTemporaryDir>
#include <QCoreApplication>
#include <native/utilities/PlatformConfiguration.h>
#include <QSet>
#include <QString>

namespace AssetProcessor
{
    class AssetScanner_Test;
    class AssetScannerTest
        : public AssetProcessor::AssetProcessorTest
    {
    public:
        AssetScannerTest();
        // Blocks and runs the QT event pump for up to millisecondsMax and will break out as soon as the scan completes.
        bool BlockUntilScanComplete(int millisecondsMax);

    protected:
        void SetUp() override;
        void TearDown() override;
        int         m_argc;
        char**      m_argv;
        QTemporaryDir m_tempDir;
        AZStd::unique_ptr<PlatformConfiguration> m_platformConfig;
        AZStd::unique_ptr<AssetScanner_Test> m_assetScanner;
        QSet<QString> m_files;
        QSet<QString> m_folders;
        bool m_scanComplete = false;
        AZStd::unique_ptr<QCoreApplication> m_qApp;
    };
}
