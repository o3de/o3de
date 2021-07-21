/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#if !defined(Q_MOC_RUN)
#include "UnitTestRunner.h"
#include <native/utilities/AssetUtilEBusHelper.h>
#endif

namespace AssetProcessor
{
    class AssetProcessorManagerUnitTests
        : public UnitTestRun
    {
        Q_OBJECT
    public:
        virtual void StartTest() override;

Q_SIGNALS:
        void Test_Emit_AssessableFile(QString filePath);
        void Test_Emit_AssessableDeletedFile(QString filePath);
    };

    class AssetProcessorManagerUnitTests_ScanFolders
        : public UnitTestRun
    {
        Q_OBJECT
    public:
        virtual void StartTest() override;
    };

    class AssetProcessorManagerUnitTests_JobKeys
        : public UnitTestRun
    {
        Q_OBJECT
    public:
        virtual void StartTest() override;
    };

    class AssetProcessorManagerUnitTests_JobDependencies_Fingerprint
        : public UnitTestRun
        , public AssetProcessor::AssetBuilderInfoBus::Handler
    {
        Q_OBJECT
    public:
        virtual void StartTest() override;

        // AssetProcessor::AssetBuilderInfoBus::Handler
        void GetMatchingBuildersInfo(const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& builderInfoList) override;
        void GetAllBuildersInfo(AssetProcessor::BuilderInfoList& builderInfoList) override;

        AssetBuilderSDK::AssetBuilderDesc m_assetBuilderDesc;
    };
} // namespace assetprocessor

