/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <qcoreapplication.h>
#include "../../utilities/assetUtils.h"
#include "../../resourcecompiler/RCBuilder.h"
#include "native/tests/AssetProcessorTest.h"

using namespace AssetProcessor;

extern const BuilderIdAndName BUILDER_ID_COPY;
extern const BuilderIdAndName BUILDER_ID_RC;
extern const BuilderIdAndName BUILDER_ID_SKIP;

class MockRCCompiler
    : public AssetProcessor::RCCompiler
{
public:
    MockRCCompiler()
        : m_executeResultResult(0, false, "c:\temp")
    {
    }

    bool Initialize() override
    {
        m_initialize++;
        return m_initializeResult;
    }
    bool Execute([[maybe_unused]] const QString& inputFile, [[maybe_unused]] const QString& watchFolder, [[maybe_unused]] const QString& platformIdentifier, [[maybe_unused]] const QString& params, [[maybe_unused]] const QString& dest, 
        [[maybe_unused]] const AssetBuilderSDK::JobCancelListener* jobCancelListener, Result& result) const override
    {
        m_execute++;
        result = m_executeResultResult;
        return m_executeResult;
    }
    void RequestQuit() override
    {
        m_request_quit++;
    }

    void ResetCounters()
    {
        this->m_initialize = 0;
        this->m_execute = 0;
        this->m_request_quit = 0;
    }

    void SetResultInitialize(bool result)
    {
        m_initializeResult = result;
    }

    void SetResultExecute(bool result)
    {
        m_executeResult = result;
    }

    void SetResultResultExecute(Result result)
    {
        m_executeResultResult = result;
    }

    bool m_initializeResult = true;
    bool m_executeResult = true;
    Result m_executeResultResult;
    mutable int m_initialize = 0;
    mutable int m_execute = 0;
    mutable int m_request_quit = 0;
};


struct MockRecognizerConfiguration
    : public RecognizerConfiguration
{
    virtual ~MockRecognizerConfiguration() = default;

    const RecognizerContainer& GetAssetRecognizerContainer() const override
    {
        return m_recognizerContainer;
    }
    const ExcludeRecognizerContainer& GetExcludeAssetRecognizerContainer() const override
    {
        return m_excludeContainer;
    }

    RecognizerContainer m_recognizerContainer;
    ExcludeRecognizerContainer m_excludeContainer;
};

