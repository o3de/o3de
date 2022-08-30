/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <native/tests/assetmanager/AssetManagerTestingBase.h>

namespace UnitTests
{
    class IntermediateAssetTests
        : public AssetManagerTestingBase
        , protected AZ::Debug::TraceMessageBus::Handler
    {
    public:
        void SetUp() override;
        void TearDown() override;

    protected:
        bool OnPreAssert(const char*, int, const char*, const char* message) override;
        bool OnPreError(const char*, const char*, int, const char*, const char* message) override;

        AZStd::string MakePath(const char* filename, bool intermediate);

        void CheckProduct(const char* relativePath, bool exists = true);
        void CheckIntermediate(const char* relativePath, bool exists = true);
        void ProcessSingleStep(int expectedJobCount = 1, int expectedFileCount = 1, int jobToRun = 0, bool expectSuccess = true);

        void ProcessFileMultiStage(
            int endStage,
            bool doProductOutputCheck,
            const char* file = nullptr,
            int startStage = 1,
            bool expectAutofail = false,
            bool hasExtraFile = false);

        void DeleteIntermediateTest(const char* fileToDelete);

        void IncorrectBuilderConfigurationTest(bool commonPlatform, AssetBuilderSDK::ProductOutputFlags flags);

        void CreateBuilder(
            const char* name,
            const char* inputFilter,
            const char* outputExtension,
            bool createJobCommonPlatform,
            AssetBuilderSDK::ProductOutputFlags outputFlags,
            bool outputExtraFile = false);

        AZStd::unique_ptr<AssetProcessor::RCController> m_rc;
        bool m_fileCompiled = false;
        bool m_fileFailed = false;
        AssetProcessor::JobEntry m_processedJobEntry;
        AssetBuilderSDK::ProcessJobResponse m_processJobResponse;
        AZStd::string m_testFilePath;

        int m_expectedErrors = 0;
    };
} // namespace UnitTests
