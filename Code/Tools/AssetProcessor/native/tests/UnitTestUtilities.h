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
#include <QDir>

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
        MockComponentApplication()
        {
            AZ::ComponentApplicationBus::Handler::BusConnect();
            AZ::Interface<AZ::ComponentApplicationRequests>::Register(this);
        }

        ~MockComponentApplication()
        {
            AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(this);
            AZ::ComponentApplicationBus::Handler::BusDisconnect();
        }

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
        MockPathConversion(const char* scanfolder = "c:/somepath")
        {
            m_scanFolderInfo = AssetProcessor::ScanFolderInfo{ scanfolder, "scanfolder", "scanfolder", true, true, { AssetBuilderSDK::PlatformInfo{ "pc", {} } }, 0, 1 };
        }

        bool ConvertToRelativePath(QString fullFileName, QString& databaseSourceName, QString& scanFolderName) const override
        {
            EXPECT_TRUE(fullFileName.startsWith(m_scanFolderInfo.ScanPath(), Qt::CaseInsensitive));

            scanFolderName = m_scanFolderInfo.ScanPath();
            databaseSourceName = fullFileName.mid(scanFolderName.size() + 1);

            return true;
        }

        const AssetProcessor::ScanFolderInfo* GetScanFolderForFile(const QString& /*fullFileName*/) const override
        {
            return &m_scanFolderInfo;
        }

        const AssetProcessor::ScanFolderInfo* GetScanFolderById(AZ::s64 /*id*/) const override
        {
            return &m_scanFolderInfo;
        }

    private:
        AssetProcessor::ScanFolderInfo m_scanFolderInfo;
    };

    struct MockMultiPathConversion : AZ::Interface<AssetProcessor::IPathConversion>::Registrar
    {
        bool ConvertToRelativePath(QString fullFileName, QString& databaseSourceName, QString& scanFolderName) const override
        {
            auto scanfolder = GetScanFolderForFile(fullFileName);
            EXPECT_TRUE(scanfolder);

            scanFolderName = scanfolder->ScanPath();
            databaseSourceName = fullFileName.mid(scanFolderName.size() + 1);

            return true;
        }

        const AssetProcessor::ScanFolderInfo* GetScanFolderForFile(const QString& fullFileName) const override
        {
            for (const auto& scanfolder : m_scanFolderInfo)
            {
                if (AZ::IO::PathView(fullFileName.toUtf8().constData())
                    .IsRelativeTo(AZ::IO::PathView(scanfolder.ScanPath().toUtf8().constData())))
                {
                    return &scanfolder;
                }
            }

            return nullptr;
        }

        const AssetProcessor::ScanFolderInfo* GetScanFolderById(AZ::s64 id) const override
        {
            return &m_scanFolderInfo[id - 1];
        }

        void AddScanfolder(QString path, QString name)
        {
            m_scanFolderInfo.push_back(
                AssetProcessor::ScanFolderInfo(path, name, name, false, true, { AssetBuilderSDK::PlatformInfo{ "pc", {} } }, 0, m_scanFolderInfo.size() + 1));
        }

    private:
        AZStd::vector<AssetProcessor::ScanFolderInfo> m_scanFolderInfo;
    };

    struct MockVirtualFileIO
    {
        static constexpr AZ::u32 ComputeHandle(AZ::IO::PathView path)
        {
            return AZ::u32(AZStd::hash<AZ::IO::PathView>{}(path));
        }

        MockVirtualFileIO()
        {
            // Cache the existing file io instance and build our mock file io
            m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            m_fileIOMock = AZStd::make_unique<testing::NiceMock<AZ::IO::MockFileIOBase>>();

            // Swap out current file io instance for our mock
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_fileIOMock.get());

            // Setup the default returns for our mock file io calls
            AZ::IO::MockFileIOBase::InstallDefaultReturns(*m_fileIOMock.get());

            using namespace ::testing;
            using namespace AZ;

            ON_CALL(*m_fileIOMock, Open(_, _, _))
                .WillByDefault(Invoke(
                    [this](auto filePath, IO::OpenMode mode, IO::HandleType& handle)
                    {
                        handle = ComputeHandle(filePath);

                        int systemMode = AZ::IO::TranslateOpenModeToSystemFileMode(filePath, mode);

                        // Any mode besides OPEN_READ_ONLY creates a file
                        if ((systemMode & ~int(IO::SystemFile::SF_OPEN_READ_ONLY)) > 0)
                        {
                            m_mockFiles[handle] = { filePath, "" };
                        }

                        return AZ::IO::Result(AZ::IO::ResultCode::Success);
                    }));

            ON_CALL(*m_fileIOMock, Tell(_, _))
                .WillByDefault(Invoke(
                    [](auto, auto& offset)
                    {
                        offset = 0;
                        return AZ::IO::ResultCode::Success;
                    }));

            ON_CALL(*m_fileIOMock, Size(An<AZ::IO::HandleType>(), _))
                .WillByDefault(Invoke(
                    [this](auto handle, AZ::u64& size)
                    {
                        auto itr = m_mockFiles.find(handle);

                        size = itr != m_mockFiles.end() ? itr->second.second.size() : 0;

                        return AZ::IO::ResultCode::Success;
                    }));

            ON_CALL(*m_fileIOMock, Size(An<const char*>(), _))
                .WillByDefault(Invoke(
                    [this](const char* filePath, AZ::u64& size)
                    {
                        auto handle = ComputeHandle(filePath);
                        auto itr = m_mockFiles.find(handle);

                        size = itr != m_mockFiles.end() ? itr->second.second.size() : 0;

                        return AZ::IO::ResultCode::Success;
                    }));

            ON_CALL(*m_fileIOMock, Exists(_))
                .WillByDefault(Invoke(
                    [this](const char* filePath)
                    {
                        auto handle = ComputeHandle(filePath);
                        auto itr = m_mockFiles.find(handle);
                        return itr != m_mockFiles.end();
                    }));

            ON_CALL(*m_fileIOMock, Rename(_, _))
                .WillByDefault(Invoke(
                    [this](const char* originalPath, const char* newPath)
                    {
                        auto originalHandle = ComputeHandle(originalPath);
                        auto newHandle = ComputeHandle(newPath);
                        auto itr = m_mockFiles.find(originalHandle);

                        if (itr != m_mockFiles.end())
                        {
                            auto& [path, contents] = itr->second;
                            path = newPath;

                            if (originalHandle != newHandle)
                            {
                                m_mockFiles[newHandle] = itr->second;
                                m_mockFiles.erase(itr);
                            }

                            return AZ::IO::ResultCode::Success;
                        }

                        return AZ::IO::ResultCode::Error;
                    }));

            ON_CALL(*m_fileIOMock, Remove(_))
                .WillByDefault(Invoke(
                    [this](const char* filePath)
                    {
                        auto handle = ComputeHandle(filePath);

                        m_mockFiles.erase(handle);

                        return AZ::IO::ResultCode::Success;
                    }));

            ON_CALL(*m_fileIOMock, Read(_, _, _, _, _))
                .WillByDefault(Invoke(
                    [this](auto handle, void* buffer, auto, auto, AZ::u64* bytesRead)
                    {
                        auto itr = m_mockFiles.find(handle);

                        if (itr == m_mockFiles.end())
                        {
                            return AZ::IO::ResultCode::Error;
                        }

                        memcpy(buffer, itr->second.second.c_str(), itr->second.second.size());
                        *bytesRead = itr->second.second.size();
                        return AZ::IO::ResultCode::Success;
                    }));

            ON_CALL(*m_fileIOMock, Write(_, _, _, _))
                .WillByDefault(Invoke(
                    [this](IO::HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten)
                    {
                        auto& pair = m_mockFiles[fileHandle];

                        pair.second.resize(size);
                        memcpy((void*)pair.second.c_str(), buffer, size);

                        if (bytesWritten)
                        {
                            *bytesWritten = size;
                        }

                        return AZ::IO::ResultCode::Success;
                    }));

            ON_CALL(*m_fileIOMock, FindFiles(_, _, _))
                .WillByDefault(Invoke(
                    [this](const char* filePath, const char* filter, auto callback)
                    {
                        for (const auto& [hash, pair] : m_mockFiles)
                        {
                            const auto& [path, contents] = pair;
                            if(AZStd::wildcard_match(AZStd::string::format("%s%s", filePath, filter), path))
                            {
                                if(!callback(path.c_str()))
                                {
                                    return AZ::IO::ResultCode::Success;
                                }
                            }
                        }

                        return AZ::IO::ResultCode::Success;
                    }));
        }
        ~MockVirtualFileIO()
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
        }

        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZStd::unordered_map<AZ::IO::HandleType, AZStd::pair<AZStd::string, AZStd::string>> m_mockFiles;
        AZStd::unique_ptr<testing::NiceMock<AZ::IO::MockFileIOBase>> m_fileIOMock;
    };

    struct MockFileStateCache : AssetProcessor::FileStateBase
    {
        bool GetFileInfo(const QString& absolutePath, AssetProcessor::FileStateInfo* foundFileInfo) const override
        {
            if (Exists(absolutePath))
            {
                auto* io = AZ::IO::FileIOBase::GetInstance();
                AZ::u64 size;
                io->Size(absolutePath.toUtf8().constData(), size);

                QString parentPath = AZ::IO::FixedMaxPath(absolutePath.toUtf8().constData()).ParentPath().FixedMaxPathStringAsPosix().c_str();
                QString relPath = AZ::IO::FixedMaxPath(absolutePath.toUtf8().constData()).Filename().FixedMaxPathStringAsPosix().c_str();
                AssetUtilities::UpdateToCorrectCase(parentPath, relPath);

                AZ::IO::FixedMaxPath correctedPath{parentPath.toUtf8().constData()};
                correctedPath /= relPath.toUtf8().constData();

                *foundFileInfo = AssetProcessor::FileStateInfo(
                    correctedPath.c_str(),
                    QDateTime::fromMSecsSinceEpoch(io->ModificationTime(absolutePath.toUtf8().constData())),
                    size,
                    io->IsDirectory(absolutePath.toUtf8().constData()));

                return true;
            }

            return false;
        }

        bool Exists(const QString& absolutePath) const override
        {
            return AZ::IO::FileIOBase::GetInstance()->Exists(absolutePath.toUtf8().constData());
        }

        bool GetHash(const QString& absolutePath, FileHash* foundHash) override;
        void RegisterForDeleteEvent(AZ::Event<AssetProcessor::FileStateInfo>::Handler& handler) override;

        AZ::Event<AssetProcessor::FileStateInfo> m_deleteEvent;
    };
}