struct TestInternalRecognizerBasedBuilder
    : public InternalRecognizerBasedBuilder
{
    TestInternalRecognizerBasedBuilder(RCCompiler* rcCompiler = nullptr)
        : InternalRecognizerBasedBuilder()
    {
        if (rcCompiler != nullptr)
        {
            m_rcCompiler.reset(rcCompiler);
        }
    }

    bool FindRC([[maybe_unused]] QString& rcPathOut) override
    {
        return true;
    }

    QFileInfoList GetFilesInDirectory([[maybe_unused]] const QString& directoryPath) override
    {
        QFileInfoList   mockFileInfoList;
        mockFileInfoList.append(m_testFileInfo);
        return mockFileInfoList;
    }

    bool SaveProcessJobRequestFile(const char* /*requestFileDir*/, const char* /*requestFileName*/, const AssetBuilderSDK::ProcessJobRequest& /*request*/) override
    {
        m_savedProcessJob = true;
        return true;
    }
    
    // returns false only if there is a critical failure.
    bool LoadProcessJobResponseFile(const char* /*responseFileDir*/, const char* /*responseFileName*/, AssetBuilderSDK::ProcessJobResponse& /*response*/, bool& /*responseLoaded*/) override
    {
        m_loadedProcessJob = true;
        return true;
    }


    void TestProcessJob(const AssetBuilderSDK::ProcessJobRequest& request,
        AssetBuilderSDK::ProcessJobResponse& response)
    {
        InternalRecognizerBasedBuilder::ProcessJob(request, response);
    }

    void TestProcessLegacyRCJob(const AssetBuilderSDK::ProcessJobRequest& request,
        QString rcParam,
        AZ::Uuid productAssetType,
        const AssetBuilderSDK::JobCancelListener& jobCancelListener,
        AssetBuilderSDK::ProcessJobResponse& response)
    {
        InternalRecognizerBasedBuilder::ProcessLegacyRCJob(request, rcParam, productAssetType, jobCancelListener, response);
    }

    void TestProcessCopyJob(const AssetBuilderSDK::ProcessJobRequest& request,
        AZ::Uuid productAssetType,
        const AssetBuilderSDK::JobCancelListener& jobCancelListener,
        AssetBuilderSDK::ProcessJobResponse& response)
    {
        const bool outputProductDependency = false;
        InternalRecognizerBasedBuilder::ProcessCopyJob(request, productAssetType, outputProductDependency, jobCancelListener, response);
    }


    TestInternalRecognizerBasedBuilder& AddTestFileInfo(const QString& testFileFullPath)
    {
        QFileInfo testFileInfo(testFileFullPath);
        m_testFileInfo.push_back(testFileInfo);
        return *this;
    }

    AZ::u32 AddTestRecognizer(QString builderID, QString extraRCParam, QString platformString)
    {
        // Create a dummy test recognizer
        AssetBuilderSDK::FilePatternMatcher patternMatcher;
        QString versionZero("0");
        AZ::Data::AssetType productAssetType = AZ::Uuid::CreateRandom();

        AssetRecognizer baseAssetRecognizer(QString("test-").append(extraRCParam), false, 1, false, false, patternMatcher, versionZero, productAssetType, false);

        QHash<QString, AssetPlatformSpec> assetPlatformSpecByPlatform;
        AssetPlatformSpec   assetSpec;
        assetSpec.m_extraRCParams = extraRCParam;
        assetPlatformSpecByPlatform[platformString] = assetSpec;

        InternalAssetRecognizer* pTestInternalRecognizer = new InternalAssetRecognizer(baseAssetRecognizer, builderID, assetPlatformSpecByPlatform);
        this->m_assetRecognizerDictionary[pTestInternalRecognizer->m_paramID] = pTestInternalRecognizer;
        return pTestInternalRecognizer->m_paramID;
    }

    void TestProcessRCResultFolder(const QString &dest, const AZ::Uuid& productAssetType, bool responseFromRCCompiler, AssetBuilderSDK::ProcessJobResponse &response)
    {
        ProcessRCResultFolder(dest, productAssetType, responseFromRCCompiler, response);
    }

    QList<QFileInfo>    m_testFileInfo;
    bool m_savedProcessJob = false;
    bool m_loadedProcessJob = false;
};


class RCBuilderTest
    : public AssetProcessor::AssetProcessorTest
{
    int         m_argc;
    char**      m_argv;

    QCoreApplication* m_qApp = nullptr;
public:
    RCBuilderTest()
        : m_argc(0)
        , m_argv(0)
    {
        m_qApp = new QCoreApplication(m_argc, m_argv);
    }
    virtual ~RCBuilderTest()
    {
        delete m_qApp;
    }

    AZ::Uuid GetBuilderUUID() const
    {
        AZ::Uuid    rcUuid;
        AssetProcessor::BUILDER_ID_RC.GetUuid(rcUuid);
        return rcUuid;
    }

    AZStd::string GetBuilderName() const
    {
        return AZStd::string(AssetProcessor::BUILDER_ID_RC.GetName().toUtf8().data());
    }

    QString GetBuilderID() const
    {
        return AssetProcessor::BUILDER_ID_RC.GetId();
    }

    AssetBuilderSDK::ProcessJobRequest CreateTestJobRequest(const AZStd::string& testFileName, bool critical, QString platform, AZ::s64 jobId = 0)
    {
        AssetBuilderSDK::ProcessJobRequest request;
        request.m_builderGuid = this->GetBuilderUUID();
        request.m_sourceFile = testFileName;
        request.m_fullPath = AZStd::string("c:\\temp\\") + testFileName;
        request.m_tempDirPath = "c:\\temp";
        request.m_jobDescription.m_critical = critical;
        request.m_jobDescription.SetPlatformIdentifier(platform.toUtf8().constData());
        request.m_jobId = jobId;
        return request;
    }
};

