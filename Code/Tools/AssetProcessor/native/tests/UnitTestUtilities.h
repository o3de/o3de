/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <utilities/AssetUtilEBusHelper.h>
#include <utilities/assetUtils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Interface/Interface.h>
#include <gmock/gmock.h>
#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>
#include <AssetManager/FileStateCache.h>
#include <AzToolsFramework/Asset/AssetUtils.h>

namespace UnitTests
{
    //! Utility class meant to check that a specific number of errors occur - will cause a test failure if any unexpected errors occur
    //! This does not suppress anything unless Begin has been called
    struct TraceBusErrorChecker : AZ::Debug::TraceMessageBus::Handler
    {
        TraceBusErrorChecker();
        virtual ~TraceBusErrorChecker();

        void Begin();
        void End(int expectedCount);

    private:
        bool OnPreAssert(const char* fileName, int line, const char* func, const char* message) override;
        bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message) override;
        bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override;

        void RecordError(const char* fileName, int line, const char* func, const char* message);

        bool m_expectingFailure = false;
        AZStd::vector<AZStd::string> m_suppressedMessages;
    };

    struct MockMultiBuilderInfoHandler : public AssetProcessor::AssetBuilderInfoBus::Handler
    {
        ~MockMultiBuilderInfoHandler() override;

        struct AssetBuilderExtraInfo
        {
            QString m_jobFingerprint;
            QString m_dependencyFilePath;
            QString m_jobDependencyFilePath;
            QString m_analysisFingerprint;
            AZStd::vector<AZ::u32> m_subIdDependencies;
        };

        //! AssetProcessor::AssetBuilderInfoBus Interface
        void GetMatchingBuildersInfo(const AZStd::string& assetPath, AssetProcessor::BuilderInfoList& builderInfoList) override;
        void GetAllBuildersInfo(AssetProcessor::BuilderInfoList& builderInfoList) override;

        void CreateJobs(
            AssetBuilderExtraInfo extraInfo,
            const AssetBuilderSDK::CreateJobsRequest& request,
            AssetBuilderSDK::CreateJobsResponse& response);
        void ProcessJob(
            AssetBuilderExtraInfo extraInfo,
            const AssetBuilderSDK::ProcessJobRequest& request,
            AssetBuilderSDK::ProcessJobResponse& response);

        void CreateBuilderDesc(
            const QString& builderName,
            const QString& builderId,
            const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns,
            const AssetBuilderExtraInfo& extraInfo);

        // Use this version if you intend to update the extraInfo struct dynamically (be sure extraInfo does not go out of scope)
        void CreateBuilderDescInfoRef(
            const QString& builderName,
            const QString& builderId,
            const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns,
            const AssetBuilderExtraInfo& extraInfo);

        void CreateBuilderDesc(
            const QString& builderName,
            const QString& builderId,
            const AZStd::vector<AssetBuilderSDK::AssetBuilderPattern>& builderPatterns,
            const AssetBuilderSDK::CreateJobFunction& createJobsFunction,
            const AssetBuilderSDK::ProcessJobFunction& processJobFunction,
            AZStd::optional<QString> analysisFingerprint = AZStd::nullopt);

        AZStd::vector<AssetUtilities::BuilderFilePatternMatcher> m_matcherBuilderPatterns;
        AZStd::unordered_map<AZ::Uuid, AssetBuilderSDK::AssetBuilderDesc> m_builderDescMap;

        int m_createJobsCount = 0;
    };

    class MockComponentApplication
        : public AZ::ComponentApplicationBus::Handler
    {
    public:
        MockComponentApplication();
        ~MockComponentApplication();

    public:
        MOCK_METHOD1(FindEntity, AZ::Entity* (const AZ::EntityId&));
        MOCK_METHOD1(AddEntity, bool(AZ::Entity*));
        MOCK_METHOD0(Destroy, void());
        MOCK_METHOD1(RegisterComponentDescriptor, void(const AZ::ComponentDescriptor*));
        MOCK_METHOD1(UnregisterComponentDescriptor, void(const AZ::ComponentDescriptor*));
        MOCK_METHOD1(RegisterEntityAddedEventHandler, void(AZ::EntityAddedEvent::Handler&));
        MOCK_METHOD1(RegisterEntityRemovedEventHandler, void(AZ::EntityRemovedEvent::Handler&));
        MOCK_METHOD1(RegisterEntityActivatedEventHandler, void(AZ::EntityActivatedEvent::Handler&));
        MOCK_METHOD1(RegisterEntityDeactivatedEventHandler, void(AZ::EntityDeactivatedEvent::Handler&));
        MOCK_METHOD1(SignalEntityActivated, void(AZ::Entity*));
        MOCK_METHOD1(SignalEntityDeactivated, void(AZ::Entity*));
        MOCK_METHOD1(RemoveEntity, bool(AZ::Entity*));
        MOCK_METHOD1(DeleteEntity, bool(const AZ::EntityId&));
        MOCK_METHOD1(GetEntityName, AZStd::string(const AZ::EntityId&));
        MOCK_METHOD1(EnumerateEntities, void(const ComponentApplicationRequests::EntityCallback&));
        MOCK_METHOD0(GetApplication, AZ::ComponentApplication* ());
        MOCK_METHOD0(GetSerializeContext, AZ::SerializeContext* ());
        MOCK_METHOD0(GetJsonRegistrationContext, AZ::JsonRegistrationContext* ());
        MOCK_METHOD0(GetBehaviorContext, AZ::BehaviorContext* ());
        MOCK_CONST_METHOD0(GetEngineRoot, const char* ());
        MOCK_CONST_METHOD0(GetExecutableFolder, const char* ());
        MOCK_CONST_METHOD1(QueryApplicationType, void(AZ::ApplicationTypeQuery&));
    };

    struct MockPathConversion : AZ::Interface<AssetProcessor::IPathConversion>::Registrar
    {
        MockPathConversion(const char* scanfolder = "c:/somepath");
        bool ConvertToRelativePath(QString fullFileName, QString& databaseSourceName, QString& scanFolderName) const override;
        const AssetProcessor::ScanFolderInfo* GetScanFolderForFile(const QString& /*fullFileName*/) const override;
        const AssetProcessor::ScanFolderInfo* GetScanFolderById(AZ::s64 /*id*/) const override;
        void SetScanFolder(const AssetProcessor::ScanFolderInfo& scanFolderInfo);

    private:
        AssetProcessor::ScanFolderInfo m_scanFolderInfo;
    };

    struct MockMultiPathConversion : AZ::Interface<AssetProcessor::IPathConversion>::Registrar
    {
        bool ConvertToRelativePath(QString fullFileName, QString& databaseSourceName, QString& scanFolderName) const override;
        const AssetProcessor::ScanFolderInfo* GetScanFolderForFile(const QString& fullFileName) const override;
        const AssetProcessor::ScanFolderInfo* GetScanFolderById(AZ::s64 id) const override;
        void AddScanfolder(QString path, QString name);

    private:
        AZStd::vector<AssetProcessor::ScanFolderInfo> m_scanFolderInfo;
    };

    struct MockVirtualFileIO
    {
        static constexpr AZ::u32 ComputeHandle(AZ::IO::PathView path);

        MockVirtualFileIO();
        ~MockVirtualFileIO();

        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZStd::unordered_map<AZ::IO::HandleType, AZStd::pair<AZStd::string, AZStd::string>> m_mockFiles;
        AZStd::unique_ptr<testing::NiceMock<AZ::IO::MockFileIOBase>> m_fileIOMock;
    };

    struct MockFileStateCache : AssetProcessor::FileStateBase
    {
        bool GetFileInfo(const QString& absolutePath, AssetProcessor::FileStateInfo* foundFileInfo) const override;
        bool Exists(const QString& absolutePath) const override;

        bool GetHash(const QString& absolutePath, FileHash* foundHash) override;
        void RegisterForDeleteEvent(AZ::Event<AssetProcessor::FileStateInfo>::Handler& handler) override;

        AZ::Event<AssetProcessor::FileStateInfo> m_deleteEvent;
    };
}
